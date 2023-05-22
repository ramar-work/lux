/*json.h*/
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zjson.h>
#include "../lua.h"

#ifndef JSON_H
#define JSON_H
extern struct luaL_Reg json_set[];
#endif

#ifdef LUA_LOPEN
int luaopen_json (lua_State *);
#endif
