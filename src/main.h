#ifndef FENSTER_MAIN_H
#define FENSTER_MAIN_H

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef _WIN32
#define FENSTER_EXPORT __declspec (dllexport)
#else
#define FENSTER_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

FENSTER_EXPORT int luaopen_fenster(lua_State* L);

#ifdef __cplusplus
}
#endif

#endif // FENSTER_MAIN_H
