/*
 *
 * depthtest.c
 *
 * Checks stackdump function on really large tables
 * (specifically really deep tables).
 *
 */
 
#include "../bridge.h"

int main (int argc, char *argv[])
{
	lua_State *L = luaL_newstate();
	struct stat sb = {0};
	char err[2048] = { 0 };
	const char *files[] = {
		"tests/depth-data/dd1.lua", 
		"tests/depth-data/dd2.lua", 
		"tests/depth-data/dd3.lua", 
		"tests/depth-data/dd4.lua", 
		"tests/depth-data/dd5.lua", 
	};

	if ( !L )
		err( 1, "Failed to initialize Lua\n" );

	if ( stat( *files, &sb ) == -1 )
		err( 2, "Looks like the file '%s' does not exist.  Please run `make depth` to generate the test files...\n", *files );

	for ( int n=0; n < sizeof (files)/sizeof(char *); n++ ) 
	{
		if ( !lua_load_file( L, files[n], err ) )
			err( 1, "Failed to load file %s\n", f );
		lua_stackdump( L );

		//Clear the stack after each run
		lua_settop( L, 0 );
	}

	return 1;
}
