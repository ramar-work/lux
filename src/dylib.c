//gcc -Wall -Werror -o lib lib.c
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>

#define FILTER_C_PRINT(...) fprintf( stderr, "%s:%d: ", __FILE__, __LINE__ ) && fprintf( stderr, __VA_ARGS__ );

#define HOME "/home/ramar/prj/hypno/"
const char filename[] = HOME "tests/filter-c/submarine.local/app.so";

int main (int argc, char *argv[]) {
	void *app = NULL;
	void **routes = NULL;
	char err[2048] = {0};
	char *fname = argv[1];
	if ( !fname ) {
		fprintf( stderr, "Please specify an object file.\n" );
		return 1;
	}

	fprintf( stderr, "try loading '%s'\n", filename );

	app = dlopen( fname, RTLD_LAZY ); 
	fprintf( stderr, "try loading '%p'\n", app );
#if 1
	if ( app == NULL ) { 
		FILTER_C_PRINT( "Could not open application at %s: %s.", fname, dlerror() );
		return 1;
	}

	FILTER_C_PRINT( "App initialized at: %p\n", app );
#endif
	//Find the routes (should always be called routes)
	if ( !( routes = dlsym( app, "routes" )) ) {
		FILTER_C_PRINT( "'routes' not found in C app: %s", strerror(errno) );
		return 1;
	}

	FILTER_C_PRINT( "App routes initialized at: %p\n", app );

#if 0
	//Find the matching route in a list of const char *

	//Handlers are much simpler now...
#endif

	if ( dlclose( app ) == -1 ) {
		FILTER_C_PRINT( "Failed to close application: %s\n", strerror( errno ) );
		return 1;
	}
	return 0;

}
