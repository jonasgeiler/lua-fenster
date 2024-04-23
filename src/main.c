#include "../include/main.h"

#include <errno.h>
#include <lauxlib.h>
#include <lua.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/compat-5.3/compat-5.3.h"
#include "../lib/fenster/fenster.h"

// Macros that ensure the same integer argument behavior in Lua 5.1/5.2
// and 5.3/5.4. In Lua 5.1/5.2 luaL_checkinteger/luaL_optinteger normally floor
// decimal numbers, while in Lua 5.3/5.4 they throw an error. These macros make
// sure to always throw an error if the number has a decimal part.
#if defined(LUA_VERSION_NUM) && LUA_VERSION_NUM <= 502
#define luaL_checkinteger(L, arg)                                              \
  (luaL_argcheck(L,                                                            \
                 floorl(luaL_checknumber(L, arg)) == luaL_checknumber(L, arg), \
                 arg, "number has no integer representation"),                 \
   luaL_checkinteger(L, arg))
#define luaL_optinteger(L, arg, def) \
  (lua_isnoneornil(L, arg) ? def : luaL_checkinteger(L, arg))
#endif

/** Default window title */
static const char *DEFAULT_TITLE = "fenster";

/** Default window scale */
static const lua_Integer DEFAULT_SCALE = 1;

/** Default target frames per second */
static const lua_Number DEFAULT_TARGET_FPS = 60.0;

/** Number of milliseconds per second */
static const lua_Number MS_PER_SEC = 1000.0;

/** Length of the fenster->keys array */
static const int KEYS_LENGTH = sizeof(((struct fenster *)0)->keys) /
                               sizeof(((struct fenster *)0)->keys[0]);

/** Maximum width/height of the window */
static const lua_Integer MAX_DIMENSION = 15360;

/** Maximum color value */
static const lua_Integer MAX_COLOR = 0xffffff;

/** Maximum value of a color component (r, g or b) */
static const lua_Integer MAX_COLOR_COMPONENT = 0xff;

/** Bit offset of the red color component in a color value */
static const lua_Integer COLOR_RED_OFFSET = 16;

/** Bit offset of the green color component in a color value */
static const lua_Integer COLOR_GREEN_OFFSET = 8;

/** Name of the window userdata and metatable */
static const char *WINDOW_METATABLE = "window*";

/** Userdata representing the fenster window */
typedef struct window {
  // "private" members
  struct fenster *p_fenster;
  int keys_ref;
  int64_t target_frame_time;
  int64_t start_frame_time;
  size_t scaled_pixels;

  // "public" members
  lua_Number delta;
  lua_Integer scaled_mouse_x;
  lua_Integer scaled_mouse_y;
  int mod_control;
  int mod_shift;
  int mod_alt;
  int mod_gui;
  lua_Integer width;
  lua_Integer height;
  lua_Integer scale;
  lua_Number target_fps;
} window;

/*
// Utility function to dump the Lua stack for debugging
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

/**
 * Utility function to get a dimension value from the Lua stack and check if
 * it's within the allowed range.
 * @param L Lua state
 * @param index Index of the dimension value on the Lua stack
 * @return The dimension value
 */
static lua_Integer check_dimension(lua_State *L, int index) {
  const lua_Integer dimension = luaL_checkinteger(L, index);
  luaL_argcheck(L, dimension > 0 && dimension <= MAX_DIMENSION, index,
                "width/height must be in range 1-15360");
  return dimension;
}

