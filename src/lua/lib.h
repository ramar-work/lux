#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef LIB_H
#define LIB_H
struct lua_fset {
	const char *namespace;
	struct luaL_Reg *functions;
};
extern struct lua_fset functions[];
#endif
