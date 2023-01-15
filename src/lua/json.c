/* -------------------------------------------- * 
json.c 
=========

JSON deserialization / serialization 


LICENSE
-------
Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
See LICENSE in the top-level directory for more information.


### send ###

### failure ###

### success ###

 * -------------------------------------------- */
#include "json.h"

//You can do this similarly to before...
#ifdef JSON_MAX_DEPTH
 #undef JSON_MAX_DEPTH
 #define JSON_MAX_DEPTH 50 
#endif

int json_decode ( lua_State *L ) {
	luaL_checkstring( L, 1 );

	char *json, *cjson, err[ 1024 ] = {0};
	int cjson_len = 0;
	zTable *t = NULL;

	if ( !( json = ( char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at json_decode()" );
	}

	lua_pop( L, 1 );

#if 0
	if ( !( t = zjson_decode( json, strlen( json ), err, sizeof( err ) ) ) ) {
		return luaL_error( L, "Failed to deserialize JSON at json_decode(): %s", err );
	}
#else
	if ( !( cjson = zjson_compress( json, strlen( json ), &cjson_len ) ) ) {
		fprintf( stderr, "Failed to compress JSON at zjson_compress(): %s", err );
		return 0;
	}

	write( 2, cjson, cjson_len );

#endif
#if 0
	if ( !ztable_to_lua( L, t ) ) {
		return luaL_error( L, "Failed to convert to Lua." );
	}

	//lt_kfdump( t, 2 );
	lua_stackdump( L );
	lt_free( t ), free( t );
#endif
	return 0;
}


int json_encode ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );
	char * src = NULL, err[ 1024 ] = {0};
	zTable *zt = NULL; 
	int count = 0;

	//We can approximate a sensible amount of slots based on the count
	if ( !( count = lua_count( L, 1 ) ) ) {
		//TODO: Is [] or {} correct?
		lua_pushstring( L, "{}" );
		return 1;
	}

	//Check if the table failed to be created
	if ( !( zt = lt_make( count * 2 ) ) ) {
		return luaL_error( L, "no space to allocate for JSON conversion." ); 
	}

	//Then convert
	if ( !lua_to_ztable( L, 1, zt ) ) {
		return luaL_error( L, "conversion to binary structure failed." ); 
	}

	//Pop the value and encode the above ztable if that did not fail
	if ( !( src = zjson_encode( zt, err, sizeof( err ) ) ) ) {
		return luaL_error( L, "Encoding failed: %s", err );
	}

	lua_pop( L, 1 );
	lua_pushstring( L, src );
	lt_free( zt ), free( zt );
	free( src );
	return 1;
}


int json_check ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );
	return 0;
}


int json_load ( lua_State *L ) {
	//Get a filename 
	luaL_checkstring( L, 1 );

	//Find a file and load it and dump it
	char * file = NULL;
	if ( !( file = ( char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at json_load()" );
	}

	lua_pop(L , 1);
	struct stat sb = {0};
	if ( stat( file, &sb ) == -1 ) {
		return luaL_error( L, "stat failed at json_load(): %s", strerror( errno ) );
	}

	int fd = 0;
	if ( ( fd = open( file, O_RDWR ) ) == -1 ) {
		return luaL_error( L, "open failed at json_load(): %s", strerror( errno ) );
	} 

	char * content = malloc( sb.st_size + 1 );
	memset( content, 0, sb.st_size + 1 );
	if ( read( fd, content, sb.st_size ) == -1 )	{
		return luaL_error( L, "read failed at json_load(): %s", strerror( errno ) );
	} 

	//Then do the decoding dance and (try) to turn it into a table
	char * cmp = NULL;
	int cmplen = 0;
	if ( !( cmp = zjson_compress( content, sb.st_size, &cmplen ) ) ) {
		return luaL_error( L, "compression failed at json_load()" );
	}

	//write( 2, cmp, cmplen );
	struct mjson ** mjson = NULL;
	char err[ 1024 ] = {0};
	if ( !( mjson = zjson_decode2( cmp, cmplen, err, sizeof( err )) ) ) {
		return luaL_error( L, "decoding failed at json_load()" );
	}

	zTable * jt = NULL;
	if ( !( jt = zjson_to_ztable( mjson, NULL, err, sizeof( err ) ) ) ) {
		return luaL_error( L, "zjson to ztable failed at json_load()" );
	}

	//lt_reset( jt );
	lt_kfdump( jt, 2 );


	if ( !ztable_to_lua( L, jt ) ) {
		return luaL_error( L, "ztable to Lua failed at json_load()" );
	}

	return 1;
}


#ifdef LUA_LOPEN
int luaopen_json (lua_State *L) {
	//Clear all values from the stack...
	//TODO: Why is anything on there in the first place?
	int count = lua_gettop( L );
	lua_pop( L, count );
	lua_newtable( L );
	lua_pushstring( L, "encode" );
	lua_pushcfunction( L, json_encode );
	lua_settable( L, 1 );
	lua_pushstring( L, "decode" );
	lua_pushcfunction( L, json_decode );
	lua_settable( L, 1 );
	lua_pushstring( L, "load" );
	lua_pushcfunction( L, json_load );
	lua_settable( L, 1 );
#if 0
	lua_setglobal( L, "json" );
#endif
	return 1;
}
#endif


struct luaL_Reg json_set[] = {
 	{ "decode", json_decode }
,	{ "encode", json_encode }
,	{ NULL }
};