/**
 * Opens a window with the given width, height, title, scale and target FPS.
 * Returns a userdata representing the window with all the methods and
 * properties we defined on the metatable.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lfenster_open(lua_State *L) {
  const lua_Integer width = check_dimension(L, 1);
  const lua_Integer height = check_dimension(L, 2);
  const char *title = luaL_optstring(L, 3, DEFAULT_TITLE);
  const lua_Integer scale = luaL_optinteger(L, 4, DEFAULT_SCALE);
  luaL_argcheck(L, (scale & (scale - 1)) == 0, 4, "scale must be a power of 2");
  const lua_Number target_fps = luaL_optnumber(L, 5, DEFAULT_TARGET_FPS);
  luaL_argcheck(L, target_fps >= 0.0, 5, "target fps must be non-negative");

  // calculate the scaled width, scaled height and amount of pixels
  const size_t scaled_width = width * scale;
  const size_t scaled_height = height * scale;
  const size_t scaled_pixels = scaled_width * scaled_height;

  // allocate memory for the window buffer
  uint32_t *buffer = calloc(scaled_pixels, sizeof(uint32_t));
  if (buffer == NULL) {
    const int error = errno;
    return luaL_error(
        L, "failed to allocate memory of size %d for window buffer (%d)",
        scaled_pixels * sizeof(uint32_t), error);
  }

  // use a temporary fenster struct to copy into the "real" one later
  // (width and height use narrow casts to int, but we made sure it's in range)
  struct fenster temp_fenster = {
      .title = title,
      .width = (int)scaled_width,
      .height = (int)scaled_height,
      .buf = buffer,
  };

  // allocate memory for the "real" fenster struct
  struct fenster *p_fenster = malloc(sizeof(struct fenster));
  if (p_fenster == NULL) {
    const int error = errno;
    free(buffer);
    buffer = NULL;
    return luaL_error(L, "failed to allocate memory of size %d for window (%d)",
                      sizeof(struct fenster), error);
  }

  // copy temporary fenster struct into the "real" one
  // (we have to do it this way because width and height are const)
  memcpy(p_fenster, &temp_fenster, sizeof(struct fenster));

  // open window and check success
  const int result = fenster_open(p_fenster);
  if (result != 0) {
    free(buffer);
    buffer = NULL;
    free(p_fenster);
    p_fenster = NULL;
    return luaL_error(L, "failed to open window (%d)", result);
  }

  // initialize the keys table and put it in the registry
  lua_createtable(L, KEYS_LENGTH, 0);
  for (int i = 0; i < KEYS_LENGTH; i++) {
    lua_pushboolean(L, p_fenster->keys[i]);
    lua_rawseti(L, -2, i);
  }
  lua_pushvalue(L, -1);  // copy the keys table since luaL_ref pops it
  const int keys_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  if (keys_ref == LUA_REFNIL || keys_ref == LUA_NOREF) {
    fenster_close(p_fenster);
    free(buffer);
    buffer = NULL;
    free(p_fenster);
    p_fenster = NULL;
    return luaL_error(L, "failed to create keys table (%d)", keys_ref);
  }
  lua_rawseti(L, LUA_REGISTRYINDEX, keys_ref);

  // create the window userdata and initialize it
  window *p_window = lua_newuserdata(L, sizeof(window));
  p_window->p_fenster = p_fenster;
  p_window->keys_ref = keys_ref;
  p_window->target_frame_time =
      target_fps ? llroundl(MS_PER_SEC / target_fps) : 0;
  p_window->start_frame_time = 0;
  p_window->scaled_pixels = scaled_pixels;
  p_window->delta = 0.0;
  p_window->scaled_mouse_x = 0;
  p_window->scaled_mouse_y = 0;
  p_window->mod_control = 0;
  p_window->mod_shift = 0;
  p_window->mod_alt = 0;
  p_window->mod_gui = 0;
  p_window->width = width;
  p_window->height = height;
  p_window->scale = scale;
  p_window->target_fps = target_fps;
  luaL_setmetatable(L, WINDOW_METATABLE);
  return 1;
}

/**
 * Pauses for a given number of milliseconds.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lfenster_sleep(lua_State *L) {
  const lua_Integer milliseconds = luaL_checkinteger(L, 1);

  fenster_sleep(milliseconds);

  return 0;
}

/**
 * Returns the current time in milliseconds.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lfenster_time(lua_State *L) {
  lua_pushinteger(L, fenster_time());
  return 1;
}

/**
 * Utility function to get a color value from the Lua stack and check if it's
 * within the allowed range.
 * @param L Lua state
 * @param index Index of the color value on the Lua stack
 * @return The color value
 */
static lua_Integer check_color(lua_State *L, int index) {
  const lua_Integer color = luaL_checkinteger(L, index);
  luaL_argcheck(L, color >= 0 && color <= MAX_COLOR, index,
                "color must be in range 0x000000-0xffffff");
  return color;
}

