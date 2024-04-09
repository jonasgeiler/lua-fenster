#include "main.h"
#include "vendor/fenster.h"
#include "vendor/compat-5.3.h"

const uint32_t MAX_COLOR = 0xffffff;

typedef struct lua_fenster {
  struct fenster *p_fenster;
  int original_width;
  int original_height;
  uint8_t scale;
  double delta;
  int64_t target_frame_time;
  int64_t start_frame_time;
  unsigned long keys_size;
} lua_fenster;

static int lua_fenster_open(lua_State *L) {
  const char *title = luaL_checklstring(L, 1, NULL);
  const int width = (int) luaL_checknumber(L, 2);
  const int height = (int) luaL_checknumber(L, 3);
  const int scale = (int) luaL_optnumber(L, 4, 1.0);
  if ((scale & (scale - 1)) != 0) {
    return luaL_error(
        L, "invalid scale value: %d (must be a power of 2)",
        scale
    );
  }
  const double target_fps = (double) luaL_optnumber(L, 5, 60.0);

  uint32_t *buffer = (uint32_t *) calloc(
      width * scale * height * scale,
      sizeof(uint32_t)
  );
  if (buffer == NULL) {
    return luaL_error(
        L, "failed to allocate memory of size %d for frame buffer (%d)",
        width * scale * height * scale, errno
    );
  }

  struct fenster temp_fenster = {
      .title = title,
      .width = width * scale,
      .height = height * scale,
      .buf = buffer,
  };

  struct fenster *p_fenster = (struct fenster *) malloc(
      sizeof(struct fenster)
  );
  if (p_fenster == NULL) {
    const int error = errno;
    free(buffer);
    buffer = NULL;
    return luaL_error(
        L, "failed to allocate memory of size %d for window (%d)",
        sizeof(struct fenster), error
    );
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

  lua_fenster *p_lf = lua_newuserdata(L, sizeof(lua_fenster));
  p_lf->p_fenster = p_fenster;
  p_lf->original_width = width;
  p_lf->original_height = height;
  p_lf->scale = scale;
  p_lf->delta = 0.0;
  p_lf->target_frame_time = lround(1000.0 / target_fps);
  p_lf->start_frame_time = 0;
  p_lf->keys_size = sizeof(p_fenster->keys) / sizeof(p_fenster->keys[0]);

  luaL_setmetatable(L, "lua_fenster");
  return 1;
}

static int lua_fenster_close(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  if (p_lf->p_fenster == NULL) {
    // already closed
    return 0;
  }
  fenster_close(p_lf->p_fenster);
  free(p_lf->p_fenster->buf);
  p_lf->p_fenster->buf = NULL;
  free(p_lf->p_fenster);
  p_lf->p_fenster = NULL;

  return 0;
}

static int lua_fenster_loop(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  // handle frame timing
  int64_t now = fenster_time();
  if (p_lf->start_frame_time == 0) {
    // initialize start frame time
    p_lf->start_frame_time = now;
  }
  const int64_t last_frame_time = now - p_lf->start_frame_time;
  if (p_lf->target_frame_time > last_frame_time) {
    // sleep for the remaining frame time to reach target frame time
    fenster_sleep(p_lf->target_frame_time - last_frame_time);
  }
  now = fenster_time();
  p_lf->delta = (double) (now - p_lf->start_frame_time) / 1000.0;
  p_lf->start_frame_time = now;

  if (fenster_loop(p_lf->p_fenster) == 0) {
    lua_pushboolean(L, 1);
  } else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

static int lua_fenster_set(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
  const int x = (int) luaL_checknumber(L, 2);
  const int y = (int) luaL_checknumber(L, 3);
  const uint32_t color = (uint32_t) luaL_checknumber(L, 4);
  if (color > MAX_COLOR) {
    return luaL_error(
        L, "invalid color value: %d (must be 0-%d)",
        color, MAX_COLOR
    );
  }

  if (x < 0 || x >= p_lf->original_width ||
      y < 0 || y >= p_lf->original_height) {
    return luaL_error(
        L, "pixel out of bounds: %d,%d (must be 0-%d,0-%d)",
        x, y, p_lf->original_width - 1, p_lf->original_height - 1
    );
  }
  int sy = y * p_lf->scale;
  int sy_end = (y + 1) * p_lf->scale;
  int sx;
  int sx_start = x * p_lf->scale;
  int sx_end = (x + 1) * p_lf->scale;
  for (; sy < sy_end; sy++) {
    for (sx = sx_start; sx < sx_end; sx++) {
      fenster_pixel(p_lf->p_fenster, sx, sy) = color;
    }
  }

  return 0;
}

static int lua_fenster_get(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
  const int x = (int) luaL_checknumber(L, 2);
  const int y = (int) luaL_checknumber(L, 3);

  if (x < 0 || x >= p_lf->original_width ||
      y < 0 || y >= p_lf->original_height) {
    return luaL_error(
        L, "pixel out of bounds: %d,%d (must be 0-%d,0-%d)",
        x, y, p_lf->original_width - 1, p_lf->original_height - 1
    );
  }

  lua_pushnumber(
      L, fenster_pixel(p_lf->p_fenster, x * p_lf->scale, y * p_lf->scale)
  );
  return 1;
}

static int lua_fenster_title(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_pushstring(L, p_lf->p_fenster->title);
  return 1;
}

static int lua_fenster_width(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_pushnumber(L, p_lf->original_width);
  return 1;
}

static int lua_fenster_height(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_pushnumber(L, p_lf->original_height);
  return 1;
}

static int lua_fenster_delta(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_pushnumber(L, p_lf->delta);
  return 1;
}

static int lua_fenster_key(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
  const int key = (int) luaL_checknumber(L, 2);

  if (p_lf->p_fenster->keys[key]) {
    lua_pushboolean(L, 1);
  } else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

static int lua_fenster_keys(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_newtable(L);
  for (int i = 0; i < p_lf->keys_size; i++) {
    lua_pushnumber(L, i);
    lua_pushboolean(L, p_lf->p_fenster->keys[i]);
    lua_settable(L, -3);
  }
  return 1;
}

static int lua_fenster_mods(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_pushboolean(L, p_lf->p_fenster->mod & 1); // ctrl
  lua_pushboolean(L, (p_lf->p_fenster->mod >> 1) & 1); // shift
  lua_pushboolean(L, (p_lf->p_fenster->mod >> 2) & 1); // alt
  lua_pushboolean(L, (p_lf->p_fenster->mod >> 3) & 1); // super/meta
  return 4;
}

static int lua_fenster_mouse(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  lua_pushnumber( // mouse x
      L, (double) round((double) p_lf->p_fenster->x / p_lf->scale)
  );
  lua_pushnumber( // mouse y
      L, (double) round((double) p_lf->p_fenster->y / p_lf->scale)
  );
  lua_pushboolean(L, p_lf->p_fenster->mouse); // mouse button
  return 3;
}

static int lua_fenster_rgb(lua_State *L) {
  if (lua_gettop(L) < 3) {
    const uint32_t color = (uint32_t) luaL_checknumber(L, 1);
    if (color > MAX_COLOR) {
      return luaL_error(
          L, "invalid color value: %d (must be 0-%d)",
          color, MAX_COLOR
      );
    }

    lua_pushnumber(L, (color >> 16) & 0xFF);
    lua_pushnumber(L, (color >> 8) & 0xFF);
    lua_pushnumber(L, color & 0xFF);
    return 3;
  } else {
    const uint8_t r = (uint8_t) luaL_checknumber(L, 1);
    const uint8_t g = (uint8_t) luaL_checknumber(L, 2);
    const uint8_t b = (uint8_t) luaL_checknumber(L, 3);

    lua_pushnumber(L, (r << 16) | (g << 8) | b);
    return 1;
  }
}

static int lua_fenster_sleep(lua_State *L) {
  const int64_t ms = (int64_t) luaL_checknumber(L, 1);

  fenster_sleep(ms);

  return 0;
}

static int lua_fenster_time(lua_State *L) {
  lua_pushnumber(L, (double) fenster_time());
  return 1;
}

static const struct luaL_Reg lua_fenster_funcs[] = {
    {"open", lua_fenster_open},
    {"close", lua_fenster_close},
    {"loop", lua_fenster_loop},
    {"set", lua_fenster_set},
    {"get", lua_fenster_get},
    {"title", lua_fenster_title},
    {"width", lua_fenster_width},
    {"height", lua_fenster_height},
    {"delta", lua_fenster_delta},
    {"key", lua_fenster_key},
    {"keys", lua_fenster_keys},
    {"mods", lua_fenster_mods},
    {"mouse", lua_fenster_mouse},
    {"sleep", lua_fenster_sleep},
    {"time", lua_fenster_time},
    {"rgb", lua_fenster_rgb},
    {NULL, NULL}  /* sentinel */
};

static const struct luaL_Reg lua_fenster_methods[] = {
    {"close", lua_fenster_close},
    {"loop", lua_fenster_loop},
    {"set", lua_fenster_set},
    {"get", lua_fenster_get},
    {"title", lua_fenster_title},
    {"width", lua_fenster_width},
    {"height", lua_fenster_height},
    {"delta", lua_fenster_delta},
    {"key", lua_fenster_key},
    {"keys", lua_fenster_keys},
    {"mods", lua_fenster_mods},
    {"mouse", lua_fenster_mouse},
    {NULL, NULL}  /* sentinel */
};

FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
  if (luaL_newmetatable(L, "lua_fenster")) {
    luaL_setfuncs(L, lua_fenster_methods, 0);

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

  luaL_newlib(L, lua_fenster_funcs);
  return 1;
}
