#include "../luabind.h"
#include "../config.h"
#include "../hosts.h"

#define TESTDIR "tests/config/"

/*
//Test data should show what happens if:
- parse error exists
- no errors exist
- required keys don't exist
- super giant files can't be parsed
- non-existent file exists
*/


struct Test {
	const char *file;
} tests[] = {
	{ TESTDIR "good.lua" },
  {	TESTDIR "badsyntax.lua" },	
	#if 0
  {	TESTDIR "def.lua" },	 //local config example (will have routes)
  {	TESTDIR "config.lua" },//local config example (w/ routes, but no complex models)
	#endif
  { NULL }
};


//Loads some random files
int main (int argc, char *argv[]) {
	//Load a bunch of configs...
	struct Test *test = tests;
	while ( test->file ) {
		char *f = (char *)test->file;
		char err[2048] = {0}; 
		struct config *config = build_config( f, err, sizeof( err ) );

		//Build configuration
		if ( !config ) {
			fprintf( stderr, "%s\n", err );	
			goto next;	
		}

		//Free configuration
		fprintf( stderr, "%p\n", config );
		free_config( config );
next:
		test++;
	}
	return 0;
}
