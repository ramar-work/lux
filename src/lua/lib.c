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
#include "hash.h"

#include "http.h"
#include "filesystem.h"
#include "db.h"

#ifdef INCLUDE_JSON_SUPPORT
 #include "json.h"
#endif

struct lua_fset functions[] = {
	{ "echo", echo_set }
, { "lua", lua_set }
, { "rand", rand_set }
, { "hash", hash_set }
, { "http", http_set }
, { "fs", fs_set }
, { "db", db_set }
#ifdef INCLUDE_JSON_SUPPORT
, { "json", json_set }
#endif
,	{ NULL }
};


