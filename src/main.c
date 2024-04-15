#include "main.h"
#include "vendor/fenster.h"
#include "vendor/compat-5.3.h"

/** Name of the lua_fenster userdata and metatable */
static const char *USERDATA_NAME = "lua_fenster";

/** Maximum color value */
static const lua_Integer MAX_COLOR = 0xffffff;

/** Maximum value of a color component (r, g or b) */
static const lua_Integer MAX_COLOR_COMPONENT = 0xff;

/** Length of the fenster->keys array */
static const int KEYS_LENGTH = sizeof(((struct fenster *) 0)->keys)
    / sizeof(((struct fenster *) 0)->keys[0]);

/** Userdata for the lua-fenster module */
typedef struct lua_fenster {
  struct fenster *p_fenster;

  lua_Integer scale;
  lua_Integer original_width;
  lua_Integer original_height;
  lua_Integer scaled_mouse_x;
  lua_Integer scaled_mouse_y;

  int mod_control;
  int mod_shift;
  int mod_alt;
  int mod_gui;

  lua_Number target_fps;
  lua_Number delta;
  int64_t target_frame_time;
  int64_t start_frame_time;

  size_t buffer_size;
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
      case LUA_TBOOLEAN:printf("%s\n", lua_toboolean(L, i) ? "true" : "false");
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
        L,
        "invalid scale value: %d (must be a power of 2)",
        scale
    );
  }
  const lua_Number target_fps = luaL_optnumber(L, 5, 60.0);

  const int scaled_width = width * scale;
  const int scaled_height = height * scale;
  const size_t scaled_pixels = scaled_width * scaled_height;

  uint32_t *buffer = (uint32_t *) calloc(
      scaled_pixels,
      sizeof(uint32_t)
  );
  const size_t buffer_size = scaled_pixels * sizeof(uint32_t);
  if (buffer == NULL) {
    const int error = errno;
    return luaL_error(
        L,
        "failed to allocate memory of size %d for window buffer (%d)",
        buffer_size, error
    );
  }

  struct fenster temp_fenster = {
      .title = title,
      .width = scaled_width,
      .height = scaled_height,
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
  p_lf->scale = scale;
  p_lf->original_width = width;
  p_lf->original_height = height;
  p_lf->scaled_mouse_x = p_fenster->x / scale;
  p_lf->scaled_mouse_y = p_fenster->y / scale;
  p_lf->mod_control = 0;
  p_lf->mod_shift = 0;
  p_lf->mod_alt = 0;
  p_lf->mod_gui = 0;
  p_lf->target_fps = target_fps;
  p_lf->delta = 0.0;
  p_lf->target_frame_time = llroundl(1000.0 / target_fps);
  p_lf->start_frame_time = 0;
  p_lf->buffer_size = buffer_size;
  luaL_setmetatable(L, USERDATA_NAME);

  // initialize the keys table and put it in the registry
  lua_pushvalue(L, -1);
  lua_createtable(L, KEYS_LENGTH, 0);
  for (int i = 0; i < KEYS_LENGTH; i++) {
    lua_pushboolean(L, p_fenster->keys[i]);
    lua_rawseti(L, -2, i);
  }
  lua_settable(L, LUA_REGISTRYINDEX);

  return 1;
}

