#include "http.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>  // memcpy

#include <curl.h>
#include <tinycthread.h>

#include "log.h"

// Must be a power-of-two.
#define REQUESTS_MAX_IN_FLIGHT 64
#define REQUESTS_INDEX_MASK    (REQUESTS_MAX_IN_FLIGHT - 1)
#define REQUESTS_ID_ADD        REQUESTS_MAX_IN_FLIGHT

typedef struct {
	CURL*      h;
	curl_mime* mime;

	void*  buffer;
	size_t free;
	size_t used;

	uint8_t status;
	uint8_t response_code;
} request_t;

typedef struct {
	request_t req;

	http_work_id_t id;
	uint8_t   index;
	uint8_t   next;
} work_t;

typedef struct {
	work_t  items[REQUESTS_MAX_IN_FLIGHT];
	uint8_t enqueue;
	uint8_t dequeue;
} work_table_t;

static struct {
	CURLSH* share;

	mtx_t  multi_lock;
	CURLM* multi;

	cnd_t got_work;
	bool  stop_worker;

	thrd_t thread;

	work_table_t work;
} s_ctx;

// TODO: Handle failed requests!
// This one happens under lock.
static void finish_work(CURL* h) {
	assert(h);

	request_t* req;
	curl_easy_getinfo(h, CURLINFO_PRIVATE, &req);
	assert(req);

	if (req->mime) curl_mime_free(req->mime);

	// TODO: Get more info & record networking stats. See: https://curl.haxx.se/libcurl/c/curl_easy_getinfo.html

	long response_code;
    curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &response_code);
	
	req->response_code = response_code;
	req->status        = HTTP_STATUS_FINISHED;
}

// TODO: There is a bug when worker sleeps *after* got_work is signalled, but it still not got the work to do :(
static int worker(void* arg) {
	mtx_lock(&s_ctx.multi_lock);
	cnd_wait(&s_ctx.got_work, &s_ctx.multi_lock);
	// Lock is held now.

	bool exit;
	while (!exit) {
		CURLM* h = s_ctx.multi;

		int running;
		curl_multi_perform(h, &running);

		CURLMsg* m;
		do {
			int msgs;
			m = curl_multi_info_read(h, &msgs);
			if (m && (m->msg == CURLMSG_DONE)) {
				CURL* eh = m->easy_handle;
				curl_multi_remove_handle(h, eh);
				// TODO: Do it outside of the lock?
				finish_work(eh);
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

			cnd_timedwait(&s_ctx.got_work, &s_ctx.multi_lock, &t);
		} else if (!s_ctx.stop_worker) {
			cnd_wait(&s_ctx.got_work, &s_ctx.multi_lock);
		}

		exit = s_ctx.stop_worker;

		// Lock is held now.
	}

	log_info("[http] Worker died.");
	return 0;
}

static size_t response_write(const char* ptr, size_t size, size_t nmemb, void* userdata) {
	assert(userdata);

	request_t* req = userdata;
	(void)req;

	const size_t bytes = size * nmemb;

	// Plus 1 as it will be zero-terminated.
	if (req->free < bytes + 1) return 0;

	memcpy(req->buffer + req->used, ptr, bytes);
	req->free -= bytes;
	req->used += bytes;
	((uint8_t*)req->buffer)[req->used] = 0;

	return bytes;
}

// HANDLE MANAGEMENT
// =================

static CURL* create_easy(void* userdata) {
	CURL* h = curl_easy_init();
	if (!h) log_fatal("[http] Failed to create an easy curl handle");

#if DEBUG
	/* curl_easy_setopt(h, CURLOPT_VERBOSE, 1); */
#endif
	curl_easy_setopt(h, CURLOPT_SHARE, s_ctx.share);

	// Enables all supported built-in compressions.
	curl_easy_setopt(h, CURLOPT_ACCEPT_ENCODING, "");

	curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, response_write);
	curl_easy_setopt(h, CURLOPT_WRITEDATA, userdata);

	curl_easy_setopt(h, CURLOPT_PRIVATE, userdata);
	return h;
}

