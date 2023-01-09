/* -------------------------------------------- * 
db
==

Database primitives for Lua


LICENSE
-------
See LICENSE in the top-level directory for more information.


Usage
-----
Database does something weird.


### exec ###

How to use db.exec.

It's very complicated.

A bunch of notes on how to use this thing.

```
Yeah
```
 * -------------------------------------------- */
#include "db.h"

static const char *engines[] = {
	"sqlite3://"
,	"mysql://"
,	"postgres://"
, NULL
};



//Accepts three arguments { db, [query,file], bind_args }
int db_exec ( lua_State *L ) {
	//Define everything at the top
	const char *connstr = NULL, *query = NULL;
	char file[ 1024 ] = {0}, conn[ 1024 ] = {0}, shadow[ 1024 ] = {0};
	int pos, len = 0, inc = 0;
	char err[ 1024 ] = { 0 };
	zTable tt, *t, *results;
	zdb_t zdb = { 0 }; 
	zdbv_t **zdbbind = NULL;

	//Check for arguments...
	luaL_checktype( L, 1, LUA_TTABLE );

	//Within this table need to exist a few keys
	//TODO: Doing this statically will save time, energy and memory
	t = &tt;
	lt_init( t, NULL, 32 ); 

	//Convert to C table for slightly easier key extraction	
	if ( !lua_to_ztable( L, 1, t ) || !lt_lock( t ) ) {
		lt_free( t );
		return luaL_error( L, "Failed to convert to zTable" );
	}

	//After we're done getting the table, that should be it
	lua_pop( L, 1 );	

	//Check for needed args, starting with dbname (or conn str, whatever)
	//Then we have to initialize the underlying lib

	//Db should point to a real file
	//Conn should be what we use
	if ( ( pos = lt_geti( t, "conn" ) ) == -1 ) {
		lt_free( t );
		return luaL_error( L, "Connection unspecified." );
	}
	else {
		//Check for a valid engine (since right now it's only sqlite)
		const char *dbstr = lt_text_at( t, pos );
		for ( const char **k = engines; *k; k++ ) {
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
	//TODO: Eventually, this should be replaced with lua_retglobal
	lua_getglobal( L, "shadow" ); 
	if ( lua_isnil( L, -1 ) )
		lua_pop( L, 1 ); 
	else {
		//Translate the connection to the right path
		snprintf( shadow, sizeof( shadow ), "%s", lua_tostring( L, -1 ) );
		snprintf( conn, sizeof( conn ), "%s/%s", shadow, connstr );
		lua_pop( L, 1 );
	}

	//Then get the query
	if ( ( pos = lt_geti( t, "string" ) ) > -1 )
		query = lt_text_at( t, pos );
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

	#if 0
	//Extract a type as well... 
	if ( ( pos = lt_geti( t, "memory" ) ) > -1 ) {
		char *type = lt_text_at( t, pos );
		if ( !type ) {
			return luaL_error( L, "type key was specified, but was not a string\n" );
		}

		if ( strcmp( type, "true" ) == 0 ) {
			//This is incredibly difficult to do with just memory...	
		}	
		else if ( strcmp( type, "false" ) == 0 ) {
		}
		else {
			return luaL_error( L, "Got invalid value for 'type' key.\n" );
		}
		lt_dump( t );
		exit( 0 );
	}	 
	#endif

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

#if 0
//Checks for the existence of a table
int db_check ( lua_State *L ) {
	//
	return 1;
}
#endif

struct luaL_Reg db_set[] = {
 	{ "exec", db_exec }
#if 0
,	{ "check", db_check }
#endif
,	{ NULL }
};


