#include "luabind.h"
#include "util.h"

#define TESTDIR "tests/luabind/"

#define TESTCASE()

//Start with this weird list to test aggregation
const char *AGGREGATION[] = {
	"a",
	"ab",
	"abc",
#if 0
	(const char *[]){ TESTDIR "boom.lua", NULL },
	(const char *[]){ TESTDIR "chaka.lua", TESTDIR "laka.lua", NULL },
#endif
	NULL
};


int main ( int argc, char *argv[] ) {
	//Create a Lua environment
	char err[ 2048 ] = { 0 };
	int status = 0;

#if 1
	//I want to test aggregate more heavily
	//Or should I just give up?
	const char **agg_tests = AGGREGATION;
	int a = 1;
	while ( *agg_tests ) {
		int status = 0;
		lua_State *L = luaL_newstate();
		const char *file = *agg_tests;
		while ( *file ) {
			char filename[ 2048 ] = { 0 };
			snprintf( filename, sizeof(filename), "%s%s%d%c.lua", TESTDIR, "luabind-combine-", a, *file );
			fprintf( stderr, "%s\n", filename );
			if ( !lua_exec_file( L, filename, err, sizeof( err ) ) ) {
				fprintf( stderr,  "error executing file %s: %s\n", filename, err );
				status = 0;
				break;
			}	
			file++; 
		}
#if 1	
		if ( status && !lua_combine( L, err, sizeof(err) ) ) {
			fprintf( stderr,  "couldn't combine: %s\n", err );
		}

		zTable *t = malloc( sizeof( zTable ) );
		lt_init( t, NULL, 4096 );
		lua_to_table( L, 1, t );
		//lua_stackdump( L );
		lt_dump( t );
#endif
		a++, agg_tests++;
	}
#else
	//Load a string, and execute
	if ( !( status = lua_exec_string( L, string, err, sizeof(err) ) ) ) {
		fprintf( stderr, "This is an error: %s\n", err );
		//return 1;
	}

	//Load a file, and execute
	const char *file = "www/wrong.lua";
	if ( !( status = lua_exec_file( L, file, err, sizeof(err) ) ) ) {
		fprintf( stderr, "This is an error: %s\n", err );
		//return 1;
	}
	//status = lua_exec_string( L, string, err, sizeof(err) );

	//Load a file or string successfully...
	const char *file1 = "www/def.lua";
	if ( !( status = lua_exec_file( L, file1, err, sizeof(err) ) ) ) {
		fprintf( stderr, "This is an error: %s\n", err );
		//return 1;
	}

	//Dump the stack and show some stuff...
	lua_stackdump( L );

	//Load a bunch of files or strings and aggregate...
	const char *files[] = { "www/wrong.lua" };
	//Convert tables back and forth...
#endif

	return 0;	
}
