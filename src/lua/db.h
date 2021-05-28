#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zdb.h>
#include <zhttp.h>
#include <ztable.h>
#include "../lua.h"
#include "../util.h"

#ifndef LDB_H
#define LDB_H

int db_exec ( lua_State * );

extern struct luaL_Reg db_set[];

#ifdef ZTABLE_H

#endif

#endif

