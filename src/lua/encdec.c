/* ------------------------------------------- * 
 * encdec.c 
 * ========
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
 * ------------------------------------------- */
#include "encdec.h"

static char b64[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";



//Calculate a base64 string from an input block
//NOTE: This code is courtesy of 'Secure Programming Cookbook for C and C++'
//with some modifications
char *spc_base64_encode( unsigned char *input, int len ) {
	char *output, *p;
	int i = 0, mod = len % 3, size = ( ( len / 3 ) + ( mod ? 1 : 0 ) ) * 4 + 1; 

	if ( !( p = output = malloc( size ) ) || !memset( p, 0, size ) ) {
		return NULL;
	}

	while ( i < ( len - mod ) ) {
		*p++ = b64[ input[ i++ ] >> 2 ];
		*p++ = b64[ (( input[ i - 1 ] << 4 ) | ( input[ i ] >> 4 )) & 0x3f ];
		*p++ = b64[ (( input[ i ] << 2 ) | ( input[ i + 1 ] >> 6 )) & 0x3f ];
		*p++ = b64[ input[ i + 1 ] & 0x3f ];
		i += 2;
	}

	if ( !mod ) {
		*p = 0;
		return output;
	}
	else {
		*p++ = b64[ input[ i++ ] >> 2 ];
		*p++ = b64[ (( input[ i - 1 ] << 4 ) | ( input[ i ] >> 4 )) & 0x3f ];
		if ( mod == 1 ) {
			*p++ = '='; 
			*p++ = '=';
			*p = 0;
			return output;
		}
		else {
			*p++ = b64[ (input[i] << 2 ) & 0x3f ];
			*p++ = '='; 
			*p = 0;
			return output;
		}
	}
	
	return NULL;	
}



int base64_encode ( lua_State *L ) {
	luaL_checkstring( L, 1 );
	char *block = NULL, *str = NULL;

	if ( !( str = ( char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at enc.base64()" );
	}

	lua_pop( L, 1 );

	if ( !( block = spc_base64_encode( (unsigned char *)str, strlen( str ) ) ) ) {
		return luaL_error( L, "Error base64 encoding block." );
	}

	lua_pushstring( L, block );
	free( block );
	return 1;
}


int base64_decode ( lua_State *L ) {
	return 0;
}

#if 0
int base96_encode ( lua_State *L ) {
	return 0;
}

int base96_decode ( lua_State *L ) {
	return 0;
}

int base128_encode ( lua_State *L ) {
	return 0;
}

int base128_decode ( lua_State *L ) {
	return 0;
}

int hex_encode ( lua_State *L ) {
	return 0;
}

int hex_decode ( lua_State *L ) {
	return 0;
}

int octal_encode ( lua_State *L ) {
	return 0;
}

int octal_decode ( lua_State *L ) {
	return 0;
}
#endif


struct luaL_Reg dec_set[] = {
 	{ NULL }
};

struct luaL_Reg enc_set[] = {
 	{ "base64", base64_encode }
, { NULL }
};
