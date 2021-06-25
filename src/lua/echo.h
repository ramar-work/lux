/* ------------------------------------------- * 
 * echo.h 
 * ====
 * 
 * Summary 
 * -------
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
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

#ifndef ECHO_H
#define ECHO_H

int echo_string_arg ( lua_State *L );

int echo_numeric_arg ( lua_State *L );

int echo_table_arg ( lua_State *L );

extern struct luaL_Reg echo_set[]; 
#endif