/**
 * Utility function to get a color component from the Lua stack and check if
 * it's within the allowed range.
 * @param L Lua state
 * @param index Index of the color component on the Lua stack
 * @return The color component value
 */
static lua_Integer check_color_component(lua_State *L, int index) {
  const lua_Integer color_component = luaL_checkinteger(L, index);
  luaL_argcheck(L,
                color_component >= 0 && color_component <= MAX_COLOR_COMPONENT,
                index, "color component must be in range 0-255");
  return color_component;
}

/**
 * Utility function to convert a color value to RGB components or vice versa.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int lfenster_rgb(lua_State *L) {
  // check if the function was called with less than 3 arguments
  if (lua_gettop(L) < 3) {
    // get color value argument
    const lua_Integer color = check_color(L, 1);

    // return RGB components
    lua_pushinteger(L, (color >> COLOR_RED_OFFSET) & MAX_COLOR_COMPONENT);
    lua_pushinteger(L, (color >> COLOR_GREEN_OFFSET) & MAX_COLOR_COMPONENT);
    lua_pushinteger(L, color & MAX_COLOR_COMPONENT);
    return 3;
  }

  // get RGB component arguments
  const lua_Integer red = check_color_component(L, 1);
  const lua_Integer green = check_color_component(L, 2);
  const lua_Integer blue = check_color_component(L, 3);

  // return color value
  lua_pushinteger(
      L, (red << COLOR_RED_OFFSET) | (green << COLOR_GREEN_OFFSET) | blue);
  return 1;
}

/** Macro to get the window userdata from the Lua stack */
#define check_window(L) (luaL_checkudata(L, 1, WINDOW_METATABLE))

/** Macro to check if the window is closed */
#define is_window_closed(p_window) ((p_window)->p_fenster == NULL)

/**
 * Utility function to get the window userdata from the Lua stack and check if
 * the window is open.
 * @param L Lua state
 * @return The window userdata
 */
static window *check_open_window(lua_State *L) {
  window *p_window = check_window(L);
  if (is_window_closed(p_window)) {
    luaL_error(L, "attempt to use a closed window");
  }
  return p_window;
}

/**
 * Close the window. Does nothing if the window is already closed.
 * The __gc and __close meta methods also call this function, so the user
 * likely won't need to call this function manually.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_close(lua_State *L) {
  window *p_window = check_open_window(L);

  // close and free window
  fenster_close(p_window->p_fenster);
  free(p_window->p_fenster->buf);
  p_window->p_fenster->buf = NULL;
  free(p_window->p_fenster);
  p_window->p_fenster = NULL;
  luaL_unref(L, LUA_REGISTRYINDEX, p_window->keys_ref);  // free keys table
  p_window->keys_ref = LUA_NOREF;

  return 0;
}

/**
 * Main loop for the window. Handles FPS limiting and updates delta time, keys,
 * mouse coordinates, modifier keys and the whole screen. Returns true if the
 * window is still open and false if it's closed (only on Windows right now).
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_loop(lua_State *L) {
  window *p_window = check_open_window(L);

  // handle fps limiting
  int64_t now = fenster_time();
  if (p_window->start_frame_time == 0) {
    // initialize start frame time (this is the first frame)
    p_window->start_frame_time = now;
  } else {
    const int64_t last_frame_time = now - p_window->start_frame_time;
    if (p_window->target_frame_time > last_frame_time) {
      // sleep for the remaining frame time to reach target frame time
      fenster_sleep(p_window->target_frame_time - last_frame_time);
    }
    now = fenster_time();  // update timestamp after sleeping
    p_window->delta =
        (lua_Number)(now - p_window->start_frame_time) / MS_PER_SEC;
    p_window->start_frame_time = now;
  }

  if (fenster_loop(p_window->p_fenster) == 0) {
    // update the keys table in the registry
    lua_rawgeti(L, LUA_REGISTRYINDEX, p_window->keys_ref);
    for (int i = 0; i < KEYS_LENGTH; i++) {
      lua_pushboolean(L, p_window->p_fenster->keys[i]);
      lua_rawseti(L, -2, i);
    }

    // update the scaled mouse coordinates (floors the coordinates)
    p_window->scaled_mouse_x = p_window->p_fenster->x / p_window->scale;
    p_window->scaled_mouse_y = p_window->p_fenster->y / p_window->scale;

    // update the modifier keys
    p_window->mod_control = p_window->p_fenster->mod & 1;
    p_window->mod_shift = (p_window->p_fenster->mod >> 1) & 1;
    p_window->mod_alt = (p_window->p_fenster->mod >> 2) & 1;
    p_window->mod_gui = (p_window->p_fenster->mod >> 3) & 1;

    lua_pushboolean(L, 1);
  } else {
    lua_pushboolean(L, 0);
  }
  return 1;
}

/**
 * Utility function to get the x coordinate from the Lua stack and check if it's
 * within bounds.
 * @param L Lua state
 * @param index Index of the x coordinate on the Lua stack
 * @param p_window The lua-fenster userdata
 * @return The x coordinate
 */
