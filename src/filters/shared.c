/**
 * util.c
 * =======
 *
 * Summary
 * -------
 * Shared utilities for filters.
 *
 * LICENSE
 * -------
 * Copyright 2020-2026 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 */
#include "shared.h"

#ifdef DEBUG_H
 #define zhttp_message_type(T) \
	( T == ZHTTP_MESSAGE_MALLOC ) ? "ZHTTP_MESSAGE_MALLOC" : \
	( T == ZHTTP_MESSAGE_STATIC ) ? "ZHTTP_MESSAGE_STATIC" : \
	( T == ZHTTP_MESSAGE_SENDFILE ) ? "ZHTTP_MESSAGE_SENDFILE" : "ZHTTP_MESSAGE_OTHER"
#else
 #define zhttp_message_type(T) "(?)" 
#endif

static const char ctype_def[] = "text/html";

/**
 * char * getpath( char *rp, char *ap, int destlen )
 *
 * Returns just the path from a URL [rp], into [ap]
 *
 */
char * getpath( char *rp, char *ap, int destlen ) {
	int pos = 0, len = strlen( rp );

	if ( ( pos = memchrat( rp , '?', len ) ) > -1 ) {
		len = pos;
	}

	if ( destlen <= pos ) {
		return NULL;
	}
	
	for ( char *p = ap, *path = rp; *path && *path != '?'; ) {
		*(p++) = *(path++);
	}

	return ap;
}


/*
// this can be done with va_args
static char *concat ( char *buf, int buflen, char *first, char *last ) {
return NULL;
}
*/


/**
 * int http_error( zhttp_t *res, int status, char *fmt, ... ) 
 *
 * Wrapper around http_finalize_response to report errors back to the client.
 *
 */
const int http_error( zhttp_t *res, int status, char *fmt, ... ) {
	va_list ap;
	char err[ 2048 ];
	memset( err, 0, sizeof( err ) );
	va_start( ap, fmt );
	vsnprintf( err, sizeof( err ), fmt, ap );
	va_end( ap );
	memset( res, 0, sizeof( zhttp_t ) );
	res->clen = strlen( err );
	http_set_status( res, status );
	http_set_ctype( res, ctype_def );
	http_copy_content( res, err , strlen( err ) );
	zhttp_t *x = http_finalize_response( res, err, strlen( err ) );
	return x ? 1 : 0;
}



/**
 * int http_send( zhttp_t *res, int status, char *fmt, ... ) 
 *
 * Wrapper around http_finalize_response to report other statuses back to the client.
 * NOTE: Whatever is currently in the response will be sent to the client
 *
 */
const int http_send_message( zhttp_t *res, int status, char *fmt, char *err, int errlen ) {
	char buf[ 2048 ];
	memset( buf, 0, sizeof( buf ) );
#if 0
	va_list ap;
	va_start( ap, fmt );
	vsnprintf( err, sizeof( err ), fmt, ap );
	va_end( ap );
#endif
	memset( res, 0, sizeof( zhttp_t ) );
	res->clen = strlen( buf );
	http_set_status( res, status );
	http_set_ctype( res, ctype_def );
	http_copy_content( res, buf, strlen( buf ) );
	zhttp_t *x = http_finalize_response( res, buf, strlen( buf ) );
	return x ? 1 : 0;
}

/**
 * const int send_static ( zhttp_t *res, const char *dir, const char *uri )
 *
 * Workaround whatever handler is in use and send a static file back to the client.
 *
 */
