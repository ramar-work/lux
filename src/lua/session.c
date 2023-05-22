/* -------------------------------------------- * 
session
=======

Session primitives.
 
Usage
-----
Session...

### start ###

### check ###

### stop ###

### set ###

### get ###


LICENSE
-------
Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
See LICENSE in the top-level directory for more information.

See LICENSE for licensing information.
 * -------------------------------------------- */
#include "session.h"

static const char sdbpath[] = 
	SESSION_DB_PATH;

static const char check_for_session_table[] = 
	"SELECT name FROM sqlite_master WHERE type='table' AND name='session'";

static const char create_session_table[] = 
"CREATE TABLE session ("
"		id INTEGER PRIMARY KEY AUTOINCREMENT"
",	sid TEXT"
",	matchid TEXT"
",	sstart INTEGER"
",	send INTEGER	)"
;

static const char create_session_values_table[] = 
"CREATE TABLE session_values ("
"		id INTEGER PRIMARY KEY AUTOINCREMENT"
",	sid INTEGER"
",	key TEXT"
",	value TEXT )"
;

const char insert_session_values[] = 
"INSERT INTO session"
"		(id, sid, matchid, sstart, send)" 
"VALUES"
"		(NULL, :sid, :mid, :start, :end)"
;


int init( const char *f ) {
	//Open the in-memory connection if there is one
	//Create the session table
	//Create the session values table
	return 0;
}


int check ( lua_State *L ) {
	//Check the scope for a key (can be configured)
	//
	return 0;
}


