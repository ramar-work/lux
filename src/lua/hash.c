/* -------------------------------------------- * 
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
sha1
sha224
sha256
sha384
sha512
 * -------------------------------------------- */
#include "hash.h"

#ifndef DISABLE_TLS

static int calc( lua_State *L, int alg ) {
	uint8_t *src = NULL, buf[64] = {0}, fbuf[ sizeof( buf ) + 1 ] = {0};
	int status = -1, srclen = 0, buflen = 0, type = lua_type( L, 1 );

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

	memset( buf, 0, sizeof( buf ) );

	if ( alg == 1 ) {
		buflen = gnutls_hash_get_len( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA1 );
		status = gnutls_hash_fast( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA1, src, srclen, &buf );
	}
	else if ( alg == 224 ) {
		buflen = gnutls_hash_get_len( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA224 );
		status = gnutls_hash_fast( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA224, src, srclen, &buf );
	}
	else if ( alg == 256 ){
		buflen = gnutls_hash_get_len( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA256 );
		status = gnutls_hash_fast( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA256, src, srclen, &buf );
	}
	else if ( alg == 384 ) {
		buflen = gnutls_hash_get_len( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA384 );
		status = gnutls_hash_fast( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA384, src, srclen, &buf );
	}
	else if ( alg == 512 ) {
		buflen = gnutls_hash_get_len( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA512 );
		status = gnutls_hash_fast( (gnutls_digest_algorithm_t)GNUTLS_MAC_SHA512, src, srclen, &buf );
	}

	//TODO: Let's be more specific about errors here.
	if ( status != 0 ) {
		luaL_error( L, "GnuTLS error at hash.sha%d()", alg );
		return 0;
	}

	//Save the printable data to a new buffer and return to user
	for ( uint8_t *shasrc = buf, *shadest = fbuf; buflen; buflen-- ) {
		sprintf( (char *)shadest, "%02x", *(shasrc++) ), shadest += 2;
	}

	lua_pushstring( L, (char *)fbuf );
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
