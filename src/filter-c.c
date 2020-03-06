#include "filter-c.h"

#define FILTER_C_DEBUG

#ifndef FILTER_C_DEBUG
 #define FILTER_C_PRINT(...)
#else
 #define FILTER_C_PRINT(...) \
	fprintf( stderr, "[%s:%d] ", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ )
#endif

#define HOMEDIR "/home/ramar/prj/hypno/"

int filter_c ( struct HTTPBody *req, struct HTTPBody *res, void *ctx ) {
	Table *c = NULL, *t = NULL;
	Route *routes = NULL;
	void *app = NULL;
	const char *apppath = "/app/app.so"; //or "/app.so"
	struct config *config = NULL;
	struct routehandler **handlers = NULL;
	char err[ 2048 ] = { 0 };
	char filename[ 2048 ] = { 0 };
	struct stat st;
	
	if ( !(config = (struct config *)ctx) ) 
		return http_set_error( res, 500, "failed to load clobal config." );

	//Execute static resources first...
	FILTER_C_PRINT( "Config ptr at: %p\n", config );

	if ( snprintf( filename, sizeof( filename ), "%s%s", config->path, apppath ) == -1 )
		return http_set_error( res, 500, "Path failed." );

	FILTER_C_PRINT( "Loading app at: %s\n", filename );

	if ( stat( filename, &st ) == -1 ) {
		snprintf( err, sizeof( err ), "Application inaccessible at %s: %s.", filename, strerror(errno) );
		return http_set_error( res, 500, err ); 
	}

	if ( !( app = dlopen( filename, RTLD_LAZY ) ) ) {
		snprintf( err, sizeof( err ), "Could not open application: %s.", dlerror() );
		return http_set_error( res, 500, err ); 
	}

	FILTER_C_PRINT( "App initialized at: %p\n", app );

	//Find the routes (should always be called routes)
	if ( !( routes = ( Route * ) dlsym( app, "routes" )) ) {
		snprintf( err, sizeof( err ), "'routes' not found in C app: %s", dlerror() );
		return http_set_error( res, 500, err ); 
	}

	FILTER_C_PRINT( "App routes initialized at: %p\n", routes );

#if 0
	//Find the matching route in a list of const char *
	while ( routes->route ) {
		FILTER_C_PRINT( "Route %s\n", routes->route );
		routes++;		
	}

	//Execute the model(s)

	//Render the view(s)

	//OR...

	//Save model(s) and view(s) to their own ptr arrays
	//Destroy dylib
	//...
#endif

	if ( dlclose( app ) == -1 ) {
		snprintf( err, sizeof( err ), "Failed to close application: %s\n", strerror( errno ) );
		return http_set_error( res, 500, err );
	}

	FILTER_C_PRINT( "App now closed, address is: %p\n", app );
	return http_set_error( res, 200, "This is a C app, and it sucks..." );
}
