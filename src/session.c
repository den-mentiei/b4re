#include "session.h"

#include <assert.h>
#include <string.h> // memset, memcpy, strcpy
#include <stdio.h>  // snprintf

#include <math.h>   // floor
#include <stdlib.h> // random

#include <tinycthread.h>

#include "allocator.h"
#include "log.h"
#include "http.h"

#include "api.h"

static struct {
	allocator_t* alloc;

	session_t current;
	session_t synced;
	bool      is_logged_in;
	bool      is_active;

	char username[128];
	char avatar[128];

	mtx_t lock;
} s_ctx;

/// API
static void login(const char* username, const char* password);
static void logout();
static void fetch_state();
static void reveal(uint32_t x, uint32_t y);
/// API END

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
	mtx_lock(&s_ctx.lock);
	memcpy(&s_ctx.current, &s_ctx.synced, sizeof(session_t));
	mtx_unlock(&s_ctx.lock);

	static bool was_state_fetched = false;
	if (safe_is_logged_in()) {
		if (!was_state_fetched) {
			was_state_fetched = true;
			fetch_state();
		}
	} else {
		was_state_fetched = false;
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
	login(username, password);
}

void session_end() {
	assert(safe_is_active());

	logout();

	// TODO: set it only if logout was successfull.
	mtx_lock(&s_ctx.lock);
	s_ctx.is_active = false;
	memset(&s_ctx.synced,  0, sizeof(session_t));
	mtx_unlock(&s_ctx.lock);

	memset(&s_ctx.current, 0, sizeof(session_t));
}

void session_reveal(uint32_t x, uint32_t y) {
	assert(safe_is_active());
	reveal(x, y);
}

/// API stuff

static char* LOGIN_TAG = "login";
static char* STATE_TAG = "state";
static char* MAP_TAG   = "map";

static void http_handler(const uint8_t* data, size_t size, void* payload) {
	if (payload == LOGIN_TAG) {
		mtx_lock(&s_ctx.lock);
			s_ctx.is_logged_in = true;
		mtx_unlock(&s_ctx.lock);
	} else if (payload == STATE_TAG) {
		api_state_t state;
		bool parsed = api_parse_state(s_ctx.alloc, (const char*)data, &state);
		assert(parsed);

		mtx_lock(&s_ctx.lock);
			// TODO: Copy field by field or re-use parsed state.
			memcpy(&s_ctx.synced.player.mind,   &state.player.mind,   sizeof(resource_t));
			memcpy(&s_ctx.synced.player.matter, &state.player.matter, sizeof(resource_t));
			s_ctx.synced.player.x   = state.player.x;
			s_ctx.synced.player.y   = state.player.y;
			s_ctx.synced.player.exp = state.player.exp;
			s_ctx.is_active         = true;
		mtx_unlock(&s_ctx.lock);
	} else if (payload == MAP_TAG) {
		api_map_t map;
		bool parsed = api_parse_map(s_ctx.alloc, (const char*)data, &map);
		assert(parsed);
	} else {
		log_info((const char*)data);
		log_info("");
	}
}

static void login(const char* username, const char* password) {
	assert(username);
	assert(password);

	log_info("[session] Logging in with username=%s", username);

	http_form_part_t form[] = {
		{ "username", username },
		{ "password", password }
	};

	http_post_form("http://ancientlighthouse.com:8080/api/login", form, sizeof(form) / sizeof(form[0]), http_handler, LOGIN_TAG);
}

static void logout() {
	log_info("[session] Logging out");
	http_post("http://ancientlighthouse.com:8080/api/logout", http_handler, NULL);
}

static void fetch_state() {
	log_info("[session] Fetching state");
	http_get("http://ancientlighthouse.com:8080/api/state", http_handler, STATE_TAG);
}

static void reveal(uint32_t x, uint32_t y) {
	log_info("[session] Revealing %u, %u", x, y);
	char buf[128];
	snprintf(buf, sizeof(buf), "http://ancientlighthouse.com:8080/api/reveal/%u/%u", x, y);
	http_get(buf, http_handler, NULL);
}

// TODO: Remove it.

void session_foo() {
	log_info("[session] Fetching map");
	http_get("http://ancientlighthouse.com:8080/api/map/homeland_3/0/0/12", http_handler, MAP_TAG);
}

