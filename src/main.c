#include "main.h"
#include "vendor/fenster.h"
#include "vendor/compat-5.3.h"

/** Maximum color value */
static const uint32_t MAX_COLOR = 0xffffff;

/** Maximum value of a color component (r, g or b) */
static const uint32_t MAX_COLOR_COMPONENT = 0xff;

/** Length of the fenster->keys array */
static const int KEYS_LENGTH = sizeof(((struct fenster *) 0)->keys)
    / sizeof(((struct fenster *) 0)->keys[0]);

typedef struct lua_fenster {
  struct fenster *p_fenster;
  lua_Integer original_width;
  lua_Integer original_height;
  lua_Integer scale;
  lua_Number target_fps;
  double delta;
  int64_t target_frame_time;
  int64_t start_frame_time;
} lua_fenster;

/*
static void _dumpstack(lua_State *L) {
  int top = lua_gettop(L);
  for (int i = 1; i <= top; i++) {
    printf("%d\t%s\t", i, luaL_typename(L, i));
    switch (lua_type(L, i)) {
      case LUA_TNUMBER:printf("%g\n", lua_tonumber(L, i));
        break;
      case LUA_TSTRING:printf("%s\n", lua_tostring(L, i));
        break;
      case LUA_TBOOLEAN:
        printf("%s\n",
               (lua_toboolean(L, i) ? "true" : "false"));
        break;
      case LUA_TNIL:printf("%s\n", "nil");
        break;
      default:printf("%p\n", lua_topointer(L, i));
        break;
    }
  }
}
 */

