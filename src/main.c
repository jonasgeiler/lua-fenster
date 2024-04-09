#include "main.h"
#include "vendor/fenster.h"
#include "vendor/compat-5.3.h"

typedef struct lua_fenster {
	struct fenster *p_fenster;
	int64_t last_frame_time;
	unsigned long keys_size;
} lua_fenster;

static int lua_fenster_open(lua_State *L) {
	const char *title = luaL_checklstring(L, 1, NULL);
	const int width = luaL_checknumber(L, 2);
	const int height = luaL_checknumber(L, 3);

	uint32_t *buffer = (uint32_t *) calloc(width * height, sizeof(uint32_t));
	if (buffer == NULL) {
		return luaL_error(L, "failed to allocate memory for frame buffer of size %d (%d)", width * height, errno);
	}

	struct fenster temp_fenster = {
		.title = title,
		.width = width,
		.height = height,
		.buf = buffer
	};

	struct fenster *p_fenster = (struct fenster *) malloc(sizeof(struct fenster));
	if (p_fenster == NULL) {
		free(buffer);
		buffer = NULL;
		return luaL_error(L, "failed to allocate memory for window (%d)", errno);
	}

	memcpy(p_fenster, &temp_fenster, sizeof(struct fenster));

	const int result = fenster_open(p_fenster);
	if (result != 0) {
		free(p_fenster);
		p_fenster = NULL;
		free(buffer);
		buffer = NULL;
		return luaL_error(L, "failed to open window (%d)", result);
	}

	lua_fenster *p_lua_fenster = lua_newuserdata(L, sizeof(lua_fenster));
	p_lua_fenster->p_fenster = p_fenster;
	p_lua_fenster->last_frame_time = fenster_time();
	p_lua_fenster->keys_size = sizeof(p_fenster->keys) / sizeof(p_fenster->keys[0]);

	luaL_setmetatable(L, "lua_fenster");
	return 1;
}

static int lua_fenster_close(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	if (p_lua_fenster->p_fenster == NULL) {
		// already closed
		return 0;
	}
	fenster_close(p_lua_fenster->p_fenster);
	free(p_lua_fenster->p_fenster->buf);
	p_lua_fenster->p_fenster->buf = NULL;
	free(p_lua_fenster->p_fenster);
	p_lua_fenster->p_fenster = NULL;

	return 0;
}