/**
 * Pauses for a given number of milliseconds.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_sleep(lua_State *L) {
  const lua_Integer ms = luaL_checkinteger(L, 1);

  fenster_sleep(ms);

  return 0;
}

/**
 * Returns the current time in milliseconds.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_time(lua_State *L) {
  lua_pushinteger(L, fenster_time());
  return 1;
}

/**
 * Utility function to convert a color value to its RGB components or vice versa.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_rgb(lua_State *L) {
  if (lua_gettop(L) < 3) {
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
 * The __gc and __close meta methods also call this function, so the user
 * likely won't need to call this function manually.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_close(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, USERDATA_NAME);

  // check if the window is already closed
  if (p_lf->p_fenster != NULL) {
    // close and free window
    fenster_close(p_lf->p_fenster);
    free(p_lf->p_fenster->buf);
    p_lf->p_fenster->buf = NULL;
    free(p_lf->p_fenster);
    p_lf->p_fenster = NULL;
  }

  return 0;
}

static int lua_fenster_loop(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, USERDATA_NAME);

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
  now = fenster_time(); // update now after sleep
  p_lf->delta = (lua_Number) (now - p_lf->start_frame_time) / 1000.0;
  p_lf->start_frame_time = now;

  if (fenster_loop(p_lf->p_fenster) == 0) {
    // update the keys table in the registry
    lua_pushvalue(L, 1);
    lua_gettable(L, LUA_REGISTRYINDEX);
    for (int i = 0; i < KEYS_LENGTH; i++) {
      lua_pushboolean(L, p_lf->p_fenster->keys[i]);
      lua_rawseti(L, -2, i);
    }

    // update the scaled mouse coordinates
    p_lf->scaled_mouse_x = p_lf->p_fenster->x / p_lf->scale;
    p_lf->scaled_mouse_y = p_lf->p_fenster->y / p_lf->scale;

    // update the modifier keys
    p_lf->mod_control = p_lf->p_fenster->mod & 1;
    p_lf->mod_shift = (p_lf->p_fenster->mod >> 1) & 1;
    p_lf->mod_alt = (p_lf->p_fenster->mod >> 2) & 1;
    p_lf->mod_gui = (p_lf->p_fenster->mod >> 3) & 1;

    lua_pushboolean(L, 1);
  } else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

/**
 * Set a pixel at the given coordinates to the given color.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_set(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, USERDATA_NAME);
  const lua_Integer x = luaL_checkinteger(L, 2);
  if (x < 0 || x >= p_lf->original_width) {
    luaL_error(
        L,
        "x coordinate out of bounds: %d (must be 0-%d)",
        x, p_lf->original_width - 1
    );
  }
  const lua_Integer y = luaL_checkinteger(L, 3);
  if (y < 0 || y >= p_lf->original_height) {
    return luaL_error(
        L,
        "y coordinate out of bounds: %d (must be 0-%d)",
        y, p_lf->original_height - 1
    );
  }
  const lua_Integer color = luaL_checkinteger(L, 4);
  if (color < 0 || color > MAX_COLOR) {
    return luaL_error(
        L,
        "invalid color value: %d (must be 0-%d)",
        color, MAX_COLOR
    );
  }

  // set the pixel at the scaled coordinates to the given color
  // (repeat this for each copy of the pixel in an area the size of the scale)
  lua_Integer sy = y * p_lf->scale;
  const lua_Integer sy_end = sy + p_lf->scale;
  lua_Integer sx;
  const lua_Integer sx_begin = x * p_lf->scale;
  const lua_Integer sx_end = sx_begin + p_lf->scale;
  for (; sy < sy_end; sy++) {
    for (sx = sx_begin; sx < sx_end; sx++) {
      fenster_pixel(p_lf->p_fenster, sx, sy) = color;
    }
  }

  return 0;
}

/**
 * Get the color of a pixel at the given coordinates.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_get(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, USERDATA_NAME);
  const lua_Integer x = luaL_checkinteger(L, 2);
  if (x < 0 || x >= p_lf->original_width) {
    return luaL_error(
        L,
        "x coordinate out of bounds: %d (must be 0-%d)",
        x, p_lf->original_width - 1
    );
  }
  const lua_Integer y = luaL_checkinteger(L, 3);
  if (y < 0 || y >= p_lf->original_height) {
    return luaL_error(
        L,
        "y coordinate out of bounds: %d (must be 0-%d)",
        y, p_lf->original_height - 1
    );
  }

  // get the color of the pixel at the scaled coordinates
  // (we don't need a loop here like in lua_fenster_set because we only need
  // the color of the first pixel in the scaled area - they should all be same)
  lua_pushinteger(
      L,
      fenster_pixel(p_lf->p_fenster, x * p_lf->scale, y * p_lf->scale)
  );
  return 1;
}

/**
 * Clear the window with the given color.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lua_fenster_clear(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, USERDATA_NAME);
  const lua_Integer color = luaL_optinteger(L, 2, 0);
  if (color < 0 || color > MAX_COLOR) {
    return luaL_error(
        L, "invalid color value: %d (must be 0-%d)",
        color, MAX_COLOR
    );
  }

  // set the whole buffer to the given color at once
  memset(p_lf->p_fenster->buf, (int) color, p_lf->buffer_size);

  return 0;
}

static int lua_fenster___index(lua_State *L) {
  lua_fenster *p_lf = (lua_fenster *) luaL_checkudata(L, 1, USERDATA_NAME);
  const char *key = luaL_checkstring(L, 2);

  // check if the key exists in the methods metatable
  luaL_getmetatable(L, USERDATA_NAME);
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);
  if (!lua_isnil(L, -1)) {
    // if the key exists in the methods metatable, return the method
    return 1;
  }
  // otherwise, pop the nil value and the metatable from the stack...
  lua_pop(L, 2);

  // ...and handle the key as a property key
  if (strcmp(key, "keys") == 0) {
    // retrieve the keys table from the registry
    lua_pushvalue(L, 1);
    lua_gettable(L, LUA_REGISTRYINDEX);
  } else if (strcmp(key, "delta") == 0) {
    lua_pushnumber(L, p_lf->delta);
  } else if (strcmp(key, "mousex") == 0) {
    lua_pushinteger(L, p_lf->scaled_mouse_x);
  } else if (strcmp(key, "mousey") == 0) {
    lua_pushinteger(L, p_lf->scaled_mouse_y);
  } else if (strcmp(key, "mousedown") == 0) {
    lua_pushboolean(L, p_lf->p_fenster->mouse);
  } else if (strcmp(key, "modcontrol") == 0) {
    lua_pushboolean(L, p_lf->mod_control);
  } else if (strcmp(key, "modshift") == 0) {
    lua_pushboolean(L, p_lf->mod_shift);
  } else if (strcmp(key, "modalt") == 0) {
    lua_pushboolean(L, p_lf->mod_alt);
  } else if (strcmp(key, "modgui") == 0) {
    lua_pushboolean(L, p_lf->mod_gui);
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
    // when no matching key is found, return nil
    lua_pushnil(L);
  }
  return 1;
}

/** Functions for the lua-fenster module */
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

/** Methods for the lua-fenster userdata */
static const struct luaL_Reg lua_fenster_methods[] = {
    {"close", lua_fenster_close},
    {"loop", lua_fenster_loop},
    {"set", lua_fenster_set},
    {"get", lua_fenster_get},
    {"clear", lua_fenster_clear},

    {NULL, NULL} // sentinel
};

/**
 * Entry point for the lua-fenster module.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
  if (luaL_newmetatable(L, USERDATA_NAME)) {
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
