/* ------------------------------------------- * 
 * filter-c.c
 * ===========
 * 
 * Summary 
 * -------
 * Filter for interpreting HTTP messages.
 *
 * Usage
 * -----
 * Modules written with filter-c in mind are apps written in C that can send
 * an HTTP/HTTPS response back to a server. This is mostly for testing, but it
 * is theoretically possible to deploy a full blown web application in C as a 
 * dynamic library.
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
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


#if 0
int resolved ( const char *path, const char **routes ) {
	while ( routes && *routes ) {
		if ( resolve( path, *routes ) ) return 1;
		routes++;
	}
	return -1;
}


Route * resolve_route ( const char *path, Route *routes ) {
	FILTER_C_PRINT( "path: %s\n", path );
	while ( routes->route ) {
		FILTER_C_PRINT( "route: %s\n", routes->route );
#if 1
		if ( resolve( routes->route, path ) ) {
			return routes;
		}
#endif
		routes++;
	}
	return NULL;
}


const char **copy_routes ( Route *routes ) {
	const char **routelist = NULL;
	int len = 0;
	while ( routes->route ) {
		if ( !add_item( &routelist, (char *)routes->route, const char *, &len ) ) {
			return NULL;
		}
		routes++;		
	}
	return routelist;
}
#endif


//...
const int filter_c ( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn ) {
#if 0
	Route *routelist, *route = NULL;
#endif

	zTable *model = NULL;
	const char *apppath = "/bin/app.so";
	void *app = NULL;
	char err[ 2048 ] = { 0 };
	char filename[ 2048 ] = { 0 };
	uint8_t *msg = NULL;
	int msglen = 0;
	struct lconfig *config = conn->hconfig;

#if 0
	//Get config (this should never happen)
	if ( !(config = (struct sconfig *)conn->ctx) ) {
		conn->count = -3;	
		return http_set_error( res, 500, "failed to load clobal config." );
	}
#endif

	//Execute static resources first...
	if ( check_static_prefix( req->path, "static" ) ) {
		return filter_static( fd, req, res, conn );
	}

	FILTER_C_PRINT( "Config ptr at: %p\n", config );

	if ( snprintf( filename, sizeof( filename ), "%s%s", config->dir, apppath ) == -1 ) {
		return http_set_error( res, 500, "Path failed." );
	}

	FILTER_C_PRINT( "Loading app at: %s\n", filename );

	//Try opening the app
	if ( !( app = dlopen( filename, RTLD_NOW ) ) ) {
		snprintf( err, sizeof( err ), "Could not open application: %s.", dlerror() );
		return http_set_error( res, 500, err ); 
	}

	FILTER_C_PRINT( "App initialized at: %p\n", app );

#if 1
	//Just run "main" from the shared object, and let that handle everything 
	int (*a)( struct HTTPBody *req, struct HTTPBody *res );

	if ( !( a = dlsym( app, "app" ) ) ) {
		snprintf( err, sizeof( err ), "Could not find main() method in shared app: %s.", dlerror() );
		return http_set_error( res, 500, err ); 
	}

	FILTER_C_PRINT( "App entry point found.\n" );

	if ( !a( req, res ) ) {
		return http_set_error( res, 500, "Execution error at shared app." );
	}

	FILTER_C_PRINT( "App successfully executed.\n" );
		
#else
#if 0
	//Find the routes (should always be called routes)
	if ( !( routelist = ( Route * ) dlsym( app, "routes" )) ) {
		dlclose( app ); 
		snprintf( err, sizeof( err ), "'routes' not found in C app: %s", dlerror() );
		return http_set_error( res, 500, err ); 
	}

	FILTER_C_PRINT( "App routes initialized at: %p\n", routelist );
	if ( !( route = resolve_route( req->path, routelist ) ) ) {
		dlclose( app ); 
		snprintf( err, sizeof( err ), "Route '%s' not in path.", req->path ); 
		return http_set_error( res, 404, err );
	}

	FILTER_C_PRINT( "Current route: %s\n", route ? route->route : "(none)" );

	//Execute the model(s)
	model = route->models( req, res, 0 );
	lt_dump( model );

	//Try to interpret views
	while ( route->views && *route->views ) {
		//Damn, forgot how hard this was...
		char view[ 2048 ] = { 0 }; 
		int filelen = 0, renlen = 0;
		uint8_t *file, *ren; 
		snprintf( view, sizeof( view ), "%s%s%s.tpl", config->path, "/views/", *route->views );
		FILTER_C_PRINT( "View: %s, %s\n", *route->views, view );

		if ( !( file = read_file( view, &filelen, err, sizeof(err) ) ) ) {
			dlclose( app ); 
			return http_set_error( res, 500, err );
		}

		FILTER_C_PRINT( "View buf len: %d\n", filelen );
		write( 2, file, filelen );

		if ( !( ren = table_to_uint8t( model, file, filelen, &renlen ) ) ) {
			free( file );
			dlclose( app ); 
			fprintf( stderr, "Render failed...\n" );
			return http_set_error( res, 500, err );
		}
		FILTER_C_PRINT( "Render len: %d\n", renlen );
#if 0
		write( 2, ren, renlen );getchar();
#endif
		if ( !append_to_uint8t( &msg, &msglen, ren, renlen ) ) {	
			free(file);
			free(ren);
			dlclose( app ); 
			snprintf( err, sizeof(err), "Failed to allocate additional space for response message." );
			return http_set_error( res, 500, err );
		}
		route->views++;
	}
#endif
#endif

	if ( dlclose( app ) == -1 ) {
		snprintf( err, sizeof( err ), "Failed to close application: %s\n", strerror( errno ) );
		return http_set_error( res, 500, err );
	}

	FILTER_C_PRINT( "App now closed, address is: %p\n", app );

	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err );
	}
	return 1;
}


void print_routes ( Route *routes ) {
	while ( routes->route ) {
		fprintf(stderr,"\n");
		FILTER_C_PRINT( "Route '%s'\n", routes->route );
		routes++;		
	}
}