int start ( lua_State *L ) {
	zTable tt, *t, *results;
	zdb_t zdb = { 0 }; 
	int zdb_arglen = 0, pos = -1;
	zdbv_t **zdbbind = NULL;
	char *token = NULL;
	char *path = NULL;
	char err[ 1024 ] = { 0 };
	char conn[ PATH_MAX ] = {0}; 
	char randid[32] = {0};
	char start[ 128 ] = {0}, end[ 128 ] = {0};
	int expiry = 0; // Hardcoded for 5 min for now...
	struct timespec tm = { 0 };
	const unsigned char word[] = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ" 
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
	;

	//Initialize a table for arguments
	luaL_checktype( L, 1, LUA_TTABLE );
	t = &tt;
	lt_init( t, NULL, 32 ); 

	//Convert to C table for slightly easier key extraction	
	if ( !lua_to_ztable( L, 1, t ) || !lt_lock( t ) ) {
		lt_free( t );
		return luaL_error( L, "Failed to convert to zTable" );
	}

	//Get rid of the table on the stack
	lua_pop( L, 1 ); 

	//Check for expiry and token
	if ( ( pos = lt_geti( t, "expiry" ) ) > -1 ) {
		expiry = lt_int_at( t, pos );
	}

	//Check for a path variable
	if ( ( pos = lt_geti( t, "path" ) ) > -1 ) {
		path = lt_text_at( t, pos );
	}

	if ( ( pos = lt_geti( t, "token" ) ) > -1 )
		token = lt_text_at( t, pos );
	else {
		lt_free( t );
		return luaL_error( L, "session - no token specified." );
	}

	//Get the sid name (if the user has specified one)
	lua_getglobal( L, "sid" ); 
	const char *sid = ( lua_isnil( L, -1 ) ) ? "sid" : lua_tostring( L, -1 );
	lua_pop( L, 1 ); 

	//Get the shadow path if there is one & create a database connection string
	lua_getglobal( L, "shadow" ); 
	if ( lua_isnil( L, -1 ) )
		lua_pop( L, 1 ); 
	else {
		//Translate the connection to the right path
		char shadow[ 2048 ] = {0};
		snprintf( shadow, sizeof( shadow ), "%s", lua_tostring( L, -1 ) );
		snprintf( conn, sizeof( conn ), "%s/db/%s", shadow, ".session.db" );
		lua_pop( L, 1 );
	}

	//Generate an unique ID
	if ( clock_gettime( CLOCK_REALTIME, &tm ) == -1 ) {
		return luaL_error( L, "Get time failure." );
	}

	srand( tm.tv_nsec );
	for ( int i = 0; i < sizeof( randid ) - 1; i++ ) {
		randid[ i ] = word[ rand() % ( sizeof( word ) - 1 ) ]; 
	}

	//Open a database handle
	if ( !zdb_open( &zdb, conn, ZDB_SQLITE ) ) { 
		return luaL_error( L, "Connection error." );
	}

	//Check if the session table exists.
	if ( !zdb_exec( &zdb, check_for_session_table, NULL ) ) {
		return luaL_error( L, "SQL session table check failed - error: %s", zdb.err );
	}

	//Create one if it doesn't
	if ( zdb.rows == 0 ) {
		if ( !zdb_exec( &zdb, create_session_table, NULL ) ) {
			return luaL_error( L, "Session table creation failed - error: %s", zdb.err );
		}

		if ( !zdb_exec( &zdb, create_session_values_table, NULL ) ) {
			return luaL_error( L, "Session values table creation failed - error: %s", zdb.err );
		}
	}

	//Generate start and end dates
	if ( clock_gettime( CLOCK_REALTIME, &tm ) == -1 ) {
		return luaL_error( L, "Get time failure." );
	}

	//Add a new ID
	zdbv_t * sid_v = malloc( sizeof( zdbv_t ) );
	memset( sid_v, 0, sizeof( zdbv_t ) );
	sid_v->field = zhttp_dupstr( ":sid" );
	sid_v->value = zhttp_dupstr( randid );
	sid_v->len = strlen( randid );
	add_item( &zdbbind, sid_v, zdbv_t *, &zdb_arglen );

	//Bind the start date
	zdbv_t * sv = malloc( sizeof( zdbv_t ) );
	memset( sv, 0, sizeof( zdbv_t ) );
	snprintf( start, sizeof( start ), "%ld", tm.tv_sec );
	sv->field = zhttp_dupstr( ":start" );
	sv->value = zhttp_dupstr( start );
	sv->len = strlen( start );
	add_item( &zdbbind, sv, zdbv_t *, &zdb_arglen );

	//Bind the end date
	zdbv_t * ev = malloc( sizeof( zdbv_t ) );
	memset( ev, 0, sizeof( zdbv_t ) );
	snprintf( end, sizeof( end ), "%ld", tm.tv_sec + expiry );
	ev->field = zhttp_dupstr( ":end" ); 
	ev->value = zhttp_dupstr( end );  
	ev->len = strlen( end );
	add_item( &zdbbind, ev, zdbv_t *, &zdb_arglen );

	//Bind the match ID (typically a username, but anything 
	//to associate the session value with something) 
	zdbv_t * id_v = malloc( sizeof( zdbv_t ) );
	memset( id_v, 0, sizeof( zdbv_t ) );
	id_v->field = zhttp_dupstr( ":mid" );
	id_v->value = zhttp_dupstr( (char *)token );
	id_v->len = strlen( token );
	add_item( &zdbbind, id_v, zdbv_t *, &zdb_arglen );

	//Add them to the table
	if ( !zdb_exec( &zdb, insert_session_values, zdbbind ) ) {
		return luaL_error( L, "Session value creation failed - error: %s", zdb.err );
	}

	//Always make a token string
	char sidvalue[2048] = {0}, *sidv = sidvalue;
	int svlen = sizeof( sidvalue );
	svlen -= snprintf( sidv, svlen, "%s=%s;", sid, randid );

	if ( expiry ) {
		svlen -= snprintf( &sidv[ svlen ], svlen, "max-age=%d;", expiry );
	}

	if ( path ) {
		svlen -= snprintf( &sidv[ svlen ], svlen, "path=%s;", path );
	}

	//Use set cookie to send a session identifier (name can be changed from config)
	lua_getglobal( L, "response" ); 
	if ( lua_isnil( L, -1 ) ) {
		lua_pop( L, 1 ); 
		lua_newtable( L );
		lua_setstrbool( L, "delay", 1, 1 );
		lua_pushstring( L, "headers" ), lua_newtable( L );
		lua_setstrstr( L, "Set-Cookie", sidv, 3 );
		lua_settable( L, 1 );
		lua_setglobal( L, "response" );
	}
	else {
		//Converting with Lua takes more memory, but is so much easier to do
		zTable tx, *ttx;
		ttx = &tx;
		lt_init( ttx, NULL, 32 ); 

		if ( !lua_to_ztable( L, 1, ttx ) || !lt_lock( ttx ) ) {
			lt_free( ttx );
			return luaL_error( L, "Failed to convert to zTable" );
		}

		if ( lt_geti( ttx, "headers" ) == -1 ) {
			lua_pushstring( L, "headers" ), lua_newtable( L );
			lua_setstrstr( L, "Set-Cookie", sidvalue, 3 );
		}
		else {
			zTable *t1p = lt_copy_by_index( ttx, lt_geti( ttx, "headers" ) );
			lua_pushstring( L, "headers" );
			lua_newtable( L ); // index 3
			lua_setstrstr( L, "Set-Cookie", sidvalue, 3 );
			lt_reset( t1p );
			zKeyval *x = lt_next( t1p );  // Skip the first one
			for ( ; ( x = lt_next( t1p ) ) && x->key.type != ZTABLE_TRM ;  ) {	
				zhValue k = x->key, v = x->value;
				if ( k.type == ZTABLE_TXT ) 
					lua_pushstring( L, k.v.vchar );
				else if ( k.type == ZTABLE_INT )
					lua_pushnumber( L, k.v.vint );
				else {
					FPRINTF( "Got useless key in response.headers\n" );
					continue;
				}

				if ( v.type == ZTABLE_INT )
					lua_pushnumber( L, v.v.vint );				
				else if ( v.type == ZTABLE_FLT )
					lua_pushnumber( L, v.v.vfloat );				
				else if ( v.type == ZTABLE_TXT )
					lua_pushstring( L, v.v.vchar );
				else /* ZTABLE_TRM || ZTABLE_NON || ZTABLE_USR */ {
					// I don't care about any of these...
					FPRINTF( "Got useless value in response.headers\n" );
					continue;
				}
				lua_settable( L, 3 );
			}
			lt_free( t1p );
		}
		lua_settable( L, 1 );
	}

	//We also return the key
	lua_pushstring( L, randid );

	//Close the connection
	zdbv_free( zdbbind );
	zdb_free( &zdb );	
	zdb_close( &zdb );	
	lt_free( t );
	return 1;
}

int stop ( lua_State *L ) {
	//Find the right key (using the sid that we chose)
	return 0;
}

int get ( lua_State *L ) {
	//Get any data associated with the key
	return 0;
}

int set ( lua_State *L ) {
	//Set any data with key
	return 0;
}

int unset ( lua_State *L ) {
	return 0;
}

struct luaL_Reg session_set[] = {
 { "start", start }
#if 0
,{ "check", generate_sha1 }
,{ "stop", generate_sha256 }
,{ "set", generate_sha384 }
#endif
,{ NULL }
};