const int send_static ( zhttp_t *res, const char *path, char *err, int errlen ) {

	// Read_file and return that...
	const struct mime_t *mime;
	int fd = 0;
	int status = 0;
	struct stat sb = {0};
	unsigned char *data = NULL;

	// Get its mimetype
	if ( !( mime = zmime_get_by_filename( path ) ) ) {
		mime = zmime_get_default();
	}

	#if 1
	FPRINTF( "*** mimetype = '%s' ***\n", mime->mimetype );
	FPRINTF( "*** extension = '%s' ***\n", mime->extension );
	FPRINTF( "*** path = '%s' ***\n", path );
	#endif

#if 1
	// Write max should be checked.
	int dlen = 0;

	// Read file to buffer and send back
	if ( !( data = read_file( path, &dlen, err, errlen ) ) ) {
		return 0; //http_error( res, 500, "static read failed: %s", err );
	}

	// Send the message out
	res->clen = dlen;
	http_set_status( res, 200 );
	http_set_ctype( res, mime->mimetype );
	http_set_content( res, data, dlen );

	// Send it
	FPRINTF( "*** finalizing response with %s ***\n", path );
	if ( !http_finalize_response( res, err, errlen ) ) {
		//return http_error( res, 500, err );
		free( data );
		return 0;
	}

	FPRINTF( "*** freeing data ***\n" );
	free( data );
#else
	// Open the file
	if ( ( fd = open( path, O_RDONLY ) ) == -1 ) {
		return http_error( res, 404, strerror( errno ) );
	}

	// Prepare the message
	// TODO: Make some distinctions between STATIC, MALLOC and SENDFILE
	#if 1
	//TODO: Enable non sendfile capable systems to be able to send a file the old crappy way.
	res->atype = ZHTTP_MESSAGE_SENDFILE;
	res->clen = sb.st_size;
	res->fd = fd;
	res->status = 200;
	res->ctype = (char *)mime->mimetype;
	#else
	http_set_fd( res, fd );
	http_set_content_length( res, sb.st_size );
	http_set_message_type( res, ZHTTP_MESSAGE_SENDFILE );
	http_set_status( res, 200 );
	http_set_ctype( res, mime->mimetype );
	#endif

	#if 1
	FPRINTF( "*** res->atype  = '%s' ***\n", zhttp_message_type( res->atype ) );
	FPRINTF( "*** res->ctype  = '%s' ***\n", res->ctype );
	FPRINTF( "*** res->clen   = %5ld ***\n", res->clen );
	FPRINTF( "*** res->fd     = %3d  ***\n", res->fd );
	FPRINTF( "*** res->status = %3d  ***\n", res->status );
	#endif

	#if 1
	// Finalize the response
	if ( !http_finalize_response( res, err, errlen ) ) {
		//return http_error( res, 500, err );
		return 0;
	}
	#else
	return http_error( res, 404, "this page was found, but we're testing stuff." );
	#endif
#endif
	return 1;
}




/**
 * int load_luaconf( struct luadata_t *l )
 *
 * Loads an instance's configuration file in a completely
 * different way...
 *
 */
