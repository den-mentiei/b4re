#include "http.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h> // memcpy
#include <time.h> // timespec

#include <curl.h>
#include <tinycthread.h>

#include "allocator.h"
#include "log.h"

// https://curl.haxx.se/libcurl/c/CURLOPT_PRIVATE.html
// https://curl.haxx.se/libcurl/c/curl_multi_timeout.html
	
/* // TODO: assert that it's a power of 2. */
/* #define MAX_REQUESTS 32 */

/* typedef struct { */
/* 	char pad; */
/* } request_t; */

/* typedef struct { */
/* 	request_t data[MAX_REQUESTS]; */
/* 	size_t read; */
/* 	size_t write; */
/* } request_ringbuf_t; */

/* static bool is_full(const request_ringbuf_t* rb) { */
/* 	return rb->read + MAX_REQUESTS == rb->write; */
/* } */

/* static size_t pending_requests(const request_ringbuf_t* rb) { */
/* 	return rb->write - rb->read; */
/* } */

/* static request_t* write_request(request_ringbuf_t* rb) { */
/* 	assert(!is_full(rb)); */
/* 	request_t* r = &rb->data[rb->write & (MAX_REQUESTS - 1)]; */
/* 	rb->write++; */
/* 	return r; */
/* } */

/* static void read_request(request_ringbuf_t* rb, request_t* r) { */
/* 	assert(pending_requests(rb) > 0); */
/* 	memcpy(r, &rb->data[rb->read & (MAX_REQUESTS - 1)], sizeof(request_t)); */
/* 	rb->read++; */
/* } */

static struct {
	mtx_t multi_lock;
	CURLM* multi;
	cnd_t got_work;
	bool stop_worker;
	thrd_t thread;
} ctx = {0};

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
				curl_easy_cleanup(eh);
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
			log_info("[http] worker will sleep til next reqest\n");
			cnd_wait(&ctx.got_work, &ctx.multi_lock);
			log_info("[http] worker awaken!\n");
		}

		exit = ctx.stop_worker;

		// lock is held now.
	}

	log_info("[http] worker dies\n");

	return 0;
}

void http_init(const allocator_t* alloc) {
	if (ctx.multi) return;

	assert(alloc);

	if (mtx_init(&ctx.multi_lock, mtx_plain) != thrd_success)
		log_fatal("[http] failed to create mutex\n");

	if (cnd_init(&ctx.got_work) != thrd_success)
		log_fatal("[http] failed to create a condvar.");

	CURLcode e = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (e != CURLE_OK)
		log_fatal("[http] failed to init curl\n");

	ctx.multi = curl_multi_init();

	if (thrd_create(&ctx.thread, worker, NULL) != thrd_success)
		log_fatal("[http] failed to create a worker thread\n");

	// TODO: curl_global_init_mem - pass memory functions.

#if DEBUG
	const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
	log_info("[http] curl version: %s\n", info->version);
	log_info("[http] curl ssl version: %s\n", info->ssl_version);
	log_info("[http] curl ssl: %d\n", (info->features & CURL_VERSION_SSL) == 1);
	log_info("[http] curl zlib: %d\n", (info->features & CURL_VERSION_LIBZ) == 1);
#endif
}

static CURL* create_easy(const char* url) {
	assert(ctx.multi);
	assert(url);

	CURL* h = curl_easy_init();
	if (!h)
		log_fatal("failed to create an easy curl handle\n");

#if DEBUG
	curl_easy_setopt(h, CURLOPT_HEADER, 1);
	/* curl_easy_setopt(h, CURLOPT_VERBOSE, 1); */
#endif

	curl_easy_setopt(h, CURLOPT_URL, url);

	return h;
}

static void add_to_multi(CURL* h) {
	assert(h);

	mtx_lock(&ctx.multi_lock);

	CURLMcode err = curl_multi_add_handle(ctx.multi, h);
	if (err != CURLM_OK)
		log_fatal("[http] failed to create request - %s\n", curl_multi_strerror(err));

	mtx_unlock(&ctx.multi_lock);
}

void http_get(const char* url) {
	CURL* h = create_easy(url);
	curl_easy_setopt(h, CURLOPT_HTTPGET, 1);
	add_to_multi(h);

	cnd_signal(&ctx.got_work);
}

void http_post_form(const char* url, const http_form_part_t* parts, size_t count) {
	assert(parts);
	assert(count > 0);

	CURL* h = create_easy(url);
	curl_easy_setopt(h, CURLOPT_HTTPPOST, 1);

	curl_mime* m = curl_mime_init(h);
	if (!m)
		log_fatal("[http] failed to create mime data\n");

	for (size_t i = 0; i < count; ++i) {
		curl_mimepart* p = curl_mime_addpart(m);
		curl_mime_name(p, parts[i].key, CURL_ZERO_TERMINATED);
		curl_mime_data(p, parts[i].value, CURL_ZERO_TERMINATED);
	}

	curl_easy_setopt(h, CURLOPT_MIMEPOST, m);

	add_to_multi(h);

	// TODO: curl_mime_free(mime); after perform!
	cnd_signal(&ctx.got_work);
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
}
