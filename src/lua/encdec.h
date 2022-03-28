/* ------------------------------------------- * 
 * encdec.h 
 * ========
 * 
 * Summary 
 * -------
 * Popular encoding routines
 * 
 * Usage
 * -----
 * 
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
#include "../lua.h"
#include "../util.h"

#ifndef LENCDEC_H
#define LENCDEC_H

int base64_encode ( lua_State * );

int base64_decode ( lua_State * );

extern struct luaL_Reg dec_set[]; 

extern struct luaL_Reg enc_set[];

#endif
