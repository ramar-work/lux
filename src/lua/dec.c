/* -------------------------------------------- * 
 * dec.c 
 * =====
 * 
 * Summary 
 * -------
 * Popular encoding routines
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
 * -------------------------------------------- */
#include "dec.h"

static char b64r[ 256 ] = {
	-3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


static unsigned int raw_base64_decode( unsigned char *in, unsigned char *out, int *err ) {
	unsigned int x, result = 0;
	unsigned char buf[ 3 ], *p = in, pad = 0;

	while ( !pad ) {
		switch ( ( x = b64r[ *p++ ] ) ) {
			case -3:
				if ( ( ( p - 1 ) - in) % 4 ) {
					*err = 1;
				}
				return result;
		
			case -2:
				if ( ( ( p - 1 ) - in ) % 4 < 2 ) {
					*err = 1;
					return result;
				}
				else if ( (( p - 1 ) - in ) % 4 == 2 ) {
					if ( *p != '=' ) {
						*err = 1;
						return result;
					}
					buf[ 2 ] = 0, pad = 2, result++;
					break;
				}
				else {
					pad = 1, result += 2;
					break;
				}
				return result;

			case -1:
				break;

			default:
				switch ( (( p - 1 ) - in ) % 4 ) {
					case 0:
						buf[ 0 ] = x << 2;
						break;
					case 1:
						buf[ 0 ] |= (x >> 4), buf[ 1 ] = x << 4;
						break;
					case 2:
						buf[ 1 ] |= (x >> 2), buf[ 2 ] = x << 6;
						break;
					case 3:
						buf[ 2 ] |= x, result += 3;
						for ( x = 0; x < 3 - pad; x++ ) {
							*out++ = buf[x];
						}
						break;
				}
				break;
		}
	}
	for ( x = 0; x < 3 - pad; x++ ) {
		*out++ = buf[ x ];
	}
	return result;
}


//TODO: Decoding can be HEAVILY refactored.
//#1 - We can do the replacemnets in place because we process 3 bytes for every 1 byte we want to write (meaning I don't need malloc within this library)
//#2 - Combine spc_base64_decode and the above function
unsigned char *spc_base64_decode( char *buf, int *len ) {
	unsigned char *ob;
	int err = 0;

	if ( !( ob = malloc( 3 * ( strlen( buf ) / 4 + 1 ) ) ) ) {
		*len = 0;
		return NULL;
	}

	*len = raw_base64_decode( ( unsigned char * )buf, ob, &err );

	if ( err ) {
		free( ob );
		*len = 0, ob = NULL;
	}	

	return ob;
}


int base64_decode ( lua_State *L ) {
	luaL_checkstring( L, 1 );
	unsigned char *block = NULL, *str = NULL;
	int len = 0;

	if ( !( str = ( unsigned char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at dec.base64()" );
	}

	lua_pop( L, 1 );

	if ( !( block = spc_base64_decode( (char *)str, &len ) ) ) {
		return luaL_error( L, "Error base64 decoding block." );
	}

	lua_newtable( L );
	lua_setstrbin( L, "value", block, len, 1 );
	lua_setstrint( L, "size", len, 1 );
	free( block );
	return 1;
}



struct luaL_Reg dec_set[] = {
 	{ "base64", base64_decode }
,	{ NULL }
};