static void free_easy(CURL* h) {
	assert(h);

	curl_easy_cleanup(h);
}

static void add_to_multi(CURL* h) {
	assert(h);

	mtx_lock(&s_ctx.multi_lock);

	CURLMcode err = curl_multi_add_handle(s_ctx.multi, h);
	if (err != CURLM_OK) log_fatal("[http] Failed to create request - %s", curl_multi_strerror(err));

	mtx_unlock(&s_ctx.multi_lock);

	cnd_signal(&s_ctx.got_work);
}

// WORK MANAGEMENT
// ===============

static void work_init(work_table_t* ctx) {
	assert(ctx);

	for (size_t i = 0; i < REQUESTS_MAX_IN_FLIGHT; ++i) {
		ctx->items[i].id   = i;
		ctx->items[i].next = i + 1;
	}
	ctx->dequeue = 0;
	ctx->enqueue = REQUESTS_MAX_IN_FLIGHT - 1;
}

static bool work_has(work_table_t* ctx, http_work_id_t id) {
	assert(ctx);

	work_t* item = &ctx->items[id & REQUESTS_INDEX_MASK];
	return item->id == id && item->index != UINT8_MAX;
}

static work_t* work_lookup(work_table_t* ctx, http_work_id_t id) {
	assert(ctx);
	assert(work_has(ctx, id));

	return &ctx->items[id & REQUESTS_INDEX_MASK];
}

static bool work_can_add(work_table_t* ctx) {
	assert(ctx);
	return ctx->dequeue != REQUESTS_MAX_IN_FLIGHT;
}

static http_work_id_t work_add(work_table_t* ctx) {
	assert(ctx);

	if (!work_can_add(ctx)) return 0;

	work_t* item = &ctx->items[ctx->dequeue];
	item->index  = ctx->dequeue;
	item->id    += REQUESTS_ID_ADD;
	ctx->dequeue = item->next;

	return item->id;
}

static void work_remove(work_table_t* ctx, http_work_id_t id) {
	assert(ctx);
	assert(work_has(ctx, id));

	work_t* item                  = &ctx->items[id & REQUESTS_INDEX_MASK];
	item->index                   = UINT8_MAX;
	ctx->items[ctx->enqueue].next = id & REQUESTS_INDEX_MASK;
	ctx->enqueue                  = id & REQUESTS_INDEX_MASK;

	if (ctx->dequeue == REQUESTS_MAX_IN_FLIGHT) ctx->dequeue = ctx->enqueue;
}

// REQUESTS MANAGEMENT
// ===================

static void requests_init() {
	work_init(&s_ctx.work);

	for (size_t i = 0; i < REQUESTS_MAX_IN_FLIGHT; ++i) {
		request_t* req = &s_ctx.work.items[i].req; 
		req->h = create_easy(req);
	}
}

static void requests_shutdown() {
	for (size_t i = 0; i < REQUESTS_MAX_IN_FLIGHT; ++i) {
		free_easy(s_ctx.work.items[i].req.h);
	}
}

static http_work_id_t requests_add(void* buffer, size_t size) {
	assert(buffer);
	assert(size > 0);
	assert(work_can_add(&s_ctx.work));

	http_work_id_t id  = work_add(&s_ctx.work);
	request_t*      req = &work_lookup(&s_ctx.work, id)->req;
	
	req->status = HTTP_STATUS_IN_PROGRESS;
	req->buffer = buffer;
	req->free   = size;
	req->used   = 0;

	return id;
}

// PUBLIC API
// ==========