ztable_t * load_luaconf( const char *path, char *err, int errlen ) {

	// Define
	int status = 0;
	lua_State *lx = NULL;
	struct stat sb = { 0 };
	ztable_t *t = NULL;

	// Status...
	FPRINTF( "*** Attempting to load a Lua config file. ***\n" );
		
	// A seperate Lua environment should be made here (to keep it contained)
	if ( !( lx = luaL_newstate() ) ) {
		snprintf( err, errlen, "%s", "Failed to initialize Lua environment." );
		return 0;
	}
	
	// Find the config file in question
FPRINTF( "--- stat ---\n" );
	if ( stat( path, &sb ) == -1 ) {
		const char *fmt = "Failed to stat() file: %s: %s";
		// TODO: Use the shallow path instead
		snprintf( err, errlen, fmt, path, strerror( errno ) );
		return 0;
	}

	// Block zero-length configs
FPRINTF( "--- check for zero ---\n" );
	if ( !sb.st_size ) {
		const char *fmt = "Configuration file for instance at path %s is zero-length.";
		snprintf( err, errlen, fmt, path );
		return 0;
	}

	// Open and execute, catching and reporting back any errors
FPRINTF( "--- attempt to exec ---\n" );
	if ( !lua_exec_file( lx, path, err, errlen ) ) {
		// NOTE: Error is already written, so this just looks funny
		return 0;
	}
FPRINTF( "--- any errors? %s ---\n", err );

	//If it's anything but a Lua table, we're in trouble
FPRINTF( "--- table check? ---\n" );
	if ( !lua_istable( lx, 1 ) ) {
		snprintf( err, errlen, "Configuration is not a Lua table." );
		return 0;
	}

#if 0
	// Get a count of how many things are in the table
	int count = 0;
FPRINTF( "--- count check? ---\n" );
	if ( ( count = lua_count( lx, 1 ) ) < 1 ) {
		snprintf( err, errlen, "Configuration table has no values." );
		return 0;
	}

	FPRINTF( "--- count is %d ---\n", count );
	lua_rdumpstack(lx);
#endif

	// Initialize a table of the right size
FPRINTF( "--- Table init ---\n" );
	if ( !( t = lt_make( LT_SIZE_MAX ) ) || !lua_to_ztable( lx, 1, t ) ) {
		snprintf( err, errlen, "Conversion of config to ztable failed." );
		return 0;
	}

	// Lock
FPRINTF( "--- Lock (and finalize hashes) ---\n" );
	if ( !lt_lock( t ) ) {
		snprintf( err, errlen, "Initialization of ztable failed." );
		return 0;
	}

#if 0
for ( const char **x = lt_get_keys(t); x && *x; x++ ) {
	FPRINTF( "--- KEY = %s ---\n", *x );
}
#endif

FPRINTF( "--- Dump? ---\n" );
lt_dump( t ); // indent is wrong, and ...

#if 0
	// TODO: use pointers instead.  There is no reason to copy all of this...
	if ( ( db = lt_text( t, "db" ) ) ) {
		memcpy( (void *)l->db, db, strlen( db ) );
	}

	if ( ( fqdn = lt_text( t, "fqdn" ) ) ) {
		memcpy( (void *)l->fqdn, fqdn, strlen( fqdn ) );
	}
#endif

#if 0
	// Rather difficult to use, but this is how looping is done
FPRINTF( "--- Loop through the table ---\n" );
	int x = 0;
	for ( int len, i = 1, size = lt_counta( t, x ); i < size; i++ ) {
		zKeyval *kv = lt_retkv( t, x + i );
		len = strlen( kv->value.v.vchar );

FPRINTF( "[ key = %s ]\n", kv->key.v.vchar );
FPRINTF( "[ value = %s ]\n", kv->value.v.vchar );

		// Copy off both the key and value?
		// Doing this for whatever is a good call, but
		// recursive descent might prove pretty difficult
	#if 0
		if ( len <= ulen && memcmp( kv->value.v.vchar, l->req->path, len ) == 0 ) {
			return 1;
		}
	#endif
	}
#endif
	lua_pop( lx, 1 );
	return t;
}



// THIS CODE BELONGS IN THE LUA HANDLER
#if 0
	// Set shadow path
	lua_pushstring( l->state, l->root );
	lua_setglobal( l->state, CKEY_SHADOW );
#endif


