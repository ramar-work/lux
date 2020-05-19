#include "lua-db.h"

static const struct luaL_Reg libhypno[] = {
	{ "sql", lua_execdb },
	{ NULL, NULL }
};


int luaopen_libhypno( lua_State *L ) {
	luaL_newlib( L, libhypno );
	return 1;
}


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
	if ( status ) {
		//You'll push whatever you're supposed to push here...
		lua_pushstring( L, "results" );
		lua_pushboolean( L, 1 );
		lua_settable( L, 1 );
	}
	else {
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
#if 1
	const char *string = lua_tostring( L, 1 );
	fprintf( stderr, "string = %s\n", string );
	lua_pop(L,1);	
#endif

#if 1
	//Try opening something and doing a query
	char err[ 2048 ] = {0};
	sqlite3 *ptr = db_open( "the.db", err, sizeof(err) );
	if ( !ptr ) {
		//This is an error condition and needs to go back
		return lua_end( L, 0, err );
	}
#endif

	//Add a table with 'status', 'results', 'time' and something else...
	return lua_end( L, 1, NULL );
}