static int lua_fenster_loop(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
	if (lua_gettop(L) >= 2) {
		const double max_fps = luaL_checknumber(L, 2);

		const int64_t delta_frame_time = fenster_time() - p_lua_fenster->last_frame_time;
		const double max_frame_time = 1000.0 / max_fps;
		if (delta_frame_time < max_frame_time) {
			// sleep for the remaining frame time
			fenster_sleep(max_frame_time - delta_frame_time);
		}

		p_lua_fenster->last_frame_time = fenster_time();
	}

	if (fenster_loop(p_lua_fenster->p_fenster) == 0) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int lua_fenster_set(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
	const int x = luaL_checknumber(L, 2);
	const int y = luaL_checknumber(L, 3);
	const uint32_t color = luaL_checknumber(L, 4);

	if (x < 0 || x >= p_lua_fenster->p_fenster->width || y < 0 || y >= p_lua_fenster->p_fenster->height) {
		return luaL_error(L, "pixel out of bounds");
	}
	fenster_pixel(p_lua_fenster->p_fenster, x, y) = color;

	return 0;
}

static int lua_fenster_get(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
	const int x = luaL_checknumber(L, 2);
	const int y = luaL_checknumber(L, 3);

	lua_pushnumber(L, fenster_pixel(p_lua_fenster->p_fenster, x, y));
	return 1;
}

static int lua_fenster_title(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	lua_pushstring(L, p_lua_fenster->p_fenster->title);
	return 1;
}

static int lua_fenster_width(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	lua_pushnumber(L, p_lua_fenster->p_fenster->width);
	return 1;
}

static int lua_fenster_height(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	lua_pushnumber(L, p_lua_fenster->p_fenster->height);
	return 1;
}

static int lua_fenster_key(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
	const int key = luaL_checknumber(L, 2);

	if (p_lua_fenster->p_fenster->keys[key]) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int lua_fenster_keys(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	lua_newtable(L);
	for (int i = 0; i < p_lua_fenster->keys_size; i++) {
		lua_pushnumber(L, i);
		lua_pushboolean(L, p_lua_fenster->p_fenster->keys[i]);
		lua_settable(L, -3);
	}
	return 1;
}

static int lua_fenster_mods(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	lua_pushboolean(L, p_lua_fenster->p_fenster->mod & 1); // ctrl
	lua_pushboolean(L, (p_lua_fenster->p_fenster->mod >> 1) & 1); // shift
	lua_pushboolean(L, (p_lua_fenster->p_fenster->mod >> 2) & 1); // alt
	lua_pushboolean(L, (p_lua_fenster->p_fenster->mod >> 3) & 1); // super/meta
	return 4;
}

static int lua_fenster_mouse(lua_State *L) {
	lua_fenster *p_lua_fenster = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

	lua_pushnumber(L, p_lua_fenster->p_fenster->x); // mouse x
	lua_pushnumber(L, p_lua_fenster->p_fenster->y); // mouse y
	lua_pushboolean(L, p_lua_fenster->p_fenster->mouse); // mouse button
	return 3;
}

static int lua_fenster_rgb(lua_State *L) {
	if (lua_gettop(L) < 3) {
		const uint32_t color = luaL_checknumber(L, 1);

		lua_pushnumber(L, (color >> 16) & 0xFF);
		lua_pushnumber(L, (color >> 8) & 0xFF);
		lua_pushnumber(L, color & 0xFF);
		return 3;
	} else {
		const uint8_t r = luaL_checknumber(L, 1);
		const uint8_t g = luaL_checknumber(L, 2);
		const uint8_t b = luaL_checknumber(L, 3);

		lua_pushnumber(L, (r << 16) | (g << 8) | b);
		return 1;
	}
}

static int lua_fenster_sleep(lua_State *L) {
	const int64_t ms = luaL_checknumber(L, 1);

	fenster_sleep(ms);

	return 0;
}

static int lua_fenster_time(lua_State *L) {
	lua_pushnumber(L, fenster_time());
	return 1;
}

static const struct luaL_Reg fenster_funcs[] = {
	{"open", lua_fenster_open},
	{"close", lua_fenster_close},
	{"loop", lua_fenster_loop},
	{"set", lua_fenster_set},
	{"get", lua_fenster_get},
	{"title", lua_fenster_title},
	{"width", lua_fenster_width},
	{"height", lua_fenster_height},
	{"key", lua_fenster_key},
	{"keys", lua_fenster_keys},
	{"mods", lua_fenster_mods},
	{"mouse", lua_fenster_mouse},
	{"sleep", lua_fenster_sleep},
	{"time", lua_fenster_time},
	{"rgb", lua_fenster_rgb},
	{NULL, NULL}  /* sentinel */
};

static const struct luaL_Reg fenster_methods[] = {
	{"close", lua_fenster_close},
	{"loop", lua_fenster_loop},
	{"set", lua_fenster_set},
	{"get", lua_fenster_get},
	{"title", lua_fenster_title},
	{"width", lua_fenster_width},
	{"height", lua_fenster_height},
	{"key", lua_fenster_key},
	{"keys", lua_fenster_keys},
	{"mods", lua_fenster_mods},
	{"mouse", lua_fenster_mouse},
	{NULL, NULL}  /* sentinel */
};

FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
	if (luaL_newmetatable(L, "lua_fenster")) {
		luaL_setfuncs(L, fenster_methods, 0);

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, -2);
		lua_settable(L, -3);

		lua_pushliteral(L, "__gc");
		lua_pushcfunction(L, lua_fenster_close);
		lua_settable(L, -3);

		lua_pushliteral(L, "__close");
		lua_pushcfunction(L, lua_fenster_close);
		lua_settable(L, -3);
	}
	lua_pop(L, 1);

	luaL_newlib(L, fenster_funcs);
	return 1;
}