static int lua_fenster_open(lua_State *L) {
  const int width = (int) luaL_checkinteger(L, 1);
  const int height = (int) luaL_checkinteger(L, 2);
  const char *title = luaL_optlstring(L, 3, "fenster", NULL);
  const int scale = (int) luaL_optinteger(L, 4, 1);
  if ((scale & (scale - 1)) != 0) {
    return luaL_error(
        L, "invalid scale value: %d (must be a power of 2)",
        scale
    );
  }
  const lua_Number target_fps = luaL_optnumber(L, 5, 60.0);

  uint32_t *buffer = (uint32_t *) calloc(
      width * scale * height * scale,
      sizeof(uint32_t)
  );
  if (buffer == NULL) {
    const int error = errno;
    return luaL_error(
        L, "failed to allocate memory of size %d for frame buffer (%d)",
        width * scale * height * scale, error
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

  // open window and check success
  const int result = fenster_open(p_fenster);
  if (result != 0) {
    free(p_fenster);
    p_fenster = NULL;
    free(buffer);
    buffer = NULL;
    return luaL_error(L, "failed to open window (%d)", result);
  }

  // create lua_fenster userdata and initialize it
  lua_fenster *p_lf = lua_newuserdata(L, sizeof(lua_fenster));
  p_lf->p_fenster = p_fenster;
  p_lf->original_width = width;
  p_lf->original_height = height;
  p_lf->scale = scale;
  p_lf->target_fps = target_fps;
  p_lf->delta = 0.0;
  p_lf->target_frame_time = lround(1000.0 / target_fps);
  p_lf->start_frame_time = 0;
  luaL_setmetatable(L, "lua_fenster");

  // initialize keys table and put it in the registry
  lua_pushvalue(L, -1);
  lua_createtable(L, KEYS_LENGTH, 0);
  lua_settable(L, LUA_REGISTRYINDEX);

  return 1;
}

/**
 * Sleep for a given number of milliseconds.
 * Lua usage: `fenster.sleep(ms: integer)`
 * @lua_param ms integer Number of milliseconds to sleep
 * @param L Lua state
 * @return Number of return values on the stack
 */
static int lua_fenster_sleep(lua_State *L) {
  const lua_Integer ms = luaL_checkinteger(L, 1);

  fenster_sleep(ms);

  return 0;
}

/**
 * Get the current time in milliseconds.
 * Lua usage: `fenster.time(): integer`
 * @lua_return integer The current time in milliseconds
 * @param L Lua state
 * @return Number of return values on the stack
 */
static int lua_fenster_time(lua_State *L) {
  lua_pushinteger(L, fenster_time());
  return 1;
}

static int lua_fenster_rgb(lua_State *L) {
  if (lua_gettop(L) < 3) {
    // TODO: pull out into a helper function
    const lua_Integer color = luaL_checkinteger(L, 1);
    if (color < 0 || color > MAX_COLOR) {
      return luaL_error(
          L, "invalid color value: %d (must be 0-%d)",
          color, MAX_COLOR
      );
    }

    lua_pushinteger(L, (color >> 16) & MAX_COLOR_COMPONENT);
    lua_pushinteger(L, (color >> 8) & MAX_COLOR_COMPONENT);
    lua_pushinteger(L, color & MAX_COLOR_COMPONENT);
    return 3;
  } else {
    const lua_Integer red = luaL_checkinteger(L, 1);
    if (red < 0 || red > MAX_COLOR_COMPONENT) {
      return luaL_error(
          L, "invalid red value: %d (must be 0-%d)",
          red, MAX_COLOR_COMPONENT
      );
    }
    const lua_Integer green = luaL_checkinteger(L, 2);
    if (green < 0 || green > MAX_COLOR_COMPONENT) {
      return luaL_error(
          L, "invalid green value: %d (must be 0-%d)",
          green, MAX_COLOR_COMPONENT
      );
    }
    const lua_Integer blue = luaL_checkinteger(L, 3);
    if (blue < 0 || blue > MAX_COLOR_COMPONENT) {
      return luaL_error(
          L, "invalid blue value: %d (must be 0-%d)",
          blue, MAX_COLOR_COMPONENT
      );
    }

    lua_pushinteger(L, (red << 16) | (green << 8) | blue);
    return 1;
  }
}

/**
 * Close the window. Does nothing if the window is already closed.
 * The __gc and __close meta methods also call this, so it's unlikely you'll
 * need to use this function directly.
 * Lua usage: `fenster.close(window: window)` or `window:close()`
 * @lua_param window userdata The window to close
 * @param L Lua state
 * @return Number of return values on the stack
 */
static int lua_fenster_close(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");

  if (p_lf->p_fenster == NULL) {
    // window already closed
    return 0;
  }

  // close and free window
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
  // TODO: pull out into a helper function
  const lua_Integer x = luaL_checkinteger(L, 2);
  if (x < 0 || x >= p_lf->original_width) {
    return luaL_error(
        L, "x coordinate out of bounds: %d (must be 0-%d)",
        x, p_lf->original_width - 1
    );
  }
  // TODO: pull out into a helper function
  const lua_Integer y = luaL_checkinteger(L, 3);
  if (y < 0 || y >= p_lf->original_height) {
      return luaL_error(
          L, "y coordinate out of bounds: %d (must be 0-%d)",
          y, p_lf->original_height - 1
      );
  }
  const lua_Integer color = luaL_checkinteger(L, 4);
  if (color < 0 || color > MAX_COLOR) {
    return luaL_error(
        L, "invalid color value: %d (must be 0-%d)",
        color, MAX_COLOR
    );
  }

  lua_Integer sy = y * p_lf->scale;
  const lua_Integer sy_end = (y + 1) * p_lf->scale;
  lua_Integer sx;
  const lua_Integer sx_start = x * p_lf->scale;
  const lua_Integer sx_end = (x + 1) * p_lf->scale;
  for (; sy < sy_end; sy++) {
    for (sx = sx_start; sx < sx_end; sx++) {
      fenster_pixel(p_lf->p_fenster, sx, sy) = color;
    }
  }

  return 0;
}

static int lua_fenster_get(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
  const lua_Integer x = luaL_checkinteger(L, 2);
  if (x < 0 || x >= p_lf->original_width) {
    return luaL_error(
        L, "x coordinate out of bounds: %d (must be 0-%d)",
        x, p_lf->original_width - 1
    );
  }
  const lua_Integer y = luaL_checkinteger(L, 3);
  if (y < 0 || y >= p_lf->original_height) {
    return luaL_error(
        L, "y coordinate out of bounds: %d (must be 0-%d)",
        y, p_lf->original_height - 1
    );
  }

  lua_pushinteger(
      L, fenster_pixel(p_lf->p_fenster, x * p_lf->scale, y * p_lf->scale)
  );
  return 1;
}

static int lua_fenster_clear(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
  const lua_Integer color = luaL_optinteger(L, 2, 0);
  if (color < 0 || color > MAX_COLOR) {
    return luaL_error(
        L, "invalid color value: %d (must be 0-%d)",
        color, MAX_COLOR
    );
  }

  // TODO: check if works and improve this
  memset(p_lf->p_fenster->buf, color, sizeof(p_lf->p_fenster->buf));

  return 0;
}

static int lua_fenster___index(lua_State *L) {
  // TODO: optimize and improve this function
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, "lua_fenster");
  const char *key = luaL_checkstring(L, 2);

  // First, check if the key exists in the methods table
  luaL_getmetatable(L, "lua_fenster");
  lua_pushstring(L, key);
  lua_rawget(L, -2);
  if (!lua_isnil(L, -1)) {
    // If the key exists in the methods table, return the method
    return 1;
  }
  lua_pop(L, 2); // Pop the nil and the metatable from the stack

  // If the key does not exist in the methods table, handle it as a property
  if (strcmp(key, "keys") == 0) {
    // retrieve keys table from registry and update it
    lua_pushvalue(L, 1);
    lua_gettable(L, LUA_REGISTRYINDEX);
    for (int i = 0; i < KEYS_LENGTH; i++) {
      lua_pushboolean(L, p_lf->p_fenster->keys[i]);
      lua_rawseti(L, -2, i);
    }
  } else if (strcmp(key, "delta") == 0) {
    lua_pushnumber(L, p_lf->delta);
  } else if (strcmp(key, "mousex") == 0) {
    // TODO: improve this part
    lua_pushinteger(L, llround((double) p_lf->p_fenster->x / p_lf->scale));
  } else if (strcmp(key, "mousey") == 0) {
    // TODO: improve this part
    lua_pushinteger(L, llround((double) p_lf->p_fenster->y / p_lf->scale));
  } else if (strcmp(key, "mousedown") == 0) {
    lua_pushboolean(L, p_lf->p_fenster->mouse);
  } else if (strcmp(key, "modcontrol") == 0) {
    lua_pushboolean(L, p_lf->p_fenster->mod & 1);
  } else if (strcmp(key, "modshift") == 0) {
    lua_pushboolean(L, (p_lf->p_fenster->mod >> 1) & 1);
  } else if (strcmp(key, "modalt") == 0) {
    lua_pushboolean(L, (p_lf->p_fenster->mod >> 2) & 1);
  } else if (strcmp(key, "modgui") == 0) {
    lua_pushboolean(L, (p_lf->p_fenster->mod >> 3) & 1);
  } else if (strcmp(key, "width") == 0) {
    lua_pushinteger(L, p_lf->original_width);
  } else if (strcmp(key, "height") == 0) {
    lua_pushinteger(L, p_lf->original_height);
  } else if (strcmp(key, "title") == 0) {
    lua_pushstring(L, p_lf->p_fenster->title);
  } else if (strcmp(key, "scale") == 0) {
    lua_pushinteger(L, p_lf->scale);
  } else if (strcmp(key, "targetfps") == 0) {
    lua_pushnumber(L, p_lf->target_fps);
  } else {
    lua_pushnil(L);
  }
  return 1;
}


static const struct luaL_Reg lua_fenster_functions[] = {
    {"open", lua_fenster_open},
    {"sleep", lua_fenster_sleep},
    {"time", lua_fenster_time},
    {"rgb", lua_fenster_rgb},

    // methods can also be used as functions with the window as first argument
    {"close", lua_fenster_close},
    {"loop", lua_fenster_loop},
    {"set", lua_fenster_set},
    {"get", lua_fenster_get},
    {"clear", lua_fenster_clear},

    {NULL, NULL} // sentinel
};

static const struct luaL_Reg lua_fenster_methods[] = {
    {"close", lua_fenster_close},
    {"loop", lua_fenster_loop},
    {"set", lua_fenster_set},
    {"get", lua_fenster_get},
    {"clear", lua_fenster_clear},

    {NULL, NULL} // sentinel
};


FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
  if (luaL_newmetatable(L, "lua_fenster")) {
    luaL_setfuncs(L, lua_fenster_methods, 0);

    lua_pushliteral(L, "__index");
    lua_pushcfunction(L, lua_fenster___index);
    lua_settable(L, -3);

    lua_pushliteral(L, "__gc");
    lua_pushcfunction(L, lua_fenster_close);
    lua_settable(L, -3);

    lua_pushliteral(L, "__close");
    lua_pushcfunction(L, lua_fenster_close);
    lua_settable(L, -3);
  }
  lua_pop(L, 1);

  luaL_newlib(L, lua_fenster_functions);
  return 1;
}
