/* -------------------------------------------- * 
json.c 
=========

JSON deserialization / serialization 


LICENSE
-------
Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
See LICENSE in the top-level directory for more information.

 * -------------------------------------------- */
#include "json.h"

/**
 * int json_check( lua_State *L )
 * 
 * Decodes a Lua string composed of JSON to Lua table.
 *
 */ 
int json_decode ( lua_State *L ) {
	char *src = NULL, *cmp = NULL, err[ 1024 ] = {0};
	int cmplen = 0;
	ztable_t *t = NULL;
	struct mjson **zjson = NULL;

	if ( !lua_isstring( L, 1 ) || !( src = ( char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at json_decode()" );
	}

	lua_pop( L, 1 );

	if ( !zjson_check_syntax( src, strlen( src ), err, sizeof( err ) ) ) {
		return luaL_error( L, "JSON syntax check failed: %s.", err );
	}

	if ( !( cmp = zjson_compress( src, strlen( src ), &cmplen ) ) ) {
		return luaL_error( L, "Failed to compress JSON at zjson_compress()." );
	}

	if ( !( zjson = zjson_decode( cmp, cmplen - 1, err, sizeof( err ) ) ) ) {
		return luaL_error( L, "Failed to deserialize JSON at json_decode(): %s", err );
	}

	//Create a blank table and return it here in case of an empty object or array
	if ( !zjson_has_real_values( zjson ) ) {
		zjson_free( zjson ), free( cmp );
		lua_newtable( L );
		return 1;
	}

	if ( !( t = zjson_to_ztable( zjson, err, sizeof( err ) ) ) ) {
		zjson_free( zjson ), free( cmp );
		return luaL_error( L, "Failed to deserialize table to JSON structures: %s", err );
	}	
	
	if ( !ztable_to_lua( L, t ) ) {
		zjson_free( zjson ), lt_free( t ), free( t ), free( cmp );
		return luaL_error( L, "Failed to convert to Lua." );
	}

	zjson_free( zjson ), lt_free( t ), free( t ), free( cmp );
	return 1;
}


/**
 * int json_encode( lua_State *L )
 *
 * Encodes a Lua table to JSON string.
 *
 */ 
int json_encode ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );
	char * src = NULL, err[ 1024 ] = {0};
	zTable *zt = NULL; 
	int count = 0;
	struct mjson **mjson = NULL;

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
		lt_free( zt ), free( zt );
		return luaL_error( L, "conversion to binary structure failed." ); 
	}

	lua_pop( L, 1 );

	if ( !( mjson = ztable_to_zjson( zt, err, sizeof( err ) ) ) ) {
		lt_free( zt ), free( zt );
		return luaL_error( L, "Encoding failed: %s", err );
	}

	if ( !( src = zjson_stringify( mjson, err, sizeof( err ) ) ) ) {
		zjson_free( mjson ), lt_free( zt ), free( zt );
		return luaL_error( L, "Encoding failed: %s", err );
	}

	lua_pushstring( L, src );
	free( src ), zjson_free( mjson ), lt_free( zt ), free( zt );
	return 1;
}


/**
 * int json_check( lua_State *L )
 *
 * Performs a syntax check on a JSON string.
 *
 */ 
int json_check ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );
	return 0;
}


/**
 * int json_load( lua_State *L )
 *
 * Loads a file and turns into a Lua table.
 *
 */ 
int json_load ( lua_State *L ) {
	char *file = NULL, *content = NULL, *cmp = NULL;
	char err[ 1024 ] = {0};
	struct stat sb = {0};
	int fd = 0, cmplen = 0;
	struct mjson ** mjson = NULL;
	zTable * jt = NULL;

	//Get the filename
	if ( !lua_isstring( L, 1 ) || !( file = ( char * )lua_tostring( L, 1 ) ) ) {
		return luaL_error( L, "No string specified at json_load()" );
	}

	lua_pop(L , 1);
	if ( stat( file, &sb ) == -1 ) {
		return luaL_error( L, "stat on '%s' failed at json_load(): %s", file, strerror( errno ) );
	}

	if ( ( fd = open( file, O_RDWR ) ) == -1 ) {
		return luaL_error( L, "open failed at json_load(): %s", strerror( errno ) );
	} 

	if ( !( content = malloc( sb.st_size + 1 ) ) || !memset( content, 0, sb.st_size + 1 ) ) {
		return luaL_error( L, "allocation failed at json_load(): %s", strerror( errno ) );
	}
	
	if ( read( fd, content, sb.st_size ) > -1 )
		close( fd );
	else {
		close( fd );
		return luaL_error( L, "read failed at json_load(): %s", strerror( errno ) );
	}

	//Then do the decoding dance and (try) to turn it into a table
	if ( ( cmp = zjson_compress( content, sb.st_size, &cmplen ) ) )
		free( content );
	else {
		free( content );
		return luaL_error( L, "compression failed at json_load()" );
	}

	/**
	 * NOTE: 
   * 'struct mjson **mjson' relies on allocated string 'cmp' as its 
	 * source.  Can't delete this reference until we're done with the
   * list.
   * 
   */
	if ( ( mjson = zjson_decode( cmp, cmplen, err, sizeof( err )) ) )
		0;	
	else {
		free( cmp );
		return luaL_error( L, "decoding failed at json_load()" );
	}

	if ( ( jt = zjson_to_ztable( mjson, err, sizeof( err ) ) ) )
		zjson_free( mjson ), free( cmp );
	else {
		zjson_free( mjson );
		return luaL_error( L, "zjson to ztable failed at json_load()" );
	}
	
	//lt_reset( jt );
	//lt_kfdump( jt, 2 );

	if ( ztable_to_lua( L, jt ) )
		lt_free( jt ), free( jt );
	else {
		lt_reset( jt ), lt_free( jt ), free( jt );
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
,	{ "load", json_load }
,	{ NULL }
};

