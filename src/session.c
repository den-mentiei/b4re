#include "session.h"

#include <assert.h>
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

	char username[128];
	char avatar[128];
} s_ctx;

static uint8_t randi(uint8_t max) {
	return floor(((float)rand() / RAND_MAX) * max);
}

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
					log_error("[session] Got unexpected 'login' message.");
				}
				break;

			case MESSAGE_TYPE_LOGOUT:
				if (s_ctx.status == STATUS_AWAITING_LOGOUT) s_ctx.status = STATUS_NA;
				break;

			case MESSAGE_TYPE_STATE: {
				if (s_ctx.status == STATUS_AWAITING_STATE) s_ctx.status = STATUS_ACTIVE;
				// TODO:
				api_state_t* s = (api_state_t*)msg->data;
				log_info("[session] t = %ul | %s at %d,%d", s->timestamp, s->player.username, s->player.x, s->player.y);
				break;
			}

			case MESSAGE_TYPE_MAP: {
				// TODO:
				api_map_t* m = (api_map_t*)msg->data;
				log_info("[session] Got map for %u,%u", m->x, m->y);
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
