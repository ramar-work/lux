/* ---------------------------------- * 
testsql.c
-------------

SQL tests.

- router
- maybe filename scopes 
------------------------------------- */

#include "bridge.h"


int main ( int argc, char *argv[] )
{
	lua_State *L = luaL_newstate();
	luaL_openlibs( L );
	GLOBAL_FN( lua_db, "exec" );

	if ( luaL_dostring( L, "print( 'chunky bacon' );" ) != LUA_OK )
	{
		return err( 1, "Nigga you failed at print()...\n" );
	}

	//if ( luaL_dostring( L, "exec( \"shib.db\" \"CREATE TABLE IF NOT EXISTS shib ( wish int PRIMARY KEY, goal text NOT NULL )\" )" ) != LUA_OK )
	//if ( luaL_dostring( L, "exec( 'shib.db', 'CREATE TABLE shib ( wish PRIMARY KEY, goal text NOT NULL ))" ) != LUA_OK )
	if ( luaL_dostring( L, "print( exec )" ) )
		return err( 1, "There is no function...\n" );

	if ( luaL_dostring( L, "print( exec( 'shib.db', '   CREATE TABLE shib ( wish int PRIMARY KEY, goal text NOT NULL )' ))" ) != LUA_OK )
		return err( 1, "Nigga you failed at db...\n" );

#if 1
	//Load a query via Lua's functionality
	if (luaL_loadstring( L, "exec( \"shib.db\" \"INSERT INTO shib VALUES ( NULL, 'Wiggly puff.' )\" )" ) )
	{
		return 0;
	}

	//Load a query via Lua's functionality
	if (luaL_loadstring( L, "a = exec( \"shib.db\" \"SELECT * FROM shib;\" )" ) )
	{
		return 0;
	}
#endif

	//Lua is just going to open the db, hit the query, and
	//close it again, that's why SQLite was written...

	//Render it according to something

	return 0;
}
