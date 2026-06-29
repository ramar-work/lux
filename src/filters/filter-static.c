/** 
 * filter-static.c
 * ===========
 * 
 * Summary 
 * -------
 * Functions comprising the static filter for interpreting HTTP messages.
 *
 * Usage
 * -----
 * filter-static.c enables Hypno to act as a static file server.
 *
 * LICENSE
 * -------
 * See LICENSE in the top-level directory for more information.
 *
 * ------------------------------------------- */
#include "filter-static.h"

#define FILTER_STATIC_DEBUG

#ifndef FILTER_STATIC_DEBUG
 #define FILTER_STATIC_PRINT(...)
#else
 #define FILTER_STATIC_PRINT(...) \
	fprintf( stderr, "[%s:%d]", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ )
#endif

// Define a default mimetype 
static const char def_mimetype[] = "application/octet-stream";

// Define a default content-type 
static const char def_ctype[] = "text/html";

// Define a config file name 
static const char conffile[] = "config.lua";

// Define disallowed paths by default
// TODO: How far do we want to go with this? (^ for the beginning, $ for the end?)
static const char *def_disallowed_paths[] = { "/config.lua",	"/misc", NULL };


// Handle keys relevant to this filter only
static const confkey_t keys[] = {
	{ "cache",    ZTABLE_TBL, 0, get_cache_header },
	/* TODO: Support this: ZTABLE_TBL || ZTABLE_TXT */
	{ "disallow", ZTABLE_TBL, 0, get_disallowed_paths }, 
	#if 0
	{ "redirect", ZTABLE_TBL, 0, get_redirect },
	#endif

#if 0
	// TODO: Move these to tests
	{ "number",   ZTABLE_INT, 0, xid },
	{ "db",       ZTABLE_TXT, 0, xsd },
	{ "things",   ZTABLE_TXT, 0, xld },
#endif
	{ NULL },
};


/**
 * const int filter_static ( const server_t *serv, conn_t *conn )
 *
 * The entry point for a static file server.
 *
 * Fills in all structures to send some static data back.
 *
 * Below is an example of a server config file:
 *
 * <pre>
 * return {
 * 	wwwroot = "/home/ramar/prj/lux-2025/lux-0.9.5-beta/test/www",
 * 	hosts = {
 * 		["localhost"] = { 
 * 		-- All of these COULD go in the projet directory
 * 		-- favicon?
 * 		-- redirect?
 * 		-- allow_relative?
 * 		-- disallow?
 * 		-- cache_control?
 * 			root_default = "/index.html",
 * 			dir = "www-static",
 * 			filter = "static"
 * 		},
 * 	}
 * }
 * </pre>
 */