// THIS CODE BELONGS IN THE LUA HANDLER
#if 0
	//Check if there are any route files
	snprintf( cpath, sizeof(cpath) - 1, "%s/%s", l->root, rkey );

	//Get a directory listing
	if ( !( dir = opendir( cpath ) ) || !dir_has_files( dir ) ) {
		count = lua_count( l->state, 1 );
	}
	else {
		//Find the routes table and put that on the stack
		for ( lua_pushnil( l->state ); lua_next( l->state, 1 ); ) {
			if ( lua_type( l->state, -2 ) == LUA_TSTRING && !strcmp( lua_tostring( l->state, -2 ), "routes" ) ) {
				lua_remove( l->state, -2 );
				break;
			}
			lua_pop( l->state, 1 );
		}

		//Load each route file and combine it with the route table
		for ( int dlen; ( d = readdir( dir ) ) ; ) {
			//Initialize structures
			memset( cpath, 0, sizeof( cpath ) );
			memset( &sb, 0, sizeof( struct stat ) );

			//Use some of these for slightly easier way to program this
			char *fn = d->d_name;

			//Only deal with regular Lua files (eventually can support symbolic links)
			if ( ( dlen = strlen( fn ) > 4 ) && strstr( fn, ".lua" ) && d->d_type == DT_REG ) {
				//Copy the canonical path
				snprintf( cpath, sizeof(cpath) - 1, "%s/%s/%s", l->root, "routes", fn );
				FPRINTF( "Checking for valid route file at: %s\n", cpath );

				//stat
				if ( stat( cpath, &sb ) == -1 ) {
					char terr[ 1024 ] = {0};
					strerror_r( errno, terr, sizeof( terr ) ); 	
					snprintf( l->err, LD_ERRBUF_LEN, "Failed to stat() file: %s: %s", cpath, terr );
					return 0;
				}

				//zero-length files make no sense here, but it's not a big deal so skip it
				//TODO: Log this
				if ( !sb.st_size ) {
					continue;	
				}

				//open and execute, catching and reporting back any errors
				if ( !lua_exec_file( l->state, cpath, l->err, LD_ERRBUF_LEN ) ) {
					return 0;
				}

				//The resultant value should ALWAYS be a table
				if ( !lua_istable( l->state, lua_gettop( l->state ) ) ) {
					snprintf( l->err, LD_ERRBUF_LEN, "File at %s did not return a table.", cpath );
					return 0;
				}

				//Merge
				if ( !lua_rmerge( l->state, 2, lua_gettop( l->state ), l->err, LD_ERRBUF_LEN ) ) {
					return 0;
				}

				//Pop the added table
				lua_pop( l->state, 1 ) /*, lua_rdumpstack( l->state ) */;
			}
		}

		//Add the right key name and set this table
		lua_pushstring( l->state, "routes" );
		lua_insert( l->state, 2 );
		lua_settable( l->state, 1 );

		//Close the directory
		count = lua_count( l->state, 1 );
	}

	//Close the directory
	closedir( dir );

	//If there is a zero count for whatever reason, this needs to stop
	if ( !count ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Configuration table has no values." );
		return 0;
	}
#endif


#if 0
// a string modifier 
void * xsd ( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen ) {
	FPRINTF( "in = %p\n", input ), FPRINTF( "out = %p\n", output );
	*output = (void *)strdup( "TEST" );
	return (void *)1;
}

// an integer modifier (should be pretty easy)
void * xid ( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen ) {
	FPRINTF( "in = %p\n", input ), FPRINTF( "out = %p\n", output );
	if ( ( *output = malloc( sizeof (int) ) ) ) {
		//**output = 1;
		int *i = *output;
		*i = 1;
	}
	return (void *)1;
}

// a test list modifier
void * xld ( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen ) {
	FPRINTF( "xld start...\n" );
	int count = 0;
	char *str1 = strdup( "wu-tang" ), *str2 = strdup( "lord finesse" );
	if ( !add_item( output, str1, char *, &count ) ) {
		FPRINTF( "*** Failed to add string %s\n", str1 );
		return NULL;
	}
	if ( !add_item( output, str2, char *, &count ) ) {
		FPRINTF( "*** Failed to add string %s\n", str2 );
		return NULL;
	}
	//FPRINTF( "success? = %p\n", output );
	FPRINTF( "xld end...\n" );
	return (void *)1;
}
#endif


/**
 * int extr_key ( ztable_t *conf, const char *key, const confkey_t ** list, const void *i, void *res, char *err, int errlen )  
 *
 * Get a key from a table.
 *
 */
int extract_key ( 
	ztable_t *conf, const char *key, const confkey_t * list, const void *input, void **output, char *err, int errlen ) { 

	// Define 
	int index = -1;
	const confkey_t *ff = NULL;

	// Check for the key in the table, return 0, if you can't find it
	if ( ( index = lt_geti( conf, key ) ) == -1 ) {
		FPRINTF( "--- key %s not found in table ---", key );
		return 0;
	}

	// Locate the key in the given configuration list 
	for ( const confkey_t *x = list; x->name; x++ ) {
		FPRINTF( "--- comparing keys ( %s, %s ) ---\n", x->name, key );
		if ( strcmp( x->name, key ) == 0 ) { 
			FPRINTF( "--- SUCCESS! ---\n" );
			ff = x;
			break;
		}
	}

	// If not found, also stop
	if ( !ff ) {
		FPRINTF( "--- key %s not found in list ---\n", key );
		return 0;
	}
	
	#if 0
	FPRINTF( "--- name = %s, exp type = %d (%s), funct = %p, actual type = %d (%s) ---\n", 
		ff->name, ff->type, lt_typename( ff->type ), ff->exec, 
		lt_vta( conf, index ), lt_typename( lt_vta( conf, index ) ) );
	#endif

	// Execute whatever function you're trying to call (key should always be there)
	if ( !ff->exec( conf, index, input, output, err, errlen ) ) {
		FPRINTF( "--- key command execution failed = %s\n", err );
		return 0;
	}

	return 1;
}

