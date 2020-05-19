#include "lua-db.h"

static const struct luaL_Reg libhypno[] = {
	{ "sql", lua_execdb },
	{ NULL, NULL }
};


int luaopen_libhypno( lua_State *L ) {
	luaL_newlib( L, libhypno );
	return 1;
}


void lua_exception ( lua_State *L ) {
	
}


int lua_execdb( lua_State *L ) {
#if 1
	const char *string = lua_tostring( L, 1 );
	fprintf( stderr, "string = %s\n", string );
	lua_pop(L,1);	
#endif

#if 0
	//Try opening something and doing a query
	char err[ 2048 ] = {0};
	sqlite3 *ptr = db_open( "the.db", err, sizeof(err) );
	if ( !ptr ) {
		//This is an error condition and needs to go back
		return 0;
	}
#endif

	//Add a table with 'status', 'results', 'time' and something else...
	lua_newtable( L );
	lua_pushstring( L, "status" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );

	lua_pushstring( L, "results" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );

	lua_pushstring( L, "time" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );
	return 1;
}
