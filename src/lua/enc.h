/* ------------------------------------------- * 
 * enc.h 
 * =====
 * Popular encoding routines
 *
 * 
 * Usage
 * -----
 * ....
 *
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * See LICENSE in the top-level directory for more information.
 *
 * ------------------------------------------- */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "../lua.h"
#include "../util.h"

#ifndef LENC_H
#define LENC_H
int base64_encode ( lua_State * );
extern struct luaL_Reg enc_set[];
#endif
