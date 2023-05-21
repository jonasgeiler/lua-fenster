#include "main.h"
#include "fenster.h"
#include "lua-compat-5.3.h"

static int open(lua_State* L) {
	const char* title = luaL_checklstring(L, 1, NULL);
	double width = luaL_checknumber(L, 2);
	double height = luaL_checknumber(L, 3);

	uint32_t* buf = (uint32_t*) calloc(width * height, sizeof(uint32_t));

	struct fenster f = {
		.title = title,
		.width = width,
		.height = height,
		.buf = buf
	};

	fenster_open(&f);

	while (fenster_loop(&f) == 0) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				fenster_pixel(&f, i, j) = rand();
			}
		}
	}

	fenster_close(&f);

	return 0;
}

static const struct luaL_Reg mylib[] = {
	{"open", open},
	{NULL,   NULL}  /* sentinel */
};

FENSTER_EXPORT int luaopen_fenster(lua_State* L) {
	luaL_newlib(L, mylib);
	return 1;
}