const int filter_static ( const server_t *serv, conn_t *conn ) {

	// Define all sorts of things
	char *extension = NULL;
	char *fname = conn->req->path;
	char cpath[ 2048 ] = {0};
	char err[ 2048 ] = {0};
	char fpath[ 2048 ] = {0};
	const char *cache_header = NULL;
	const char *redir_header = NULL;
	int da = 0, *disallowed = &da;
	int fd = 0;
	int status = 0;
	struct lconfig *host = conn->config;
	struct mime_t *mimetype = (struct mime_t *)zmime_get_default();
	struct stat sb = {0};
	ztable_t *conf = NULL;

	// Die if no host directory is specified.
	// TEST: Add a domain and don't specify the host directory
	if ( !host->dir ) {
		return http_error( conn->res, 500, "%s", "No directory path specified for this site." );
	}

	// Catch / requests when dealing with static servers
	if ( !fname || ( strlen( fname ) == 1 && *fname == '/' ) ) {
		// TEST: Add a domain and don't specify the root_default
		if ( !host->root_default ) {
			return http_error( conn->res, 404, "%s", "No default root specified for this site." );
		}
		fname = (char *)host->root_default; 
	}

	// TODO: Deal with relative directories (or completely disable them in static mode)
	// ...
		
	// Create a fullpath
	// TEST: Use ultra long names that result in an impossibly long full path
	if ( !concat( fpath, sizeof( fpath ) - 1, "%s%s", host->dir, fname ) ) {
		return http_error( conn->res, 500, "%s", "Path truncation occurred." );
	}

	// Check for the existence of file on server
	// TEST: Stat SHOULD fail if x perm is NOT on a directory in the path we're searching
	if ( stat( fpath, &sb ) == -1 ) {
		snprintf( err, sizeof( err ) - 1, "%s: %s.", strerror( errno ), fpath );
		// TEST: Disallow access most anywhere in the requested path
		if ( errno == EACCES ) {
			// log it, and do any custom handling
			return http_error( conn->res, 401, "%s", "Unauthorized" );
		}
		// TEST: Request a file that's not there
		else if ( errno == ENOENT ) {
			// log it, and do any custom handling
			return http_error( conn->res, 404, "%s", "Not found" );
		}
		// TEST: too many symbolic links is somewhat easy to simulate
		else {
			return http_error( conn->res, 500, "%s", "Server error occurred" );
		}
	}

	// Then check that it's a real file (if not, it's a 400)
	if ( !S_ISREG( sb.st_mode ) && !S_ISLNK( sb.st_mode ) ) { 
		return http_error( conn->res, 400, "%s", "Bad Request" );
	}

	// TODO: This is much simpler... but could pose a security risk (see `man 2 access`)
	// TODO: Consider replacing with sb.st_mode bitmasks
	if ( access( fpath, R_OK ) == -1 ) {
	#if 1
		return http_error( conn->res, 401, "%s", "Unauthorized" );
	#else
		snprintf( err, sizeof( err ) - 1, "%s: %s.", strerror( errno ), fpath );
		// TEST: Disallow access most anywhere in the requested path
		if ( errno == EACCES ) {
			// log it, and do any custom handling
			return http_error( conn->res, 401, "%s", "Unauthorized" );
		}
		// TEST: Request a file that's not there
		else if ( errno == ENOENT ) {
			// log it, and do any custom handling
			return http_error( conn->res, 404, "%s", "Not found" );
		}
		// TEST: too many symbolic links is somewhat easy to simulate
		else {
			return http_error( conn->res, 500, "%s", "Server error occurred" );
		}
	#endif
	}

	// Check for default disallowed paths	
	for ( const char **dname = def_disallowed_paths; dname && *dname; dname++ ) {
		if ( memstrat( fname, *dname, strlen(fname) ) == 0 ) {
			FPRINTF( "--- Caught disallowed path '%s' ~= %s ---\n", *dname, fname );
			// TODO: Log the attempt 
			return http_error( conn->res, 401, "Unauthorized" );
		}	
	}
 	
	// Create a fully qualified path to a config file
	if ( !concat( cpath, sizeof( cpath ), "%s/%s", conn->config->dir, conffile ) ) {
		return http_error( conn->res, 500, "%s", "Failed to create configuration file path." );
	}

	#if 0
	FPRINTF( "*** fname (requested URI) is '%s' ***\n", fname );
	FPRINTF( "*** site dir is '%s'***\n", conn->config->dir );
	FPRINTF( "*** fullpath is '%s' ***\n", fpath );
	FPRINTF( "*** config path is '%s' ***\n", cpath );
	FPRINTF( "*** can access? = %d ***\n", access( cpath, R_OK ) ) ;
	#endif

	#if 0
	// Check for config file and log if it does not exist
	if ( access( cpath, R_OK ) == -1 ) {
		// This would log any error, for the purposes of keeping your sanity
	}
	#endif

	// Check for config file and run it if it exists
	if ( access( cpath, R_OK ) == 0 ) {

		// Any misconfiguration here is instantly a 500
		if ( !( conf = load_luaconf( cpath, err, sizeof( err ) ) ) ) {
			FPRINTF( "*** Load err: %s ***\n", err );
			return http_error( conn->res, 500, "%s", err );
		}

		// Check types and all here (and if it's required)
		if ( !check_conf_keys( conf, keys, conffile, err, sizeof( err ) ) ) {
			FPRINTF( "--- conf err: %s ***\n", err );
			return http_error( conn->res, 500, "%s", err );
		}

		// Search for disallow
		if ( extr_key( conf, "disallow", keys, fname, &disallowed, err, sizeof( err ) ) && *disallowed ) {
			FPRINTF( "--- disallow = %s ***\n", err );
			return http_error( conn->res, 401, "%s", "Unauthorized" );
		}

		#if 0
		// Search for redirect
		if ( extr_key( conf, "redirect", keys, fname, &redir_header, err, sizeof( err ) ) && redir_header ) {
			// May have to make the entire message
			FPRINTF( "--- redirect = %s ***\n", redir_header );
			//return http_error( conn->res, 401, "%s", "Allocation failure." );
		}
		#endif

		// Search for cache 
		if ( extr_key( conf, "cache", keys, fname, &cache_header, err, sizeof( err ) ) && cache_header ) {
			//http_set_header( conn->res, "Cache-control", cache_header );
			http_copy_header( conn->res, "Cache-control", cache_header );
			// free( cache_header );
		}
	}

	// Send the requested file (or a 404)
	if ( !send_static( conn->res, fpath, err, sizeof( err ) ) ) {
		FPRINTF( "*** an error occurred? %s ***\n", err );
		return http_error( conn->res, 500, "%s", err );
	}

	return 1;
}
