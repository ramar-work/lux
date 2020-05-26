#include <stdio.h>
#include <time.h>
#include "../vendor/zhasher.h"
#include "db-sqlite.h"
#include "luabind.h"

#ifndef LUA_EXT_H
#define LUA_EXT_H

int lua_execdb( lua_State * );

#endif