/**
 * int check_conf_keys ( ztable_t *conf, confkey_t *keys, const char *confname, char *err, int errlen ) 
 *
 * Checks configuration keys.
 *
 */
int check_conf_keys ( ztable_t *conf, const confkey_t *keys, const char *confname, char *err, int errlen ) {

	// Search through all the keys
	for ( const confkey_t *k = keys; k->name; k++ ) {

		// Define 
		int index = -1;
		const confkey_t *ff = NULL;

		// Check for the presence of the key (get it's index)
		index = lt_geti( conf, k->name );

		// If the key is required and not there, die
		if ( index == -1 && k->required ) {
			const char fmt[] = "Misconfiguration in %s.  Required key '%s' not found.";
			snprintf( err, errlen, fmt, confname, k->name );
			return 0;
		}

		// If the key is there and type does not match, die
		if ( index > -1 && k->type != lt_vta( conf, index ) ) {
			const char fmt[] = "Misconfiguration at key '%s' in %s.  Expected %s, got %s";
			const char *extype = lt_typename( k->type );
			const char *actype = lt_typename( lt_vta( conf, index ) );
			snprintf( err, errlen, fmt, k->name, confname, extype, actype ); 
			return 0;
		}

	}

	return 1;
}


/**
 * const char ** get_disallowed_paths ( ztable_t *conf, const char **dp, int i, int *dpcount ) 
 *
 * Get disallowed paths from config table.
 *
 */
void *get_disallowed_paths( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen ) {

	const char *uri = (const char *)input;
	int *x = (int *)*output;
	
	for ( int len, ii = 1, size = lt_counti( conf, i ); ii <= size; ii++ ) {
		zKeyval *kv = lt_retkv( conf, i + ii );
		const char *value = NULL; // TODO: Deref to this instead?
		if ( lt_vta( conf, i + ii ) == ZTABLE_TXT && kv->value.v.vchar ) { 
			FPRINTF( "--- Comparing path against disallowed path ('%s' ?= '%s' ) ---\n", uri, kv->value.v.vchar );
			if ( memstrat( uri, kv->value.v.vchar, strlen( uri ) ) == 0 ) {
				*x = 1;	
				return (void *)1;	
			}
		}
	}

	return (void *)1;
}


/**
 * void * get_redirect ( ztable_t *conf, int i, void *input, void *output, char *err, int errlen ) 
 *
 * Get redirect paths from config file.
 *
 */
void * get_redirect ( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen ) {

	const char *uri = (const char *)input;
	char **x = (char **)output;

	// Stop immediately here, b/c this is the end of the request
	for ( int ii = 1, size = lt_counti( conf, i ); ii <= size; ii++ ) {
		zKeyval *kv = lt_retkv( conf, i + ii );
	#if 0
		if ( lt_kta( conf, i + ii ) == ZTABLE_TXT && kv->key.v.vchar ) { 
			FPRINTF( "--- Comparing path against redirect path ('%s' ?= '%s' ) ---\n", uri, kv->key.v.vchar );
			#if 1
			if ( memstrat( uri, kv->key.v.vchar, strlen( uri ) ) == 0 ) {
				// If it's a string, then assume it's a 302
				if ( lt_vta( conf, i + ii ) == ZTABLE_TXT ) {
					*x = kv->value.v.vchar;
				}
				// If it's a table, do something else, status/type and location must exist
				else if ( lt_vta( conf, i + ii ) == ZTABLE_TBL ) {
					// Pop off and search for Location and status
					// Either missing is an error (and the code needs to handle that)
				}
				// This could return an error, and you need to define that, leave the thing blank
				else {
					x = NULL;
					const char fmt[] = "Misconfiguration at key %s in %s. Expected %s, got %s.";
					snprintf( err, errlen, fmt, "redirect", "?", "table or string", "?" ) ;
					return 0;
				}
			}
			#endif
		}
	#endif
	}
	return NULL;
}


