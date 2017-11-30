#include "session.h"

#include <assert.h>
#include <string.h> // memset, memcpy, strcpy
#include <stdio.h>  // snprintf

#include <math.h>   // floor
#include <stdlib.h> // random

#include <tinycthread.h>

#include "allocator.h"
#include "log.h"

#include "client.h"

#include "api.h"

#define AUTO_UPDATE_INTERVALS_MS 5000.0f

static struct {
	allocator_t* alloc;

	float t;

	session_t current;
	session_t synced;

	bool is_logged_in;
	bool is_active;

	char username[128];
	char avatar[128];

	mtx_t lock;
} s_ctx;

static bool safe_is_logged_in() {
	mtx_lock(&s_ctx.lock);
	bool is_logged_in = s_ctx.is_logged_in;
	mtx_unlock(&s_ctx.lock);
	return is_logged_in;
}

static bool safe_is_active() {
	mtx_lock(&s_ctx.lock);
	bool is_active = s_ctx.is_active;
	mtx_unlock(&s_ctx.lock);
	return is_active;
}

static uint8_t randi(uint8_t max) {
	return floor(((float)rand() / RAND_MAX) * max);
}

void session_init(struct allocator_t* alloc) {
	assert(alloc);

	s_ctx.alloc = alloc;
	if (mtx_init(&s_ctx.lock, mtx_plain) != thrd_success) log_fatal("[seession] failed to create mutex");

	// TODO: Remove it.
	for (size_t y = 0; y < WORLD_PLANE_SIZE; ++y) {
		for (size_t x = 0; x < WORLD_PLANE_SIZE; ++x) {
			s_ctx.synced.world.locations[x][y].has_data  = true;
			s_ctx.synced.world.locations[x][y].is_hidden = false;
			s_ctx.synced.world.locations[x][y].terrain   = 1 + randi(TERRAIN_WATER_DEEP);
			/* if ((float)rand() / RAND_MAX > 0.5f) { */
			/* 	s_ctx.synced.world.locations[x][y].has_data  = true; */
			/* 	s_ctx.synced.world.locations[x][y].is_hidden = false; */
			/* 	s_ctx.synced.world.locations[x][y].terrain   = 1 + randi(TERRAIN_WATER_DEEP); */
			/* } else { */
			/* 	s_ctx.synced.world.locations[x][y].has_data  = true; */
			/* 	s_ctx.synced.world.locations[x][y].is_hidden = true; */
			/* } */
		}
	}
}

void session_update(float dt) {
	message_t* msg;
	while (client_messages_peek(&msg)) {
		log_info("[session] Got a message!");
		switch (msg->type) {
			case MESSAGE_TYPE_NOOP:
				break;

			case MESSAGE_TYPE_LOGIN:
				s_ctx.is_logged_in = true;
				break;

			case MESSAGE_TYPE_LOGOUT:
				s_ctx.is_logged_in = false;
				break;

			case MESSAGE_TYPE_STATE: {
				// TODO:
				api_state_t* s = (api_state_t*)msg->data;
				log_info("[sesion] t = %ul | %s at %d,%d", s->timestamp, s->player.username, s->player.x, s->player.y);
				break;
			}

			/* case MESSAGE_TYPE_MAP: */
			/* 	break; */

			/* case MESSAGE_TYPE_REVEAL: */
			/* 	break; */

			default: log_fatal("[session] Got an unknown message!");
		}
		client_messages_consume();
	}

	if (safe_is_logged_in()) {
		s_ctx.t -= dt;
		if (s_ctx.t < 0.0f) {
			s_ctx.t = AUTO_UPDATE_INTERVALS_MS;
			client_state();
		}
	}
}

void session_shutdown() {
	mtx_destroy(&s_ctx.lock);
}

const session_t* session_current() {
	return safe_is_active() ? &s_ctx.current : NULL;
}

void session_start(const char* username, const char* password) {
	assert(!safe_is_active());
	client_login(username, password);
}

void session_end() {
	assert(safe_is_active());
	client_logout();
}

void session_reveal(uint32_t x, uint32_t y) {
	assert(safe_is_active());
	client_reveal(x, y);
}
