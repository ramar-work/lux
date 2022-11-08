#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef RAN_H
#define RAN_H

int rand_str ( lua_State * ) ;
int rand_seq ( lua_State * ) ;
int rand_nums ( lua_State * ) ;
extern struct luaL_Reg rand_set[];

#endif
