/* ------------------------------------------- * 
 * json.c 
 * =========
 * 
 * Summary 
 * -------
 * JSON deserialization / serialization 
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * No entries yet.
 *
 * ------------------------------------------- */
#include "json.h"

//You can do this similarly to before...
#ifdef JSON_MAX_DEPTH
 #undef JSON_MAX_DEPTH
 #define JSON_MAX_DEPTH 50 
#endif

int json_decode ( lua_State *L ) {
	luaL_checkstring( L, 1 );

	char *json, err[ 1024 ] = {0};
	zTable *t = NULL;

	if ( !( json = ( char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at json_decode()" );
	}

	lua_pop( L, 1 );

	if ( !( t = zjson_decode( json, strlen( json ), err, sizeof( err ) ) ) ) {
		return luaL_error( L, "Failed to deserialize JSON at json_decode(): %s", err );
	}

	if ( !ztable_to_lua( L, t ) ) {
		return luaL_error( L, "Failed to convert to Lua." );
	}

	lt_free( t ), free( t );
	return 1;
}


int json_encode ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );
	char * src = NULL, err[ 1024 ] = {0};
	zTable *zt = NULL;
	//lua to ztable
	if ( !lua_to_ztable( L, 1, zt ) ) {
		return luaL_error( L, "conversion to binary structure failed." ); 
	}

	lua_pop( L, 1 );

	//encode said table if that did not fail
	if ( !( src = zjson_encode( zt, err, sizeof( err ) ) ) ) {
		return luaL_error( L, "Encoding failed." );
	}

	lua_pushstring( L, src );
	return 1;
}


int json_check ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );
	return 0;
}


struct luaL_Reg json_set[] = {
 	{ "decode", json_decode }
,	{ "encode", json_encode }
,	{ NULL }
};

