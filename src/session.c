#include "session.h"

#include <assert.h>
#include <string.h> // strncpy
#include <math.h>   // floor
#include <stdlib.h> // rand

#include "log.h"
#include "client.h"
#include "api.h"

#define AUTO_UPDATE_INTERVALS_MS 5000.0f

typedef enum {
	STATUS_NA = 0,
	STATUS_AWAITING_LOGIN,
	STATUS_AWAITING_STATE,
	STATUS_ACTIVE,
	STATUS_AWAITING_LOGOUT
} status_t;

static struct {
	float t;

	session_t current;
	uint8_t   status;
} s_ctx;

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static inline uint8_t randi(uint8_t max) {
	return floor(((float)rand() / RAND_MAX) * max);
}

static void handle_state_resource(const api_state_resource_t* r, resource_t* out) {
	assert(r);
	assert(out);

	out->last_update     = r->last_update;
	out->booster_time    = r->booster_time;
	out->value           = r->value;
	out->max             = r->max;
	out->regen_rate      = r->regen_rate;
	out->filled_segments = r->filled_segments;
	out->segment_time    = r->segment_time;
}

static void handle_state(const api_state_t* s) {
	assert(s);

	const size_t n = MIN(MAX_API_STRING_LENGTH, MAX_SESSION_STRING_LENGTH);
	strncpy(s_ctx.current.player.username, s->player.username, n - 1);
	s_ctx.current.player.username[n - 1] = 0;

	s_ctx.current.player.x      = s->player.x;
	s_ctx.current.player.y      = s->player.y;
	s_ctx.current.player.level  = s->player.level;
	s_ctx.current.player.exp    = s->player.exp;
	s_ctx.current.player.avatar = s->player.avatar;
	
	handle_state_resource(&s->player.mind,   &s_ctx.current.player.mind);
	handle_state_resource(&s->player.matter, &s_ctx.current.player.matter);
}

// PUBLIC API
// ==========

void session_init() {
	// TODO: Remove it.
	for (size_t y = 0; y < WORLD_PLANE_SIZE; ++y) {
		for (size_t x = 0; x < WORLD_PLANE_SIZE; ++x) {
			s_ctx.current.world.locations[x][y].has_data  = true;
			s_ctx.current.world.locations[x][y].is_hidden = false;
			s_ctx.current.world.locations[x][y].terrain   = 1 + randi(TERRAIN_WATER_DEEP);
			/* if ((float)rand() / RAND_MAX > 0.5f) { */
			/* 	s_ctx.current.world.locations[x][y].has_data  = true; */
			/* 	s_ctx.current.world.locations[x][y].is_hidden = false; */
			/* 	s_ctx.current.world.locations[x][y].terrain   = 1 + randi(TERRAIN_WATER_DEEP); */
			/* } else { */
			/* 	s_ctx.current.world.locations[x][y].has_data  = true; */
			/* 	s_ctx.current.world.locations[x][y].is_hidden = true; */
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
				if (s_ctx.status == STATUS_AWAITING_LOGIN) {
					s_ctx.status = STATUS_AWAITING_STATE;
					client_state();
				} else {
					log_error("[session] Got unexpected 'login' message");
				}
				break;

			case MESSAGE_TYPE_LOGOUT:
				if (s_ctx.status == STATUS_AWAITING_LOGOUT) {
					s_ctx.status = STATUS_NA;
				} else {
					log_error("[session] Got unexpected 'logout' message");
				}
				break;

			case MESSAGE_TYPE_STATE: {
				if (s_ctx.status == STATUS_AWAITING_STATE) s_ctx.status = STATUS_ACTIVE;
				handle_state((api_state_t*)msg->data);
				break;
			}

			case MESSAGE_TYPE_MAP: {
				if (s_ctx.status == STATUS_ACTIVE) {
					// TODO:
					api_map_t* m = (api_map_t*)msg->data;
					log_info("[session] Got map for %u,%u", m->x, m->y);
				} else {
					log_error("[session] Got unexpected 'map' message");
				}
				break;
			}

			/* case MESSAGE_TYPE_REVEAL: */
			/* 	break; */

			default: log_fatal("[session] Got an unknown message!");
		}

		client_messages_consume();
	}

	if (s_ctx.status == STATUS_ACTIVE) {
		s_ctx.t -= dt;
		if (s_ctx.t < 0.0f) {
			s_ctx.t = AUTO_UPDATE_INTERVALS_MS;
			client_state();
		}
	}
}

void session_shutdown() {}

const session_t* session_current() {
	return s_ctx.status == STATUS_ACTIVE ? &s_ctx.current : NULL;
}

void session_start(const char* username, const char* password) {
	assert(s_ctx.status == STATUS_NA);
	s_ctx.status = STATUS_AWAITING_LOGIN;
	client_login(username, password);
}

void session_end() {
	assert(s_ctx.status == STATUS_ACTIVE);
	client_logout();
}

void session_reveal(uint32_t x, uint32_t y) {
	assert(s_ctx.status == STATUS_ACTIVE);
	client_reveal(x, y);
}
