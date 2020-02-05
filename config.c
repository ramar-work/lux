//Compile me with: 
//gcc -ldl -llua -DSQROOGE_H -o config vendor/single.o config.c && ./config
#include "vendor/single.h"
#include "luabind.h"


int main (int argc, char *argv[]) {

	int lerr;
	lua_State *L = luaL_newstate();
	const char *f = "www/def.lua";
	char fileerr[2048] = {0};
	
#if 1
//START lua_load_file	
	//Load route table file
	fprintf( stderr, "Attempting to parse file: %s\n", f );
	if (( lerr = luaL_loadfile( L, f )) != LUA_OK ) { 
		int errlen = 0;
		if ( lerr == LUA_ERRSYNTAX )
			errlen = snprintf( fileerr, sizeof(fileerr), "Syntax error at file: %s", f );
		else if ( lerr == LUA_ERRMEM )
			errlen = snprintf( fileerr, sizeof(fileerr), "Memory allocation error at file: %s", f );
		else if ( lerr == LUA_ERRGCMM )
			errlen = snprintf( fileerr, sizeof(fileerr), "GC meta-method error at file: %s", f );
		else if ( lerr == LUA_ERRFILE ) {
			errlen = snprintf( fileerr, sizeof(fileerr), "File access error at: %s", f );
		}
		
		fprintf(stderr, "LUA LOAD ERROR: %s, %s", fileerr, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		exit( 1 );
	}
	fprintf( stderr, "SUCCESS!\n" );

	//Then execute
	fprintf( stderr, "Attempting to execute file: %s\n", f );
	if (( lerr = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
		if ( lerr == LUA_ERRRUN ) 
			snprintf( fileerr, sizeof(fileerr), "Runtime error at: %s", f );
		else if ( lerr == LUA_ERRMEM ) 
			snprintf( fileerr, sizeof(fileerr), "Memory allocation error at file: %s", f );
		else if ( lerr == LUA_ERRERR ) 
			snprintf( fileerr, sizeof(fileerr), "Error while running message handler: %s", f );
		else if ( lerr == LUA_ERRGCMM ) {
			snprintf( fileerr, sizeof(fileerr), "Error while runnig __gc metamethod at: %s", f );
		}

		fprintf(stderr, "LUA EXEC ERROR: %s, %s", fileerr, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		exit( 1 );
	}
	fprintf( stderr, "SUCCESS!\n" );
//END lua_load_file	
#endif

	//The stack SHOULD contain the set of routes and handlers...
	lua_stackdump( L );

	//There is more than a route table here.
	//db config, and much more is probably going to be here...

	//Build static route list?
	//1. Use inherent hash table prims (map /route to [route.0.etc])
	//	This method is MUCH faster...  but not as flexible
	//	(can't reallly support wildcards, eithers or parameters)
	//	(I would need to resolve part of the hash, and that's not really helpful)
	//2. Create full route strings out of each of the keys
	//   Doing this will allow me to make a model and view (and whatever else)
	//   out of a route table
	//So it will look something like:
	//	- /route/x -> { model: ..., view: ..., etc: ... }
	//Then I can just load from there provided that a matching route was found 
	return 0;
}