/**
 * void * get_cache_header ( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen );
 *
 * Get cache header for a specific path.
 *
 */
void * get_cache_header ( ztable_t *conf, int i, const void *input, void **output, char *err, int errlen ) {
#if 1
	char **header = (char **)output;
	char *uri = (char *)input;
	char tmpbuf[ 512 ] = {0};

	// Add each occurrence here, b/c different paths are configured differently
	for ( int x, ii = 1, size = lt_counti( conf, i ); ii <= size; ii++ ) {
	
		// Dereference the table	
		zKeyval *kv = lt_retkv( conf, i + ii );

		// TODO: This is pretty limited, but will get you 90% of the way...
		if ( memstrat( uri, kv->key.v.vchar, strlen( uri ) ) == 0 ) {
			// ...take the string or concat table's values into a string, misconfigs are 500
			int type = lt_vta( conf, i + ii );
			if ( type != ZTABLE_TXT && type != ZTABLE_TBL ) {
				const char fmt[] = "Misconfiguration at key 'cache.%s'.  Expected table or string, got %s.";
				snprintf( err, errlen, fmt, kv->key.v.vchar, lt_typename( type ) );
				*header = NULL;
				return NULL;
			}
			else if ( type == ZTABLE_TXT ) {
				*header = strdup( kv->value.v.vchar );
				return (void *)1;
			}
			else if ( type == ZTABLE_TBL ) {
				int tblen = 0;
				// Extract these keys...
				for ( int ci = 1, size = lt_counti( conf, i + ii ); ci <= size; ci++ ) {
					int ctype = lt_vta( conf, i + ii + ci );
					zKeyval *xv = lt_retkv( conf, i + ii + ci );
					// TODO: Trying to manually use snprintf has been a problem.  This is yet another spot to fix...
					if ( lt_kta( conf, i + ii + ci ) == ZTABLE_INT && ctype == ZTABLE_TXT )
						tblen += snprintf( &tmpbuf[ tblen ], sizeof( tmpbuf ) - tblen - 1, "%s, ", xv->value.v.vchar );
					else if ( ctype == ZTABLE_INT ) 
						tblen += snprintf( &tmpbuf[ tblen ], sizeof( tmpbuf ) - tblen - 1, "%s=%d, ", xv->key.v.vchar, xv->value.v.vint );
					else if ( ctype == ZTABLE_FLT ) 
						tblen += snprintf( &tmpbuf[ tblen ], sizeof( tmpbuf ) - tblen - 1, "%s=%f, ", xv->key.v.vchar, xv->value.v.vfloat );
					else if ( ctype == ZTABLE_TXT && xv->value.v.vchar ) { 
						if ( strcmp( xv->value.v.vchar, "true" ) )
							tblen += snprintf( &tmpbuf[ tblen ], sizeof( tmpbuf ) - tblen - 1, "%s, ", xv->key.v.vchar );
						else {
							tblen += snprintf( &tmpbuf[ tblen ], sizeof( tmpbuf ) - tblen - 1, "%s=%s, ", xv->key.v.vchar, xv->value.v.vchar );
						}
					}
					else {
						const char fmt[] = "Misconfiguration at key 'cache.%s.*'.  Expected simple value, got %s.";
						snprintf( err, errlen, fmt, kv->key.v.vchar, lt_typename( ctype ) );
						*header = NULL;
						return NULL;
					}
				}
				if ( tblen > 2 ) {
					tmpbuf[ tblen - 2 ] = '\0';
					*header = strdup( tmpbuf );
				}
				return (void *)1;
			}
		}
	}
#endif
	return (void *)1;
}


#ifndef DEBUG_H
 #define find_key(z,k) ( lt_geti( z, k ) > -1 )
#else
int find_key( ztable_t *conf, const char *key ) {
	FPRINTF( "--- Searching for %s in config => %p\n", key, conf );
	return lt_geti( conf, key ) > -1;
}
#endif



