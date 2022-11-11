/* ------------------------------------------- * 
 * lib.c 
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
#include "lib.h"
#include "echo.h"
#include "lua.h"
#include "rand.h"
#include "http.h"
#include "filesystem.h"
#include "db.h"
#include "json.h"
#include "encdec.h"
#ifndef DISABLE_TLS
 #include "hash.h"
#endif

struct lua_fset functions[] = {
	{ "echo", echo_set }
, { "lua", lua_set }
, { "rand", rand_set }
, { "http", http_set }
, { "fs", fs_set }
, { "db", db_set }
, { "json", json_set }
, { "enc", enc_set }
, { "dec", dec_set }
#ifndef DISABLE_TLS
 #if 0
, { "hash", hash_set }
 #endif
#endif
,	{ NULL }
};


