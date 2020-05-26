#include <stdio.h>
#include <time.h>
#include "../vendor/zhasher.h"
#include "db-sqlite.h"
#include "luabind.h"

#ifndef LUADB_H
#define LUADB_H

int lua_execdb( lua_State * );
int luaopen_libhypno( lua_State * );

#endif
