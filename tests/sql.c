/* ---------------------------------- * 
* sql.c
* -------------
* 
* The things that should be tested for are:
* - compound queries work 'SELECT ...; INSERT...;'
* - OR
* - disable compound queries and die when you see ';'
* 
* - see a big set of results on the stack
* - binding, this will be incredibly annoying without it
* 	numeric is not reliable with Lua, so text binding must be done
* - tables should just work
* - crashes and shit....
* - check for garbage collected results (can't imagine this if the session is still there... but over high use it could be)
------------------------------------- */

#include "../bridge.h"

#define STRING \
	"print( 'chunky bacon' )\n" \
	"print( 'exec is ' .. exec )\n" \
	"a = exec( 'tests/ges.db', 'SELECT * FROM general_election LIMIT 10;' )\n" \
	"print( a )\n" \
	"for k,v in pairs( a.db ) do print( k,v ) end\n" 

Option options[] = {
	{ "-t", "--test",     "Proceed with the tests." },
	{ "-q", "--query",    "A query for the database (use @ to denote a file).", 's' },
	{ "-d", "--database", "Which database to use?", 's' },
	{ "-l", "--eval",     "Evaluate a string of Lua code (or use @ to denote a file).", 's' },
	{ NULL, NULL, .sentinel = 1 }
};


int main ( int argc, char *argv[] )
{
	lua_State *L = luaL_newstate();
	luaL_openlibs( L );
	GLOBAL_FN( lua_db, "exec" );

	if ( argc <= 1 )
		opt_usage( options, argv[0], "Nothing to do.", 1 );
	else {
		opt_eval( options, argc, argv );
	}

	//char *ni  = OPT_IF( options, "--query", s, <val> )
	//expands to 
	//char *ni  = opt_set( options, "--query" ) ? opt_get( options, "--query" ).s : <val>
	char *query = opt_get( options, "--query" ).s;
	char *db    = opt_get( options, "--database" ).s;
	char *eval  = opt_get( options, "--eval" ).s;

	if ( !L )
		return err( 1, "Failed to allocate Lua!\n" );

	if ( luaL_dostring( L, "print( 'chunky bacon' );" ) != LUA_OK )
		return err( 1, "Failed to print string()...\n" );

	if ( luaL_dostring( L, "print( exec )" ) != LUA_OK )
		return err( 1, "There is no function...\n" );

	//Load a query via Lua's functionality
	if ( luaL_dostring( L, "a = exec( 'tests/ges.db', 'SELECT * FROM general_election LIMIT 10' )" ) != LUA_OK )
		return err( 1, "Failed to run query...\n" );

	//Rendering is still handled in C world, so output the data here.
	//lua_to_table( L, 1 );		
	//if ( luaL_dostring( L, "for k,v in a do print( k,v ) end" ) != LUA_OK )
	if ( luaL_dostring( L, "print( a )" ) != LUA_OK )
		return err( 1, "Failed to see type of a...\n" );

	if ( luaL_dostring( L, "for k,v in pairs( a.db ) do print( '['..k..']', v.votes, v.party ) end\n" ) != LUA_OK )
		return err( 1, "Failed to run loop code through a.db...\n" );

	return 0;
}
