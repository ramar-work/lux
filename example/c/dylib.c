//gcc -Wall -Werror -ldl -o lib lib.c
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <errno.h>

#define FILTER_C_PRINT(...) fprintf( stderr, "%s:%d: ", __FILE__, __LINE__ ) && fprintf( stderr, __VA_ARGS__ );

int main (int argc, char *argv[]) {
	void *app = NULL;
	void **routes = NULL;
	const char *fname = NULL, *symbol = NULL;  

	//Stop if we don't specify enough
	if ( argc < 3 ) {
		fprintf( stderr, "Usage: ./dylib <lib> <symbol>\n" );
		return 1;
	}

	//Set these
	fname = argv[ 1 ];
	symbol = argv[ 2 ];

	//Try to load the app
	FILTER_C_PRINT( "Attempting to load '%s'\n", fname );
	if ( !( app = dlopen( fname, RTLD_LAZY ) ) ) {
		FILTER_C_PRINT( "Could not open application at %s: %s.\n", fname, dlerror() );
		return 1;
	}

	//Find the routes (should always be called routes)
	FILTER_C_PRINT( "App initialized at: %p\n", app );
	if ( !( routes = dlsym( app, symbol )) ) {
		dlclose( app );
		FILTER_C_PRINT( "'%s' not found in C app: %s\n", symbol, strerror(errno) );
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