void http_init() {
	assert(!s_ctx.multi);

	if (mtx_init(&s_ctx.multi_lock, mtx_plain) != thrd_success) log_fatal("[http] Failed to create mutex");
	if (cnd_init(&s_ctx.got_work) != thrd_success) log_fatal("[http] Failed to create a condvar");

	CURLcode e = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (e != CURLE_OK) log_fatal("[http] Failed to init curl");
	
	s_ctx.multi = curl_multi_init();
	s_ctx.share = curl_share_init();

	curl_share_setopt(s_ctx.share, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
	curl_share_setopt(s_ctx.share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);

	// TODO: curl_global_init_mem - pass memory functions.
	
	requests_init();

	if (thrd_create(&s_ctx.thread, worker, NULL) != thrd_success) log_fatal("[http] Failed to create a worker thread");

#if DEBUG
	const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
	log_info("[http] Curl version: %s", info->version);
	log_info("[http] Curl ssl version: %s", info->ssl_version);
	log_info("[http] Curl ssl: %d", (info->features & CURL_VERSION_SSL) == 1);
	log_info("[http] Curl zlib: %d", (info->features & CURL_VERSION_LIBZ) == 1);
#endif
}

void http_shutdown() {
	assert(s_ctx.multi);

	mtx_lock(&s_ctx.multi_lock);
	s_ctx.stop_worker = true;
	mtx_unlock(&s_ctx.multi_lock);

	cnd_signal(&s_ctx.got_work);

	thrd_join(s_ctx.thread, NULL);
	mtx_destroy(&s_ctx.multi_lock);

	requests_shutdown();

	curl_multi_cleanup(s_ctx.multi);
	curl_share_cleanup(s_ctx.share);

	curl_global_cleanup();
}

http_work_id_t http_get(const char* url, void* buffer, size_t size) {
	assert(url);
	
	if (!work_can_add(&s_ctx.work)) return 0;

	http_work_id_t id = requests_add(buffer, size);
	CURL*           h  = work_lookup(&s_ctx.work, id)->req.h;

	curl_easy_setopt(h, CURLOPT_URL, url);
	curl_easy_setopt(h, CURLOPT_HTTPGET, 1);

	add_to_multi(h);

	return id;
}

http_work_id_t http_post_form(const char* url, const http_form_part_t* parts, size_t num_parts, void* buffer, size_t size) {
	assert(url);
	assert(buffer);
	assert(size > 0);

	if (!work_can_add(&s_ctx.work)) return 0;

	http_work_id_t id  = requests_add(buffer, size);
	request_t*     req = &work_lookup(&s_ctx.work, id)->req;
	CURL*          h   = req->h;

	curl_easy_setopt(h, CURLOPT_URL, url);
	curl_easy_setopt(h, CURLOPT_POST, 1);

	if (parts && num_parts > 0) {
		curl_mime* m = curl_mime_init(h);
		if (!m) log_fatal("[http] Failed to create mime data");

		req->mime = m;

		for (size_t i = 0; i < num_parts; ++i) {
			curl_mimepart* p = curl_mime_addpart(m);
			curl_mime_name(p, parts[i].key,   CURL_ZERO_TERMINATED);
			curl_mime_data(p, parts[i].value, CURL_ZERO_TERMINATED);
		}

		curl_easy_setopt(h, CURLOPT_MIMEPOST, m);
	} else {
		curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE, 0);
	}

	add_to_multi(h);

	return id;
}

http_work_id_t http_post(const char* url, void* buffer, size_t size) {
	return http_post_form(url, NULL, 0, buffer, size);
}

// TODO: SYNCHRONIZATION IS LACKING NOW, CAN BE TOTALLY BROKEN.

http_status_t http_status(http_work_id_t id) {
	if (work_has(&s_ctx.work, id)) {		
		return work_lookup(&s_ctx.work, id)->req.status;
	}
	return HTTP_STATUS_UNKNOWN;
}

bool http_response_code(http_work_id_t id, uint8_t* code) {
	assert(code);

	if (!work_has(&s_ctx.work, id)) return false;
	
	request_t* req = &work_lookup(&s_ctx.work, id)->req;

	if (req->status != HTTP_STATUS_FINISHED) return false;

	*code = req->response_code;
	return true;
}

bool http_response_size(http_work_id_t id, size_t* size) {
	assert(size);

	if (!work_has(&s_ctx.work, id)) return false;

	request_t* req = &work_lookup(&s_ctx.work, id)->req;

	if (req->status != HTTP_STATUS_FINISHED) return false;

	*size = req->used;
	return true;
}
