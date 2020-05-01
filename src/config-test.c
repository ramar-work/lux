#include "luabind.h"
#include "config.h"
#include "hosts.h"
#include "routes.h"

#define TESTDIR "tests/config/"


//Loads some random files
int main (int argc, char *argv[]) {

	const char *files[] = {
		TESTDIR "config.lua",  //global config example (won't have routes)
		TESTDIR "def.lua",		 //local config example (will have routes)
		TESTDIR "config.lua", //local config example (w/ routes, but no complex models)
	};

	for ( int i=0; i < sizeof(files)/sizeof(const char *); i++ ) {

		//Pull some keys from the config file?
		lua_State *L = luaL_newstate();
		const char *f = files[i];
		char err[2048] = {0};
		Table *t = NULL;
		fprintf( stderr, "Attempting to load and parse %s\n", f );

		//Loading a config file is awful difficult...
		if ( !lua_exec_file( L, f, err, sizeof(err) ) ) {
			fprintf( stderr, "%s\n", err );
			return 1;
		}

		if ( !( t = malloc( sizeof(Table) ) ) || !lt_init( t, NULL, 2048 ) ) {
			fprintf( stderr, "Failed to allocate space for new Table data.\n" );
			return 1;
		}

		if ( !lua_to_table( L, 1, t ) ) {
			fprintf( stderr, "Failed to convert Lua data to table.\n" );
			return 1;	
		}

		if ( 0 ) {
			lt_dump( t );
		}

		//This isn't superflexible now...
		struct host **hostlist = build_hosts( t );
		struct route **routelist = build_routes( t );
		int item = get_int_value( t, "number", -1 ); 
		char *wash = get_char_value( t, "wash" );

		fprintf( stderr, "(%s).number: %d\n", f, item );
		fprintf( stderr, "(%s).wash: %s\n", f, wash );
		fprintf( stderr, "(%s).hosts:  %p\n", f, hostlist );
		dump_hosts( hostlist );
		fprintf( stderr, "(%s).routes: %p\n", f, routelist );
		dump_routes( routelist );

	}
	return 0;
}
