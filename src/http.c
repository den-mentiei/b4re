#include "http.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h> // memcpy

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
	bool stop_worker;
	thrd_t thread;
} ctx = {0};

static int worker(void* arg) {
	int running;

	bool exit;
	while (!exit) {
		mtx_lock(&ctx.multi_lock);

		int running;
		// TODO: wait for a condvar if there are no running handles.
		curl_multi_perform(ctx.multi, &running);

		int msgs;
		CURLMsg* m = curl_multi_info_read(ctx.multi, &msgs);
		if (m && (m->msg == CURLMSG_DONE)) {
			CURL* eh = m->easy_handle;
			curl_multi_remove_handle(ctx.multi, eh);
			curl_easy_cleanup(eh);
		}

		exit = ctx.stop_worker;

		mtx_unlock(&ctx.multi_lock);
	}

	return 0;
}

void http_init(const allocator_t* alloc) {
	if (ctx.multi) return;

	assert(alloc);

	if (mtx_init(&ctx.multi_lock, mtx_plain) != thrd_success)
		log_fatal("http: failed to create mutex\n");

	CURLcode e = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (e != CURLE_OK)
		log_fatal("http: failed to init curl\n");

	ctx.multi = curl_multi_init();

	if (thrd_create(&ctx.thread, worker, NULL) != thrd_success)
		log_fatal("http: failed to create a worker thread\n");

	// TODO: curl_global_init_mem - pass memory functions.

#if DEBUG
	const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
	log_info("curl version: %s\n", info->version);
	log_info("curl ssl version: %s\n", info->ssl_version);
	log_info("curl ssl: %d\n", (info->features & CURL_VERSION_SSL) == 1);
	log_info("curl zlib: %d\n", (info->features & CURL_VERSION_LIBZ) == 1);
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
		log_fatal("http: failed to create request - %s\n", curl_multi_strerror(err));

	mtx_unlock(&ctx.multi_lock);
}

void http_get(const char* url) {
	CURL* h = create_easy(url);
	add_to_multi(h);
}

void http_post_form(const char* url, const http_form_part_t* parts, size_t count) {
	assert(parts);
	assert(count > 0);

	CURL* h = create_easy(url);

	curl_mime* m = curl_mime_init(h);
	if (!m)
		log_fatal("http: failed to create mime data\n");

	for (size_t i = 0; i < count; ++i) {
		curl_mimepart* p = curl_mime_addpart(m);
		curl_mime_name(p, parts[i].key, CURL_ZERO_TERMINATED);
		curl_mime_data(p, parts[i].value, CURL_ZERO_TERMINATED);
	}

	curl_easy_setopt(h, CURLOPT_MIMEPOST, m);

	add_to_multi(h);

	// TODO: curl_mime_free(mime); after perform!
}

void http_shutdown() {
	if (!ctx.multi) return;

	mtx_lock(&ctx.multi_lock);
	ctx.stop_worker = true;
	mtx_unlock(&ctx.multi_lock);

	thrd_join(ctx.thread, NULL);
	mtx_destroy(&ctx.multi_lock);

	curl_multi_cleanup(ctx.multi);
	curl_global_cleanup();
}
