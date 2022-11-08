/* ------------------------------------------- * 
 * hash.c 
 * ======
 * 
 * Summary 
 * -------
 * Handle common hashing tasks via Lua
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

#include "hash.h"

#define HYPNO_MAX_ALG_HASH_LENGTH SHA512_DIGEST_LENGTH 

static int calc( lua_State *L, int alg ) {
	unsigned char *src = NULL, *final = NULL; 
	unsigned char buf[ HYPNO_MAX_ALG_HASH_LENGTH ];
	int srclen = 0, buflen, type = lua_type( L, 1 );
	struct algorithm *c = NULL;

	if ( type != LUA_TSTRING && type != LUA_TTABLE ) {
		luaL_error( L, "Argument to hash.sha%d() was neither a string or table." );
		return 0;
	}
	else if ( type == LUA_TSTRING ) {
		src = (unsigned char *)lua_tostring( L, 1 );
		srclen = strlen( (char *) src );
		lua_pop( L, 1 );
	}	
	else if ( type == LUA_TTABLE ) {
		luaL_error( L, "Table as argument is under construction." );
		return 0;
		lua_pop( L, 1 );
	}

	memset( buf, 0, HYPNO_MAX_ALG_HASH_LENGTH );

	if ( alg == 1 )
		final = SHA1( src, srclen, buf ), buflen = SHA_DIGEST_LENGTH;
	else if ( alg == 224 )	
		final = SHA224( src, srclen, buf ), buflen = SHA224_DIGEST_LENGTH;
	else if ( alg == 256 )	
		final = SHA256( src, srclen, buf ), buflen = SHA256_DIGEST_LENGTH;
	else if ( alg == 384 )	
		final = SHA384( src, srclen, buf ), buflen = SHA384_DIGEST_LENGTH;
	else if ( alg == 512 ) {
		final = SHA512( src, srclen, buf ), buflen = SHA512_DIGEST_LENGTH;
	}

	//TODO: OpenSSL must give us some kind of error should one ever occur
	if ( !final ) {
		luaL_error( L, "OpenSSL error at hash.sha%d()", alg );
		return 0;
	}

	char fbuf[ ( buflen * 2 ) + 1 ], *sha;
	memset( fbuf, 0, ( buflen * 2 ) + 1 );
	sha = fbuf;

	//Save the printable data to a new buffer and return to user
	for ( int i = 0; i < buflen; i++, final++, sha += 2 ) {
		sprintf( sha, "%02x", *final );
	}

	lua_pushstring( L, fbuf );
	return 1;
}


int generate_sha1( lua_State *L ) {
	return calc( L, 1 );
}

int generate_sha224( lua_State *L ) {
	return calc( L, 224 );
}

int generate_sha256( lua_State *L ) {
	return calc( L, 256 );
}

int generate_sha384( lua_State *L ) {
	return calc( L, 384 );
}

int generate_sha512( lua_State *L ) {
	return calc( L, 512 );
}


struct luaL_Reg hash_set[] = {
 { "sha1", generate_sha1 }
,{ "sha224", generate_sha224 }
,{ "sha256", generate_sha256 }
,{ "sha384", generate_sha384 }
,{ "sha512", generate_sha512 }
,{ NULL }
};

#endif
