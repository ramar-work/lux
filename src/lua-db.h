#include "database.h"
#include "luabind.h"
#include <stdio.h>

#ifndef LUADB_H
#define LUADB_H

int lua_execdb( lua_State * );
int luaopen_libhypno( lua_State * );

#endif
