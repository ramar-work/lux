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
#include "../lua.h"

//Need to replace with GnuTLS primitives and test again...
//Making C test programs might actually make your life easier...
//You can use embedded strings...
//#include <openssl/sha.h>

#if 1

#if !defined(DISABLE_TLS) && !defined(LHASH_H)
 #include <gnutls/crypto.h>
 #define LHASH_H
int generate_sha1( lua_State * );
int generate_sha224( lua_State * );
int generate_sha256( lua_State * );
int generate_sha384( lua_State * );
int generate_sha512( lua_State * );
extern struct luaL_Reg hash_set[];
#endif

#endif
