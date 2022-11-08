/* ------------------------------------------- * 
 * hash.h
 * ======
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
#ifndef NO_HTTPS_SUPPORT
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <openssl/sha.h>
#include "../lua.h"
#include "../util.h"

#ifndef LHASH_H
#define LHASH_H

int generate_sha1( lua_State * );
int generate_sha224( lua_State * );
int generate_sha256( lua_State * );
int generate_sha384( lua_State * );
int generate_sha512( lua_State * );
extern struct luaL_Reg hash_set[];

#endif
#endif
