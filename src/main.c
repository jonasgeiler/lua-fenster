#include "main.h"
#include "fenster.h"
#include "lua-compat-5.3.h"

typedef struct lua_fenster {
	struct fenster *f;
} lua_fenster;

static int open(lua_State *L) {
	const char *title = luaL_checklstring(L, 1, NULL);
	const int width = luaL_checknumber(L, 2);
	const int height = luaL_checknumber(L, 3);

	uint32_t *buf = (uint32_t *) calloc(width * height, sizeof(uint32_t));
	if (buf == NULL) return luaL_error(L, "failed to allocate frame buffer of size %d", width * height);

	struct fenster f = {
		.title = title,
		.width = width,
		.height = height,
		.buf = buf
	};

	int res = fenster_open(&f);
	if (res != 0) return luaL_error(L, "failed to open window (%d)", res);

	lua_fenster *lf = lua_newuserdata(L, sizeof(lua_fenster));
	lf->f = &f;

	return 1;
}

static const struct luaL_Reg fenster_funcs[] = {
	{"open", open},
	{NULL, NULL}  /* sentinel */
};

FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
	luaL_newlib(L, fenster_funcs);
	return 1;
}
