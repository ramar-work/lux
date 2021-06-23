#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zdb.h>
#include <zhttp.h>
#include <ztable.h>
#include "../lua.h"
#include "../util.h"

#ifndef LLUA_H
#define LLUA_H

int lua_dump_var ( lua_State * );

extern struct luaL_Reg lua_set[];

#endif

