#include "lua-db.h"

#if 0
static const struct luaL_Reg libhypno[] = {
	{ "sql", lua_execdb },
	{ NULL, NULL }
};


int luaopen_libhypno( lua_State *L ) {
	luaL_newlib( L, libhypno );
	return 1;
}
#endif


//Wraps a nice standard return set for Lua values 
int lua_end ( lua_State *L, int status, const char *errmsg ) {
	lua_newtable( L );
	lua_pushstring( L, "status" );
	lua_pushboolean( L, status );
	lua_settable( L, 1 );

#if 1
	//Is it worth it to record the time all the time?
	const char timefmt[] = "Executed %a, %d %b %Y %H:%M:%S %Z"; 
	char timestr[256] = {0};
	time_t t = time( NULL );
	struct tm *tmp = localtime( &t );
	if ( !tmp || !strftime( timestr, sizeof(timestr), timefmt, tmp ) ) {
		;
	}
	else {
		lua_pushstring( L, "time" );
		lua_pushstring( L, timestr );
		lua_settable( L, 1 );
	}
#endif

	//the value on the other side needs to be something or other
	if ( !status ) {
		lua_pushstring( L, "results" );
		lua_pushboolean( L, 0 );
		lua_settable( L, 1 );
		lua_pushstring( L, "error" );
		lua_pushstring( L, errmsg );
		lua_settable( L, 1 );
	}
	return 1;
}


int lua_execdb( lua_State *L ) {
	zTable *t;
	sqlite3 *ptr;
	char err[ 2048 ] = {0};
	const char *db_name = lua_tostring( L, 1 );
	const char *db_query = lua_tostring( L, 2 );
#if 0
	fprintf( stderr, "name = %s\n", db_name );
	fprintf( stderr, "query = %s\n", db_query );
#endif
	lua_pop(L, 2);	

	//Try opening something and doing a query
	if ( !( ptr = db_open( db_name, err, sizeof(err) )) ) {
		//This is an error condition and needs to go back
		return lua_end( L, 0, err );
	}

	if ( !( t = db_exec( ptr, db_query, NULL, err, sizeof(err) ) ) ) {
		return lua_end( L, 0, err );
	}

	if ( !db_close( (void **)&ptr, err, sizeof(err) ) ) {
		return lua_end( L, 0, err );
	}

	//Add a table with 'status', 'results', 'time' and something else...
	lua_end( L, 1, NULL );
	lua_pushstring( L, "results" );
	lua_newtable( L );
	if ( !table_to_lua( L, 3, t ) ) {
		//free the table
		//something else will probably happen
		return lua_end( L, 0, "Could not add result set to Lua.\n" );
	}
	lua_settable( L, 1 );
	lt_free( t );
	free( t );	
	return 1;
}
