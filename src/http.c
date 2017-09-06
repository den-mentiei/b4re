#include "http.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h> // memcpy, memset
#include <time.h>   // timespec

#include <curl.h>
#include <tinycthread.h>

#include "allocator.h"
#include "pool.h"
#include "log.h"

#define RESPONSE_INITIAL_POOL_CAPACITY 8
#define MAX_RESPONSE_SIZE (16 * 1024 * 1024)

typedef struct {
	curl_mime* mime;

	http_handler_t handler;
	void* handler_payload;

	size_t free;
	size_t size;
	uint8_t data[];
} pending_t;

static struct {
	mtx_t multi_lock;
	CURLM* multi;

	cnd_t got_work;
	bool stop_worker;

	thrd_t thread;

	pool_t* pending;
} ctx = {0};

static void free_easy_handle(CURL* h) {
	assert(h);

	pending_t* p;
	curl_easy_getinfo(h, CURLINFO_PRIVATE, &p);
	assert(p);

	if (p->mime) curl_mime_free(p->mime);

	p->handler(p->data, p->size, p->handler_payload);

	// TODO: pool_free(ctx.pending, r); thread-safety!
	curl_easy_cleanup(h);
}

static int worker(void* arg) {
	int running;

	mtx_lock(&ctx.multi_lock);
	cnd_wait(&ctx.got_work, &ctx.multi_lock);
	// lock is held now.

	bool exit;
	while (!exit) {
		CURLM* h = ctx.multi;

		int running;
		curl_multi_perform(h, &running);

		CURLMsg* m;
		do {
			int msgs;
			m = curl_multi_info_read(h, &msgs);
			if (m && (m->msg == CURLMSG_DONE)) {
				CURL* eh = m->easy_handle;
				curl_multi_remove_handle(h, eh);
				free_easy_handle(eh);
			}
		} while (m);

		if (running > 0) {
			long timeout;
			curl_multi_timeout(h, &timeout);

			// It just means libcurl currently has no stored timeout value.
			if (timeout < 0) timeout = 1000;

			struct timespec t;
			t.tv_sec  = timeout / 1000;
			t.tv_nsec = (timeout % 1000) * 1000000;

			cnd_timedwait(&ctx.got_work, &ctx.multi_lock, &t);
		} else {
			cnd_wait(&ctx.got_work, &ctx.multi_lock);
		}

		exit = ctx.stop_worker;

		// lock is held now.
	}

	log_info("[http] worker dies\n");

	return 0;
}

static size_t response_write(const char* ptr, size_t size, size_t nmemb, void* userdata) {
	assert(userdata);

	pending_t*  p      = (pending_t*)userdata;
	const size_t bytes = size * nmemb;

	// As it will be zero-terminated.
	if (p->free < bytes + 1) return 0;

	memcpy(p->data + p->size, ptr, bytes);
	p->free -= bytes;
	p->size += bytes;
	p->data[p->size] = 0;

	return bytes;
}

static pending_t* create_pending() {
	pending_t* p = pool_alloc(ctx.pending);
	memset(p, 0, sizeof(pending_t));
	p->free = MAX_RESPONSE_SIZE - sizeof(pending_t);
}

static CURL* create_easy(const char* url, pending_t* r) {
	assert(ctx.multi);

	CURL* h = curl_easy_init();
	if (!h) log_fatal("failed to create an easy curl handle\n");

#if DEBUG
	/* curl_easy_setopt(h, CURLOPT_VERBOSE, 1); */
#endif

	curl_easy_setopt(h, CURLOPT_URL, url);
	// Enables all supported built-in compressions.
	curl_easy_setopt(h, CURLOPT_ACCEPT_ENCODING, "");

	curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, response_write);
	curl_easy_setopt(h, CURLOPT_WRITEDATA, r);

	curl_easy_setopt(h, CURLOPT_PRIVATE, r);

	return h;
}

static void add_to_multi(CURL* h) {
	assert(h);

	mtx_lock(&ctx.multi_lock);

	CURLMcode err = curl_multi_add_handle(ctx.multi, h);
	if (err != CURLM_OK) log_fatal("[http] failed to create request - %s\n", curl_multi_strerror(err));

	mtx_unlock(&ctx.multi_lock);
}

void http_init(allocator_t* alloc) {
	if (ctx.multi) return;

	assert(alloc);

	ctx.pending = pool_create(MAX_RESPONSE_SIZE, RESPONSE_INITIAL_POOL_CAPACITY, alloc);

	if (mtx_init(&ctx.multi_lock, mtx_plain) != thrd_success) log_fatal("[http] failed to create mutex\n");
	if (cnd_init(&ctx.got_work) != thrd_success) log_fatal("[http] failed to create a condvar.");

	CURLcode e = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (e != CURLE_OK) log_fatal("[http] failed to init curl\n");

	ctx.multi = curl_multi_init();

	if (thrd_create(&ctx.thread, worker, NULL) != thrd_success) log_fatal("[http] failed to create a worker thread\n");

	// TODO: curl_global_init_mem - pass memory functions.

#if DEBUG
	const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
	log_info("[http] curl version: %s\n", info->version);
	log_info("[http] curl ssl version: %s\n", info->ssl_version);
	log_info("[http] curl ssl: %d\n", (info->features & CURL_VERSION_SSL) == 1);
	log_info("[http] curl zlib: %d\n", (info->features & CURL_VERSION_LIBZ) == 1);
#endif
}

void http_shutdown() {
	if (!ctx.multi) return;

	mtx_lock(&ctx.multi_lock);
	ctx.stop_worker = true;
	mtx_unlock(&ctx.multi_lock);

	cnd_signal(&ctx.got_work);

	thrd_join(ctx.thread, NULL);
	mtx_destroy(&ctx.multi_lock);

	curl_multi_cleanup(ctx.multi);
	curl_global_cleanup();

	pool_destroy(ctx.pending);
}

void http_get(const char* url, http_handler_t handler, void* payload) {
	assert(url);
	assert(handler);

	pending_t* p       = create_pending();
	p->handler         = handler;
	p->handler_payload = payload;

	CURL* h = create_easy(url, p);
	curl_easy_setopt(h, CURLOPT_HTTPGET, 1);
	add_to_multi(h);

	cnd_signal(&ctx.got_work);
}

void http_post_form(const char* url, const http_form_part_t* parts, size_t count, http_handler_t handler, void* payload) {
	assert(url);
	assert(parts && count > 0);
	assert(handler);

	pending_t* p       = create_pending();
	p->handler         = handler;
	p->handler_payload = payload;

	CURL* h = create_easy(url, p);
	curl_easy_setopt(h, CURLOPT_HTTPPOST, 1);

	curl_mime* m = curl_mime_init(h);
	if (!m) log_fatal("[http] failed to create mime data\n");

	p->mime = m;

	for (size_t i = 0; i < count; ++i) {
		curl_mimepart* p = curl_mime_addpart(m);
		curl_mime_name(p, parts[i].key, CURL_ZERO_TERMINATED);
		curl_mime_data(p, parts[i].value, CURL_ZERO_TERMINATED);
	}

	curl_easy_setopt(h, CURLOPT_MIMEPOST, m);

	add_to_multi(h);

	cnd_signal(&ctx.got_work);
}
