/* session.c */
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
	char err[ 1024 ] = { 0 };
	char conn[ PATH_MAX ] = {0}; 
	int expiry = 300; // Hardcoded for 5 min for now...
	char start[ 128 ] = {0}, end[ 128 ] = {0};
	struct timespec tm = { 0 };
	char rid[32] = {0};
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

	//Check for expiry and token
	if ( ( pos = lt_geti( t, "expiry" ) ) > -1 ) {
		expiry = lt_int_at( t, pos );
	}

	if ( ( pos = lt_geti( t, "token" ) ) > -1 )
		token = lt_text_at( t, pos );
	else {
		lt_free( t );
		return luaL_error( L, "session - no token specified." );
	}

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
	for ( int i = 0; i < sizeof( rid ) - 1; i++ ) {
		rid[ i ] = word[ rand() % ( sizeof( word ) - 1 ) ]; 
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
	sid_v->value = zhttp_dupstr( rid );
	sid_v->len = strlen( rid );
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

	//Use set cookie to send a session identifier (name can be changed from config)
#if 0
	zhttp_add_header
	or
	lua_getglobal( L, "response" );
	lua_?( ... );	
#endif	 

	//Close the connection
	zdbv_free( zdbbind );
	zdb_free( &zdb );	
	zdb_close( &zdb );	
	lt_free( t );
	return 0;
}

int stop ( lua_State *L ) {
	return 0;
}

int get ( lua_State *L ) {
	return 0;
}

int set ( lua_State *L ) {
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


