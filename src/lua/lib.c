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
#include "db.h"
#include "lua.h"
#include "filesystem.h"
#include "http.h"
#include "json.h"
#include "rand.h"
#include "hash.h"

struct lua_fset functions[] = {
	{ "echo", echo_set }
, { "db", db_set }
, { "lua", lua_set }
, { "fs", fs_set }
, { "http", http_set }
, { "json", json_set }
, { "rand", rand_set }
#if 1
, { "hash", hash_set }
#endif
,	{ NULL }
};


