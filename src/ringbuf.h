/*

// TODO: assert that it's a power of 2.
#define MAX_REQUESTS 32

typedef struct {
	char pad;
} request_t;

typedef struct {
	request_t data[MAX_REQUESTS];
	size_t read;
	size_t write;
} request_ringbuf_t;

static bool is_full(const request_ringbuf_t* rb) {
	return rb->read + MAX_REQUESTS == rb->write;
}

static size_t pending_requests(const request_ringbuf_t* rb) {
	return rb->write - rb->read;
}

static request_t* write_request(request_ringbuf_t* rb) {
	assert(!is_full(rb));
	request_t* r = &rb->data[rb->write & (MAX_REQUESTS - 1)];
	rb->write++;
	return r;
}

static void read_request(request_ringbuf_t* rb, request_t* r) {
	assert(pending_requests(rb) > 0);
	memcpy(r, &rb->data[rb->read & (MAX_REQUESTS - 1)], sizeof(request_t));
	rb->read++;
}

*/
