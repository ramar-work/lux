#include "filter-c.h"
#include "filter-static.h"

#define FILTER_C_DEBUG

#ifndef FILTER_C_DEBUG
 #define FILTER_C_PRINT(...)
#else
 #define FILTER_C_PRINT(...) \
	fprintf( stderr, "[%s:%d] ", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ )
#endif

#define HOMEDIR "/home/ramar/prj/hypno/"

int resolved ( const char *path, const char **routes ) {
	while ( routes && *routes ) {
		if ( resolve( path, *routes ) ) return 1;
		routes++;
	}
	return 0;
}

const char **copy_routes ( Route *routes ) {
	const char **routelist = NULL;
	int len = 0;
	while ( routes->route ) {
		add_item( &routelist, (char *)routes->route, const char *, &len );
		routes++;		
	}
	return routelist;
}

void print_routes ( Route *routes ) {
	while ( routes->route ) {
		fprintf(stderr,"\n");
		FILTER_C_PRINT( "Route '%s'\n", routes->route );
		routes++;		
	}
}

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

	//Get config	
	if ( !(config = (struct config *)ctx) ) 
		return http_set_error( res, 500, "failed to load clobal config." );

	//Static
	if ( check_static_prefix( req->path, "static" ) )
		return filter_static( req, res, config );

	//Execute static resources first...
	FILTER_C_PRINT( "Config ptr at: %p\n", config );

	if ( snprintf( filename, sizeof( filename ), "%s%s", config->path, apppath ) == -1 )
		return http_set_error( res, 500, "Path failed." );

	FILTER_C_PRINT( "Loading app at: %s\n", filename );

#if 0
	if ( stat( filename, &st ) == -1 ) {
		snprintf( err, sizeof( err ), "Application inaccessible at %s: %s.", filename, strerror(errno) );
		return http_set_error( res, 500, err ); 
	}
#endif

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
	if ( copy_routes( routes ) )
		print_routes( routes );	
	
#if 0
	//Find the matching route in a list of const char *
	if ( !resolved( config->path, copy_routes( routes ) ) )
		return http_set_error( res, 404, "Route not resolved." );


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
