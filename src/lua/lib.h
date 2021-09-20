/* ------------------------------------------- * 
 * lib.h 
 * ====
 * 
 * Summary 
 * -------
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "../config.h"

#ifndef LIB_H
#define LIB_H
struct lua_fset {
	const char *namespace;
	struct luaL_Reg *functions;
};
extern struct lua_fset functions[];
#endif