static lua_Integer check_x(lua_State *L, window *p_window) {
  const lua_Integer x = luaL_checkinteger(L, 2);
  luaL_argcheck(L, x >= 0 && x < p_window->width, 2,
                "x coordinate must be in range 0-[width-1]");
  return x;
}

/**
 * Utility function to get the y coordinate from the Lua stack and check if it's
 * within bounds.
 * @param L Lua state
 * @param index Index of the y coordinate on the Lua stack
 * @param p_window The lua-fenster userdata
 * @return The y coordinate
 */
static lua_Integer check_y(lua_State *L, window *p_window) {
  const lua_Integer y = luaL_checkinteger(L, 3);
  luaL_argcheck(L, y >= 0 && y < p_window->height, 3,
                "y coordinate must be in range 0-[height-1]");
  return y;
}

/**
 * Set a pixel in the window buffer at the given coordinates to the given color.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_set(lua_State *L) {
  window *p_window = check_open_window(L);
  const lua_Integer x = check_x(L, p_window);
  const lua_Integer y = check_y(L, p_window);
  const lua_Integer color = check_color(L, 4);

  // set the pixel at the scaled coordinates to the given color
  // (repeat this for each copy of the pixel in an area the size of the scale)
  lua_Integer scaled_y = y * p_window->scale;
  const lua_Integer scaled_y_end = scaled_y + p_window->scale;
  lua_Integer scaled_x;
  const lua_Integer scaled_x_begin = x * p_window->scale;
  const lua_Integer scaled_x_end = scaled_x_begin + p_window->scale;
  for (; scaled_y < scaled_y_end; scaled_y++) {
    for (scaled_x = scaled_x_begin; scaled_x < scaled_x_end; scaled_x++) {
      fenster_pixel(p_window->p_fenster, scaled_x, scaled_y) = color;
    }
  }

  return 0;
}

/**
 * Get the color of a pixel in the window buffer at the given coordinates.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_get(lua_State *L) {
  window *p_window = check_open_window(L);
  const lua_Integer x = check_x(L, p_window);
  const lua_Integer y = check_y(L, p_window);

  // get the color of the pixel at the scaled coordinates
  // (we don't need a loop here like in the set method because we only need
  // the color of the first pixel in the scaled area - they should all be same)
  lua_pushinteger(L, fenster_pixel(p_window->p_fenster, x * p_window->scale,
                                   y * p_window->scale));
  return 1;
}

/**
 * Clear the window buffer with the given color.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_clear(lua_State *L) {
  window *p_window = check_open_window(L);
  // we cant use check_color here since it's optional...
  const lua_Integer color = luaL_optinteger(L, 2, 0x000000);
  luaL_argcheck(L, color >= 0 && color <= MAX_COLOR, 2,
                "color must be in range 0x000000-0xffffff");

  // overwrite the whole buffer with the given color
  for (size_t i = 0; i < p_window->scaled_pixels; i++) {
    p_window->p_fenster->buf[i] = color;
  }

  return 0;
}

/**
 * Index function for the window userdata. Checks if the key exists in the
 * methods metatable and returns the method if it does. Otherwise, checks for
 * properties and returns the property value if it exists.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_index(lua_State *L) {
  window *p_window = check_open_window(L);
  const char *key = luaL_checkstring(L, 2);

  // check if the key exists in the methods metatable
  luaL_getmetatable(L, WINDOW_METATABLE);
  lua_pushvalue(L, 2);
  lua_rawget(L, -2);
  if (lua_isnil(L, -1)) {
    // key not found in the methods metatable, check for properties
    if (strcmp(key, "keys") == 0) {
      // retrieve the keys table from the registry
      lua_rawgeti(L, LUA_REGISTRYINDEX, p_window->keys_ref);
    } else if (strcmp(key, "delta") == 0) {
      lua_pushnumber(L, p_window->delta);
    } else if (strcmp(key, "mousex") == 0) {
      lua_pushinteger(L, p_window->scaled_mouse_x);
    } else if (strcmp(key, "mousey") == 0) {
      lua_pushinteger(L, p_window->scaled_mouse_y);
    } else if (strcmp(key, "mousedown") == 0) {
      lua_pushboolean(L, p_window->p_fenster->mouse);
    } else if (strcmp(key, "modcontrol") == 0) {
      lua_pushboolean(L, p_window->mod_control);
    } else if (strcmp(key, "modshift") == 0) {
      lua_pushboolean(L, p_window->mod_shift);
    } else if (strcmp(key, "modalt") == 0) {
      lua_pushboolean(L, p_window->mod_alt);
    } else if (strcmp(key, "modgui") == 0) {
      lua_pushboolean(L, p_window->mod_gui);
    } else if (strcmp(key, "width") == 0) {
      lua_pushinteger(L, p_window->width);
    } else if (strcmp(key, "height") == 0) {
      lua_pushinteger(L, p_window->height);
    } else if (strcmp(key, "title") == 0) {
      lua_pushstring(L, p_window->p_fenster->title);
    } else if (strcmp(key, "scale") == 0) {
      lua_pushinteger(L, p_window->scale);
    } else if (strcmp(key, "targetfps") == 0) {
      lua_pushnumber(L, p_window->target_fps);
    } else {
      // no matching key is found, return nil
      lua_pushnil(L);
    }
  }
  return 1;  // return either the method or the property value
}

/**
 * Close the window when the window userdata is garbage collected.
 * Just calls the close method but ignores if the window is already closed.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_gc(lua_State *L) {
  window *p_window = check_window(L);

  // ignore if the window is already closed
  if (!is_window_closed(p_window)) {
    window_close(L);
  }

  return 0;
}

/**
 * Returns a string representation of the window userdata.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
static int window_tostring(lua_State *L) {
  window *p_window = check_window(L);

  if (is_window_closed(p_window)) {
    lua_pushliteral(L, "window (closed)");
  } else {
    lua_pushfstring(L, "window (%p)", p_window);
  }
  return 1;
}

/** Functions for the lua-fenster module */
static const struct luaL_Reg lfenster_functions[] = {
    {"open", lfenster_open},
    {"sleep", lfenster_sleep},
    {"time", lfenster_time},
    {"rgb", lfenster_rgb},

    // methods can also be used as functions with the userdata as first argument
    {"close", window_close},
    {"loop", window_loop},
    {"set", window_set},
    {"get", window_get},
    {"clear", window_clear},

    {NULL, NULL}};

/** Methods for the window userdata */
static const struct luaL_Reg window_methods[] = {
    {"close", window_close},
    {"loop", window_loop},
    {"set", window_set},
    {"get", window_get},
    {"clear", window_clear},

    // metamethods
    {"__index", window_index},
    {"__gc", window_gc},
#if LUA_VERSION_NUM >= 504
    {"__close", window_gc},
#endif
    {"__tostring", window_tostring},

    {NULL, NULL}};

/**
 * Entry point for the lua-fenster module.
 * @param L Lua state
 * @return Number of return values on the Lua stack
 */
FENSTER_EXPORT int luaopen_fenster(lua_State *L) {
  // create the window metatable
  const int result = luaL_newmetatable(L, WINDOW_METATABLE);
  if (result == 0) {
    luaL_error(L, "fenster metatable already exists (%s)", WINDOW_METATABLE);
  }
  luaL_setfuncs(L, window_methods, 0);

  // create and return the lua-fenster module
  luaL_newlib(L, lfenster_functions);
  return 1;
}
