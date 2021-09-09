/* ------------------------------------------- * 
 * lua.h
 * ======
 * 
 * Summary 
 * -------
 * Lua primitives
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
#include <zhttp.h>
#include <ztable.h>
#include <zrender.h>
#include <zmime.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <router.h>

#ifndef LUA_BASE_H
#define LUA_BASE_H 

#define LD_LEN 128

#define LD_ERRBUF_LEN 1024

#define lua_pushibt(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushboolean(L, v), lua_settable(L, p)

#define lua_pushict(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushcclosure(L, v), lua_settable(L, p)

#define lua_pushift(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushcfunction(L, v), lua_settable(L, p)

#define lua_pushint(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushnumber(L, v), lua_settable(L, p)

#define lua_pushiit(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushinteger(L, v), lua_settable(L, p)

#define lua_pushist(L, i, v, p) \
	lua_pushinteger(L, i), lua_pushstring(L, v), lua_settable(L, p)

#define lua_pushsbt(L, s, v, p) \
	lua_pushstring(L, s), lua_pushboolean(L, v), lua_settable(L, p)

#define lua_pushsct(L, s, v, p) \
	lua_pushstring(L, s), lua_pushcclosure(L, v), lua_settable(L, p)

#define lua_pushsft(L, s, v, p) \
	lua_pushstring(L, s), lua_pushcfunction(L, v), lua_settable(L, p)

#define lua_pushsnt(L, s, v, p) \
	lua_pushstring(L, s), lua_pushnumber(L, v), lua_settable(L, p)

#define lua_pushsit(L, s, v, p) \
	lua_pushstring(L, s), lua_pushinteger(L, v), lua_settable(L, p)

#define lua_pushsst(L, s, v, p) \
	lua_pushstring(L, s), lua_pushstring(L, v), lua_settable(L, p)

enum zlua_error {
	ZLUA_NO_ERROR,
	ZLUA_MISSING_ARGS,
	ZLUA_INCORRECT_ARGS
};


void lua_dumpstack ( lua_State * );
int ztable_to_lua ( lua_State *, zTable * ) ;
int lua_to_ztable ( lua_State *, int, zTable * ) ;
int lua_exec_file( lua_State *, const char *, char *, int );
int lua_merge ( lua_State * );
int lua_count ( lua_State *, int );
void lua_istack ( lua_State * );
int lua_retglobal( lua_State *, const char *, int );
int lua_getv ( lua_State *, const char *, int, int );

//int http_error( struct HTTPBody *, int, char *, ... );
//unsigned char *read_file ( const char *, int *, char *, int );
//void * add_item_to_list( void ***, void *, int, int *);

#define lua_stackdump(L) lua_dumpstack(L)

#endif
