#include "main.h"
#include "fenster.h"
#include "compat-5.3.h"

typedef struct lua_fenster {
	struct fenster *f;
	uint32_t *buf;
	int64_t last_frame_time;
} lua_fenster;

static int lua_fenster_open(lua_State *L) {
	const char *title = luaL_checklstring(L, 1, NULL);
	const int width = luaL_checknumber(L, 2);
	const int height = luaL_checknumber(L, 3);

	uint32_t *buf = (uint32_t *) calloc(width * height, sizeof(uint32_t));
	if (buf == NULL) return luaL_error(L, "failed to allocate memory for frame buffer of size %d", width * height);

	struct fenster temp_f = {
		.title = title,
		.width = width,
		.height = height,
		.buf = buf
	};

	struct fenster *f = (struct fenster *) malloc(sizeof(struct fenster));
	if (f == NULL) {
		free(f);
		free(buf);
		return luaL_error(L, "failed to allocate memory for window");
	}

	memcpy(f, &temp_f, sizeof(struct fenster));

	const int res = fenster_open(f);
	if (res != 0) {
		free(f);
		free(buf);
		return luaL_error(L, "failed to open window (%d)", res);
	}

	lua_fenster *lf = lua_newuserdata(L, sizeof(lua_fenster));
	lf->f = f;
	lf->buf = buf;
	lf->last_frame_time = fenster_time();

	luaL_setmetatable(L, "fenster_window");

	return 1;
}

static int lua_fenster_close(lua_State *L) {
	lua_fenster *lf = (lua_fenster *) luaL_checkudata(L, 1, "fenster_window");

	fenster_close(lf->f);
	free(lf->f);
	free(lf->buf);
	return 0;
}

static int lua_fenster_loop(lua_State *L) {
	lua_fenster *lf = (lua_fenster *) luaL_checkudata(L, 1, "fenster_window");

	if (lua_gettop(L) >= 2) {
		const double max_fps = luaL_checknumber(L, 2);
		const double max_frame_time = 1000.0 / max_fps;

		const int64_t curr_frame_time = fenster_time();
		const int64_t frame_time = curr_frame_time - lf->last_frame_time;

		if (frame_time < max_frame_time) {
			fenster_sleep(max_frame_time - frame_time);
		}

		lf->last_frame_time = curr_frame_time;
	}

	if (fenster_loop(lf->f) == 0) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int lua_fenster_set(lua_State *L) {
	lua_fenster *lf = (lua_fenster *) luaL_checkudata(L, 1, "fenster_window");
	const int x = luaL_checknumber(L, 2);
	const int y = luaL_checknumber(L, 3);
	const uint32_t color = luaL_checknumber(L, 4);

	if (x < 0 || x >= lf->f->width || y < 0 || y >= lf->f->height) {
		return luaL_error(L, "pixel out of bounds");
	}

	fenster_pixel(lf->f, x, y) = color;
	return 0;
}

static int lua_fenster_get(lua_State *L) {
	lua_fenster *lf = (lua_fenster *) luaL_checkudata(L, 1, "fenster_window");
	const int x = luaL_checknumber(L, 2);
	const int y = luaL_checknumber(L, 3);

	lua_pushnumber(L, fenster_pixel(lf->f, x, y));
	return 1;
}

static int lua_fenster_key(lua_State *L) {
	lua_fenster *lf = (lua_fenster *) luaL_checkudata(L, 1, "fenster_window");
	const int key = luaL_checknumber(L, 2);

	if (lf->f->keys[key]) {
		lua_pushboolean(L, 1);
	} else {
		lua_pushboolean(L, 0);
	}
	return 1;
}

static int lua_fenster_time(lua_State *L) {
	lua_pushnumber(L, fenster_time());
	return 1;
}

static int lua_fenster_sleep(lua_State *L) {
	const int64_t ms = luaL_checknumber(L, 1);

	fenster_sleep(ms);
	return 0;
}

static const struct luaL_Reg fenster_funcs[] = {
	{"open", lua_fenster_open},
	{"close", lua_fenster_close},
	{"loop", lua_fenster_loop},
	{"set", lua_fenster_set},
	{"get", lua_fenster_get},
	{"key", lua_fenster_key},
	{"time", lua_fenster_time},
	{"sleep", lua_fenster_sleep},
	{NULL, NULL}  /* sentinel */
};

static const struct luaL_Reg fenster_methods[] = {
	{"close", lua_fenster_close},
	{"loop", lua_fenster_loop},
	{"set", lua_fenster_set}
	{"get", lua_fenster_get},
	{"key", lua_fenster_key},
	{NULL, NULL}  /* sentinel */
};

FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
	if (luaL_newmetatable(L, "fenster_window")) {
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
