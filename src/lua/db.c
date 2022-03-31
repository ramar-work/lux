/* ------------------------------------------- * 
 * db.c 
 * ====
 * 
 * Summary 
 * -------
 * Database primitives for Lua
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
#include "db.h"

static const char *engines[] = {
	"sqlite3://"
,	"mysql://"
,	"postgres://"
, NULL
};



//Accepts three arguments { db, [query,file], bind_args }
int db_exec ( lua_State *L ) {
	//Check for arguments...
	luaL_checktype( L, 1, LUA_TTABLE );

	//Within this table need to exist a few keys
#if 1
	char err[ 1024 ] = { 0 };
	zTable tt, *results, *t = &tt;
	lt_init( t, NULL, 32 ); 
#else
	//Doing this statically will save time, energy and memory
#endif

	//Convert to C table for slightly easier key extraction	
	if ( !lua_to_ztable( L, 1, t ) || !lt_lock( t ) ) {
		lt_free( t );
		return luaL_error( L, "Failed to convert to zTable" );
	}

	//lt_lock( t );
#if 0
	//Lock and dump?
	lt_dump( t );
	getchar();
#endif

	//After we're done getting the table, that should be it
	lua_pop( L, 1 );	

	//Check for needed args, starting with dbname (or conn str, whatever)
	const char *connstr = NULL, *query = NULL;
	char file[ 1024 ] = {0}, conn[ 1024 ] = {0}, shadow[ 1024 ] = {0};
	int pos, len = 0, inc = 0;
	//Then we have to initialize the underlying lib
	zdb_t zdb = { 0 }; 
	zdbv_t **zdbbind = NULL;

	//Db should point to a real file
	//Conn should be what we use
	if ( ( pos = lt_geti( t, "conn" ) ) == -1 ) {
		lt_free( t );
		return luaL_error( L, "Connection unspecified." );
	}
	else {
		//Check for a valid engine (since right now it's only sqlite)
		const char **k = engines, *dbstr = lt_text_at( t, pos );
		for ( ; *k; k++ ) {
			int len = strlen( *k );
			if ( strlen( dbstr ) >= len && memcmp( *k, dbstr, len ) == 0 ) {
				connstr = dbstr += len;
				break; 
			}
		}
		if ( !connstr ) {
			char buf[64] = {0}, *i = index( dbstr, ':' );
			memcpy( buf, dbstr, strlen( dbstr ) - ( ( i ) ? strlen( i ) : 0 ) );
			return luaL_error( L, "Unsupported database type: '%s'.", buf );
		}
	}

	//Get the shadow path if there is one
	lua_getglobal( L, "shadow" ); 
#if 0
	if ( lua_isstring( L, -1 ) ) {
#else 
	if ( lua_isnil( L, -1 ) )
		lua_pop( L, 1 ); 
	else {
#endif
		//Translate the connection to the right path
		snprintf( shadow, sizeof( shadow ), "%s", lua_tostring( L, -1 ) );
		snprintf( conn, sizeof( conn ), "%s/%s", shadow, connstr );
		lua_pop( L, 1 );
	}

	//Then get the query
	if ( ( pos = lt_geti( t, "string" ) ) > -1 ) {
		query = lt_text_at( t, pos );
	}
	else if ( ( pos = lt_geti( t, "file" ) ) > -1 ) {
		char *f = NULL;
		if ( ( f = lt_text_at( t, pos ) ) )
			snprintf( file, sizeof( file ), "%s/%s", shadow, f );
		else {
			return luaL_error( L, "file key was specified, but was not a string\n" );
		}

		if ( !( query = (char *)read_file( file, &len, err, sizeof( err ) ) ) ) {
			lt_free( t );
			return luaL_error( L, "Failed to open file %s: %s\n", file, err );
		}
	}
	else {
		lt_free( t );
		return luaL_error( L, "Neither 'string' nor 'file' were present in call to %s", "db_exec" );
	}

	//Finally, handle bind args if there are any
	if ( ( pos = lt_geti( t, "bindargs" ) ) > -1 ) {
		int zlen = 0;
		ztable_t *tt = lt_copy_by_index( t, pos );
		zKeyval *kv = NULL;

		//TODO: This is way more complicated than it should be...
		lt_lock( tt );
		lt_reset( tt );
		lt_next( tt );

		//Loop through everything
		for ( zdbv_t *tv = NULL; ( kv = lt_next( tt ) ) && kv->key.type != ZTABLE_TRM; ) {
			char key[ 128 ] = { 0 };
			//if the key is text, do something, if not do something
			if ( !( tv = malloc( sizeof( zdbv_t ) ) ) ) {
				lt_free( t ), lt_free( tt );
				return luaL_error( L, "Allocation failure." );
			}
		
			tv->field = NULL, tv->value = NULL;	
			//if the key is text, do something, if not do something
			if ( kv->key.type == ZTABLE_TXT ) {
				snprintf( key, sizeof(key) - 1, ":%s", kv->key.v.vchar );	
				tv->field = zhttp_dupstr( key );
			}
		#if 0
			else if ( kv->key.type == ZTABLE_INT ) {
				snprintf( key, sizeof(key) - 1, ":%d", kv->key.v.vint );
				tv->field = zhttp_dupstr( key );
			}
		#endif

			if ( kv->value.type == ZTABLE_TBL ) {
				fprintf( stderr, "you are a table...\n" );	
			}

			if ( kv->value.type == ZTABLE_TXT ) {
				if ( !kv->value.v.vchar ) {
					tv->value = zhttp_dupstr( "" ); 
					tv->len = 0;
				}
				else {
					tv->value = zhttp_dupstr( kv->value.v.vchar ); 
					tv->len = strlen( kv->value.v.vchar );
				}
			}
			else if ( kv->value.type == ZTABLE_INT ) {
				char buf[ 64 ] = { 0 };
				int len = snprintf( buf, 63, "%d", kv->value.v.vint );
				tv->value = zhttp_dupstr( buf );
				tv->len = len;
			}
			else {
				//This should result in failure
				lt_free( t ), lt_free( tt );
				return luaL_error( L, "Invalid type of bindarg at key '%s'", key ); 
			}
			add_item( &zdbbind, tv, zdbv_t *, &zlen );
		}

		lt_free( tt ), free( tt );
	}

	//Open or connect to database, run the query and close the connection 
	if ( !zdb_open( &zdb, conn, ZDB_SQLITE ) ) { 
		( len ) ? free( (void *)query ) : 0;
		return luaL_error( L, "Connection error." );
	}

	if ( !zdb_exec( &zdb, query, zdbbind ) ) {
		( len ) ? free( (void *)query ) : 0;
		return luaL_error( L, "SQL execution error: %s", zdb.err );
	}

	//TODO: freeing query in this manner may cause issues when using a string as the argument
	if ( len ) {
		free( (void *)query );
	}

	if ( !( results = zdb_to_ztable( &zdb, "results" ) ) ) {
		( len ) ? free( (void *)query ) : 0;
		return luaL_error( L, "conversion error: %s", zdb.err );
	}

	//It's infinitely easier to write this first...
	if ( !ztable_to_lua( L, results ) ) {
		zdb_close( &zdb ), lt_free( results ), free( results ), lt_free( t );
		return luaL_error( L, "ztable to Lua failed...\n" );
	}

	//Add a status code
	lua_pushstring( L, "status" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );

	//Add a count of rows received (if any)
	lua_pushstring( L, "count" );
	lua_pushnumber( L, zdb.rows );
	lua_settable( L, 1 );

	//Add a count of rows affected (if any)
	lua_pushstring( L, "affected" );
	lua_pushnumber( L, zdb.affected );
	lua_settable( L, 1 );

	//Close the connection and whatnot (seems like a lot...)
	//zdb_dump( &zdb );	
	zdbv_free( zdbbind );
	zdb_free( &zdb );	
	zdb_close( &zdb );	
	lt_free( results );
	free( results );
	lt_free( t );
	return 1;
}

struct luaL_Reg db_set[] = {
 	{ "exec", db_exec }
,	{ NULL }
};


