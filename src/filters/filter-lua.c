/**
 * filter-lua.c
 * =============
 *
 * Summary 
 * -------
 * The Lua filter and all the functions that comprise it.
 *
 *
 * More
 * ----
 * TBD
 *
 *
 * License
 * -------
 * See LICENSE in the top-level directory for more information.
 *
 */
#include "filter-lua.h"

#define DYLIB ".so"

static const char rname[] = "route";

static const char def[] = "default";

static const char confname[] = "config.lua";

static const char configkey[] = "config";

static const char modelkey[] = "model";

static const char rkey[] = "routes";

static const char ctype_def[] = "text/html";

static const char extfmt[] = "%s;%s/?;%s/?.lua";

static const char libcfmt[] = "%s;%s/lib/?" DYLIB;


typedef enum {
	CTYPE_TEXTHTML
,	CTYPE_PLAINTEXT
, CTYPE_JSON
, CTYPE_XML
} ctypen_t;



typedef struct ctype_t {
	const char *ctypename;
	ctypen_t ctype;
} ctype_t;



ctype_t ctypes_serializable[] = {
	{ ctype_def, CTYPE_TEXTHTML }
,	{ "text/plain", CTYPE_PLAINTEXT }
,	{ "application/json", CTYPE_JSON }
, { "application/xml", CTYPE_XML }
, { "text/xml", CTYPE_XML }
, { NULL }
};



static const char *ctype_tags[] = {
	"ctype"
, "content-type"
, "contenttype"
, NULL
};


//TODO: Change me to accept a function pointer...
static struct mvcmeta_t {
	const char *dir;
	const char *ext;
	const char *reserved;
	int (*fp)( int );
} mvcmeta [] = {
	{ "app", "lua", "model,models" }
,	{ "sql", "sql", "query,queries" }
,	{ "views", "tpl", "view,views" }
,	{ NULL, NULL, "content-type" }
//,	{ NULL, "inherit", NULL }
};


static const confkey_t keys[] = {
	{ "cache",    ZTABLE_TBL, 0, get_cache_header },
	/* TODO: Support this: ZTABLE_TBL || ZTABLE_TXT */
	{ "disallow", ZTABLE_TBL, 0, get_disallowed_paths }, 
	{ "db", ZTABLE_TXT, 0, NULL },
	{ "fqdn", ZTABLE_TXT, 0, NULL },
	{ "static", ZTABLE_TBL, 0, NULL },
	{ "title", ZTABLE_TBL, 0, NULL },
	#if 0
	{ "redirect", ZTABLE_TBL, 0, get_redirect },
	{ "shadow", ZTABLE_TXT, 0, NULL }, // Use a different shadow directory
	{ "map", ZTABLE_TBL, 0, NULL }, // Map directories to other places
	{ "routes", ZTABLE_TBL, 0, get_redirect }, // Can't require this
	#endif
	{ NULL },
};


/**
 * int lua_loadlibs( lua_State *L, struct lua_fset *set )
 *
 * Load the non-standard Lua libraries (i.e. everything in lux)
 *
 */
int lua_loadlibs( lua_State *L, struct lua_fset *set ) {
	//Now load everything written elsewhere...
	for ( ; set->namespace; set++ ) {
		lua_newtable( L );
		for ( struct luaL_Reg *f = set->functions; f->name; f++ ) {
			lua_pushstring( L, f->name );
			lua_pushcfunction( L, f->func );	
			lua_settable( L, 1 );
		}
		lua_setglobal( L, set->namespace );
	}

#if 0
	//And finally, add some functions that we'll need later (if this fails, meh)
	if ( !run_lua_buffer( L, read_only_block ) ) {
		return 0;
	}
#endif
	return 1;
}



/**
 * static int is_reserved( const char *a )
 *
 * Check if there is a reserved keyword being requested.
 *
 */
static int is_reserved( const char *a ) {
	for ( int i = 0; i < sizeof( mvcmeta ) / sizeof( struct mvcmeta_t ); i ++ ) {
#if 0
		if ( memstrat( mvcmeta[i].reserved, a, strlen( mvcmeta[i].reserved ) ) > -1 ) {
			return 1;
		}
#else
		zWalker w = {0};
		for ( ; strwalk( &w, mvcmeta[i].reserved, "," ); ) {
	//int sl = strlen( (char *)mvcmeta[ i ].reserved );
		//for ( ; memwalk( &w, (unsigned char *)mvcmeta[i].reserved, (unsigned char *)",", sl, 1 ); ) {
#if 0
fprintf( stderr,
	"POS: %d Size: %d Len: %ld Next: %d\n",
	w.pos, w.size, strlen(mvcmeta[i].reserved), w.next ); getchar();
#endif
			char buf[64];
			memset( buf, 0, sizeof( buf ) );
			memcpy( buf, w.src, ( w.chr == ',' ) ? w.size - 1 : w.size );
			if ( strcmp( a, buf ) == 0 ) return 1;
		}
#endif
	}
	return 0;
}



/**
 * static int make_route_list ( zKeyval *kv, int i, void *p )
 *
 * Make a route list.
 *
 */
static int make_route_list ( zKeyval *kv, int i, void *p ) {
	struct route_t *tt = (struct route_t *)p;
	const int routes_wordlen = 6;
	if ( kv->key.type == ZTABLE_TXT && !is_reserved( kv->key.v.vchar ) ) {
		char key[ 2048 ] = { 0 };
		lt_get_full_key( tt->src, i, (unsigned char *)&key, sizeof( key ) );
		//replace all '.' with '/'
		for ( char *k = key; *k; k++ ) ( *k == '.' ) ? *k = '/' : 0;	
		struct iroute_t *ii = malloc( sizeof( struct iroute_t ) );
		ii->index = i, ii->route = zhttp_dupstr( &key[ routes_wordlen ] ), *ii->route = '/';
		add_item( &tt->iroute_tlist, ii, struct iroute_t *, &tt->iroute_tlen );
	}
	return 1;	
}



/**
 * static int make_mvc_list ( zKeyval *kv, int i, void *p )
 *
 * Create a list of resources (an alternate version of this will inherit everything)
 *
 */
static int make_mvc_list ( zKeyval *kv, int i, void *p ) {
	struct mvc_t *tt = (struct mvc_t *)p;
	char *key = NULL;
	int ctype = 0;

	if ( tt->depth == 1 ) {
		if ( kv->key.type == ZTABLE_TXT && is_reserved( key = kv->key.v.vchar ) ) {
			if ( !strcmp( key, "model" ) || !strcmp( key, "models" ) )
				tt->mset = &mvcmeta[ 0 ], tt->type = kv->value.type, tt->model = 1;
			else if ( !strcmp( key, "query" ) )
				tt->mset = &mvcmeta[ 1 ], tt->type = kv->value.type, tt->query = 1;
			else if ( !strcmp( key, "content-type" ) )
				tt->mset = &mvcmeta[ 3 ], tt->type = kv->value.type, ctype = 1;
			else if ( !strcmp( key, "view" ) || !strcmp( key, "views" ) ) {
				tt->mset = &mvcmeta[ 2 ], tt->type = kv->value.type, tt->view = 1;
			}
		}

		//write content type
		if ( tt->mset && ctype ) {
			memcpy( (char *)tt->ctype, kv->value.v.vchar, strlen( kv->value.v.vchar ) );
			return 1;
		}
	}

	if ( kv->value.type == ZTABLE_TBL ) {
		tt->depth++;
		return 1;
	}

	if ( tt->mset && kv->value.type == ZTABLE_TXT && memchr( "mvq", *tt->mset->reserved, 3 ) ) {
		struct imvc_t *imvc = malloc( sizeof( struct imvc_t ) );
		memset( imvc, 0, sizeof( struct imvc_t ) );
		snprintf( (char *)imvc->file, sizeof(imvc->file) - 1, "%s/%s.%s",
			tt->mset->dir, kv->value.v.vchar, tt->mset->ext );
		snprintf( (char *)imvc->base, sizeof(imvc->base) - 1, "%s.%s",
			kv->value.v.vchar, tt->mset->ext );
		snprintf( (char *)imvc->ext, sizeof(imvc->ext) - 1, "%s",
			tt->mset->ext );
		add_item( &tt->imvc_tlist, imvc, struct imvc_t *, &tt->flen );
	}

	if ( kv->key.type == ZTABLE_TRM || tt->type == ZTABLE_TXT ) {
		tt->mset = NULL;	
	}
	if ( kv->key.type == ZTABLE_TRM ) {
		tt->depth--;
	}
	return 1;	
}



/**
 * static void free_mvc_list ( void ***list )
 *
 * Free MVC list.
 *
 */
static void free_mvc_list ( void ***list ) {
	for ( void **l = *list; l && *l; l++ ) {
		free( *l );
	}
	free( *list ), *list = NULL;
}



/**
 * static void free_route_list ( struct iroute_t **list )
 *
 * Free the route list.
 *
 */
static void free_route_list ( struct iroute_t **list ) {
	for ( struct iroute_t **l = list; *l; l++ ) {
		free( (*l)->route ), free( *l );
	}
	free( list );
}



/**
 * static struct dirent * dir_has_files ( DIR *dir )
 *
 * Check if a directory contains ANY files, returning NULL if there are none.
 *
 */
static struct dirent * dir_has_files ( DIR *dir ) {
	int fcount = 0;
	struct dirent *d = NULL;
	for ( int dlen ; ( d = readdir( dir ) ); ) {
		if ( ( dlen = strlen( d->d_name ) > 4 ) && strstr( d->d_name, ".lua" ) ) {
			//TODO: Will this need to happen b/c it's a different scope?
			rewinddir( dir );
			return d;
		}
	}
	return d;
}



/**
 * static int load_lua_config( struct luadata_t *l )
 *
 * Loads the instance's configuration file.
 *
 */
static int load_lua_config( struct luadata_t *l ) {
	char *db, *fqdn, cpath[ 2048 ] = { 0 };
	DIR *dir = NULL;
	struct dirent *d = NULL;
	struct stat sb = { 0 };
	ztable_t *t = NULL;
	int count = 0;

	//Create a full path to the config file
	snprintf( cpath, sizeof(cpath) - 1, "%s/%s", l->root, confname );

	//Set shadow path
	lua_pushstring( l->state, l->root );
	lua_setglobal( l->state, CKEY_SHADOW );

	//stat
	if ( stat( cpath, &sb ) == -1 ) {
		char terr[ 1024 ] = {0};
		strerror_r( errno, terr, sizeof( terr ) );
		snprintf( l->err, LD_ERRBUF_LEN, "Failed to stat() file: %s: %s", cpath, terr );
		return 0;
	}

	//zero-length isn't allowed here...
	if ( !sb.st_size ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Configuration file for instance at path %s is zero-length.", l->root );
		return 0;
	}

	//open and execute, catching and reporting back any errors
	if ( !lua_exec_file( l->state, cpath, l->err, LD_ERRBUF_LEN ) ) {
		return 0;
	}

	//If it's anything but a Lua table, we're in trouble
	if ( !lua_istable( l->state, 1 ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Configuration is not a Lua table." );
		return 0;
	}

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

	//Initialize a table of the right size
	if ( !( t = lt_make( count * 2 ) ) || !lua_to_ztable( l->state, 1, t ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Conversion of config to ztable failed." );
		return 0;
	}

	//Lock?
	lt_lock( l->zconfig = t );

	//TODO: use pointers instead.  There is no reason to copy all of this...
	if ( ( db = lt_text( t, "db" ) ) ) {
		memcpy( (void *)l->db, db, strlen( db ) );
	}

	if ( ( fqdn = lt_text( t, "fqdn" ) ) ) {
		memcpy( (void *)l->fqdn, fqdn, strlen( fqdn ) );
	}

	lua_pop( l->state, 1 );
	return 1;
}




/**
 * static int path_is_static ( struct luadata_t *l )
 *
 * Check for static paths.
 *
 */
static int path_is_static ( struct luadata_t *l ) {
	int i, size, ulen = strlen( l->req->path );
	if ( ( i = lt_geti( l->zconfig, "static" ) ) == -1 ) {
		return 0;
	}
	
	//Start at the pulled index, count x times, and reset?
	for ( int len, ii = 1, size = lt_counta( l->zconfig, i ); ii < size; ii++ ) {
		zKeyval *kv = lt_retkv( l->zconfig, i + ii );
		//pbuf[ ii - 1 ] = kv->value.v.vchar;
		len = strlen( kv->value.v.vchar );

		//I think I can just calculate the current path
		if ( len <= ulen && memcmp( kv->value.v.vchar, l->req->path, len ) == 0 ) {
			return 1;
		}
	}

	return 0;
}


/**
 * static int init_lua_routes ( struct luadata_t *l )
 *
 * Initialize the routes list
 *
 */
static int init_lua_routes ( struct luadata_t *l ) {
	zWalker w = {0}, w2 = {0};
	const char *active = NULL, *path = l->apath + 1, *resolved = l->rroute + 1;
	char **routes = { NULL };
	int index = 0, rlen = 0, pos = 1;
	
	//Add a table.
	lua_newtable( l->state );

	//Handle root requests
	if ( !*path ) {
		lua_pushinteger( l->state, 1 );
		lua_pushstring( l->state, def );
		lua_settable( l->state, pos );

		lua_pushstring( l->state, "active" );
		lua_pushstring( l->state, def );
		lua_settable( l->state, pos );

		memcpy( (void *)l->aroute, def, strlen (def) );
		return 1;
	}
	
	// Loop twice to set up the map
	for ( ; strwalk( &w, path, "/" ); ) {
		// ...
		char stub[ 1024 ] = {0};
		char id[ 1024 ] = {0};

		// Write the length of the block between '/'
		memset( stub, 0, sizeof(stub) );
		memcpy( stub, w.src, ( w.chr == '/' ) ? w.size - 1 : w.size );

		for ( ; strwalk( &w2, resolved, "/" ); ) {
			int size = ( w2.chr == '/' ) ? w2.size - 1 : w2.size;
			// If there is an equal, most likely it's an id
			if ( *w2.src != ':' )
				lua_pushinteger( l->state, ++index );	
			else {
				// Find the key/id name
				for ( char *p = (char *)w2.src, *b = id; *p && ( *p	!= '=' || *p != '/' ); ) {
					*(b++) = *(p++);
				}

				// Check that id is not active, because that's a built-in
				if ( strcmp( id, "active" ) == 0 ) {
					return 0;
				}
				
				// Add a numeric key first, then a text key
				lua_pushinteger( l->state, ++index );
				lua_pushstring( l->state, stub );
				lua_settable( l->state, pos );
				lua_pushstring( l->state, id );
			}
			break;
		}

		//Copy the value (stub) to value in table
		lua_pushstring( l->state, stub );
		lua_settable( l->state, pos );
		active = &path[ w.pos ];
	}

	lua_pushstring( l->state, "active" );
	lua_pushstring( l->state, active );
	lua_settable( l->state, pos );
	memcpy( (void *)l->aroute, active, strlen (active) );
	return 1;
}


/**
 * int init_lua_request ( struct luadata_t *l )
 *
 * Creates the global "request" key and associated table data.
 *
 */
static int init_lua_request ( struct luadata_t *l ) {
	//Loop through all things
	const char *str[] = { "headers", "url" };
	struct HTTPRecord **ii[] = { l->req->headers, l->req->url, l->req->body };

	//Add one table for all structures
	lua_newtable( l->state );

	//Add general request info
	lua_setstrstr( l->state, "path", l->req->path, 1 );
	lua_setstrstr( l->state, "method", l->req->method, 1 );
	lua_setstrstr( l->state, "protocol", l->req->protocol, 1 );
	lua_setstrstr( l->state, "host", l->req->host, 1 );
	lua_setstrstr( l->state, "ipv4", l->ipv4, 1 );

	//In some cases browsers omit the content-type
	if ( !strcmp( l->req->method, "GET" ) && !strcmp( l->req->ctype, "application/octet-stream" ) )
		lua_setstrstr( l->state, "ctype", "text/html", 1 );
	else {
		lua_setstrstr( l->state, "ctype", l->req->ctype, 1 );
	}

	//If the method is NOT idempotent, don't bother with content-length
	if ( l->req->idempotent )
		lua_setstrint( l->state, "clength", l->req->clen, 1 );

	//Add simple keys for headers and URL
	for ( int pos = 3, i = 0; i < 2; i++ ) {
		struct HTTPRecord **r = ii[ i ];
		if ( r && *r ) {
			lua_pushstring( l->state, str[i] ), lua_newtable( l->state );
			for ( ; r && *r; r++ ) {
			#if 0	
				if ( strcmp( "Cookie", (*r)->field ) != 0 )
					lua_pushstring( l->state, (*r)->field );
				else {	
					char *f = (char *)(*r)->field;
					*f = ( *f > 63 && *f < 91 ) ? *f + 32 : *f;
					lua_pushstring( l->state, f );
				}
			#else
				char *f = (char *)(*r)->field;
				*f = ( *f > 63 && *f < 91 ) ? *f + 32 : *f;
				lua_pushstring( l->state, f );
			#endif

				//Add the lower case version of whatever the header title may be
				lua_newtable( l->state );
				lua_setstrbin( l->state, "value", ( char * )(*r)->value, (*r)->size, pos + 2 );
				lua_setstrint( l->state, "size", (*r)->size, pos + 2 );

			#if 0
				//For now, we only need to worry with authentication and cookies
				if ( strcmp( "cookie", (*r)->field ) == 0 ) {

					zw_t ww, *w = memset( &ww, 0, sizeof( zw_t ) );
					//int count = 0;
					for ( int x = 0, p = 0; memwalk( w, (*r)->value, (unsigned char *)"=;", (*r)->size, 2 ); ) {

						//Get size and initialize a buffer
						unsigned char *b, buf[ 256 ] = {0};
						int si = 0, size = memchr( "=;", w->chr, 2 ) ? w->size - 1 : w->size;

						//Die on sizes that are too large
						if ( size >= 256 ) {
							snprintf( l->err, LD_ERRBUF_LEN, "Header %s too large.", i ? "value" : "key" );
							return 0;
						}

						//Trim any excess from current value
						b = trim( w->src, "\r\"' \t", size, &si );	
						if ( si > 0 ) {
							memcpy( buf, b, si );
							if ( w->chr == '=' )
								lua_pushstring( l->state, (char *)buf ), p++;
							else if ( !p && w->chr == ';' )	{
								lua_pushnumber( l->state, x++ ), lua_pushstring( l->state, (char *)buf );
								lua_settable( l->state, pos + 2 );
							}
							else {
								lua_pushstring( l->state, (char *)buf );
								lua_settable( l->state, pos + 2 );
								p = 0;
							}
						}
					}
				}
			#endif
				lua_settable( l->state, pos );
			}
			lua_settable( l->state, 1 );
		}
	}

	//We gotta do the body now	
	struct HTTPRecord **b;
	if ( ( b = l->req->body ) ) {
		lua_pushstring( l->state, "body" ), lua_newtable( l->state );
		if ( l->req->formtype == ZHTTP_OTHER ) {
			lua_setstrbin( l->state, "value", (*b)->value, (*b)->size, 3 );
			lua_setstrint( l->state, "size", (*b)->size, 3 );
		}
		else {
			for ( ; b && *b; b++ ) {
				lua_pushstring( l->state, (*b)->field );
				lua_newtable( l->state );
				lua_setstrbin( l->state, "value", (*b)->value, (*b)->size, 5 );
				lua_setstrint( l->state, "size", (*b)->size, 5 );
			#if 0
				//Content-disposition
				//Filename?
				//Any other fields?
			#endif
				lua_settable( l->state, 3 );
			}
		}
		lua_settable( l->state, 1 );			
	}	

	//Set global name
	return 1;
}


static int init_lua_shadowpath ( struct luadata_t *l ) {
	lua_pushstring( l->state, l->root );
	return 1;	
}


static int init_lua_config ( struct luadata_t *l ) {
	return ztable_to_lua( l->state, l->zconfig );
}


/**
 * TODO:
 * struct lua_readonly_t
 *
 * Initializes global elements that should show up in MOST requests made through the Lua handler.
 *
 * This is not quite functional yet, but the idea is going the right way.
 * Calling (*exec)(...) should fill in any tables that are expected to be
 * used in the global scope.
 *
 * Right now, this list is:
 * config - A Lua table representation of everything in the instance config file (config.lua).
 * request - The headers, IP address and body of the request received
 * route - The completed part, and route parts of the current request.
 *
 * This should also include:
 * shadow - The "shadow" path, or the root file path accessible to this instance of Lua.
 * date - Access to the os.date primitives from anywhere within the context of the current request
 * session - Access to a local or network datastore with the intention of persisting data
 * cache - A list of objects that should be cached, also possibly handled via local or network datastore.
 *
 */
//Data to initialize global elements
static struct lua_readonly_t {
	const char *name;
	int (*exec)( struct luadata_t * );
	//int (*exec)( lua_State *, zhttp_t *, const char *, const char * );	
} lua_readonly[] = {
  { "config", init_lua_config }
, { "request", init_lua_request }
, { "route", init_lua_routes }
#if 0
, { "shadow", init_lua_shadowpath }
, { "cache", init_lua_cache }
#endif
, { NULL }
};



/**
 * static int free_ld ( struct luadata_t *l )
 *
 * Destroy the Lua data structure.
 *
 */
static int free_ld ( struct luadata_t *l ) {
	lua_close( l->state );
	lt_free( l->zconfig ), free( l->zconfig );
	lt_free( l->zroute ), free( l->zroute );
	lt_free( l->zmodel ), free( l->zmodel );
	free_mvc_list( (void ***)&(l->pp.imvc_tlist) );
	return 1;
}



/**
 * static char * text_encode ( ztable_t *t )
 *
 * TODO: Come up with a better way to handle this.
 *
 */
static char * text_encode ( ztable_t *t ) {
	char *c = malloc( 1 );
	*c = '\0';
	return c;
}



/**
 * static zhttp_t * return_as_serializable ( struct luadata_t *l, ctype_t *t )
 *
 * Return content as a serializable type.
 *
 */
static zhttp_t * return_as_serializable ( struct luadata_t *l, ctype_t *t ) {
	char * content = NULL;
	const char *ctype = NULL;
	int clen = 0;
	zhttp_t *p = NULL;
	
	if ( 0 ) { ; }
	#if 0
	else if ( t->ctype == CTYPE_XML ) {
		content = xml_encode( l->zmodel, "model" );
		clen = strlen( content );
		ctype = t->ctypename;
	}
	#endif
	else if ( t->ctype == CTYPE_JSON ) {
	#if 0
		content = zjson_encode( l->zmodel, l->err, 1024 );
	#else
		struct mjson **zjson = NULL;
		if ( !( zjson = ztable_to_zjson( l->zmodel, l->err, 1024 ) ) ) {
			return NULL;
		}
		if ( !( content = zjson_stringify( zjson, l->err, 1024 ) ) ) {
			zjson_free( zjson );
			return NULL;
		}
		zjson_free( zjson );
	#endif
		clen = strlen( content );
		ctype = t->ctypename;
	}
	else {
		//TODO: This should handle the other types...
		content = text_encode( l->zmodel );
		clen = strlen( content );
		ctype = "text/plain";
	}

	l->res->clen = clen;
	http_set_status( l->res, 200 );
	http_set_ctype( l->res, t->ctypename );
	http_set_content( l->res, (unsigned char *)content, l->res->clen );

	//Return the finished message if we got this far
	p = http_finalize_response( l->res, l->err, LD_ERRBUF_LEN );

	free( content );
	return p;
}



/**
 * static int return_as_response ( struct luadata_t *l )
 *
 * Return the completed operations as a response versus an HTML (or other visual markup) page.
 *
 */
static int return_as_response ( struct luadata_t *l ) {

	ztable_t *rt = NULL;
	int count = 0, status = 200, clen = 0;
	int header_i = 0;
	int status_i = 0;
	int ctype_i = 0;
	int clen_i = 0;
	int file_i = 0;
	int prepped_own_content = 0;
	int content_i = 0;
	int delayed = 0;
	char ctype[ 128 ] = { 0 }; //'t','e','x','t','/','h','t','m','l','\0', 0 };
	unsigned char *content = NULL;

#if 1
	count = 512;
#else
	//Get the count to approximate size of conversion needed (and to handle blanks)
	count = lua_count( l->state, 1 );
#endif
	
	if ( !lua_istable( l->state, 1 ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Response is not a table." );
		return 0;
	}

	if ( !( rt = lt_make( count * 2 ) ) ) {
		lt_free( rt ), free( rt );
		snprintf( l->err, LD_ERRBUF_LEN, "Could not generate response table." );
		return 0;
	}

	if ( !lua_to_ztable( l->state, 1, rt ) ) {
		lt_free( rt ), free( rt );
		snprintf( l->err, LD_ERRBUF_LEN, "Error in model conversion." );
		return 0;
	}

	//Lock the ztable to enable proper hashing and collision mgmt
	if ( !lt_lock( rt ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "%s", lt_strerror( rt ) );
		return 0;
	}

	//Check if the user wants to delay the response.
	if ( lt_geti( rt, "delay" ) > -1 ) {
		delayed = 1;
	}

	//Get the status
	if ( ( status_i = lt_geti( rt, "status" ) ) > -1 ) {
		status = lt_int_at( rt, status_i );
	}
	
	//Get the content-type (if there is one)
	if ( ( ctype_i = lt_geti( rt, "ctype" ) ) > -1 ) {
		snprintf( ctype, sizeof( ctype ) - 1, "%s", lt_text_at( rt, ctype_i ) );
	}

	//Get the content-length (if there is one)
	if ( ( clen_i = lt_geti( rt, "clen" ) ) > -1 ) {
		clen = lt_int_at( rt, clen_i );
	}

	//Get the content
	if ( !delayed && ( content_i = lt_geti( rt, "content" ) ) > -1 ) {
		content = (unsigned char *)lt_text_at( rt, content_i );
		if ( clen_i == -1 ) {
			if ( !content ) {
				// TODO: A stack trace showing where exactly the lack of content came from would be very helpful
				prepped_own_content = 1;
				char *c = malloc( 256 );
				memset( c, 0, 256 );
				snprintf( c, 255, "%d %s - No content specified\n", status, http_get_status_text( status ) );
				content = (unsigned char *)c;
			}
			clen = strlen( (char *)content );
		}
	}

	//In this case, set clen with the file
	if ( !delayed && ( file_i = lt_geti( rt, "file" ) ) > -1 ) {
		const char * fname = lt_text_at( rt, file_i );
		char fbuf[ PATH_MAX ];
		int len = 0;
		memset( fbuf, 0, PATH_MAX );

		//Do I need a shadow?
		snprintf( fbuf, sizeof( fbuf ) - 1, "%s/%s", l->root, fname );

		//Problem reading the file
		if ( !( content = read_file( fbuf, &len, l->err, LD_ERRBUF_LEN ) ) )  {
			lt_free( rt ), free( rt );
			return 0;
		}

		//const struct mime_t *mime = zmime_get_by_filename( fbuf );
		snprintf( ctype, sizeof( ctype ) - 1, "%s", zmime_get_mimetype( zmime_get_by_filename( fbuf ) ) );
		clen = len;
	}

	//Set content type to default if it was not set anywhere else
	if ( *ctype == 0 ) {
		snprintf( ctype, sizeof( ctype ) - 1, "%s", ctype_def );
	}

	//Likewise, if all we have is status, and no file or content then
	//you need to error with a 500
	if ( status_i > -1 && file_i == -1 && content_i == -1 ) {
		lt_free( rt ), free( rt );
		snprintf( l->err, LD_ERRBUF_LEN, "Status specified with no content." );
		return 0;
	}

	//Finally, get any headers if there are any
	if ( ( header_i = lt_geti( rt, "headers" ) ) > -1 ) {
		//You can use get keys or loop through the thing...
		for ( zKeyval *kv = lt_items( rt, "headers" ); ( kv = lt_items( rt, "headers" ) ); ) {
			if ( kv->key.type == ZTABLE_TRM )
				break;
			if ( kv->key.type	!= ZTABLE_TXT && (  kv->value.type	!= ZTABLE_TXT && kv->value.type != ZTABLE_INT ) ) {
				snprintf( l->err, LD_ERRBUF_LEN, "Got invalid header value." );
				return 0; // die
			}

			if ( kv->value.type == ZTABLE_TXT )
				http_set_header( l->res, kv->key.v.vchar, kv->value.v.vchar );
			else if ( kv->value.type == ZTABLE_INT ) {
				char intbuf[ 64 ] = {0};
				snprintf( intbuf, sizeof( intbuf ), "%d", kv->value.v.vint );
				http_set_header( l->res, kv->key.v.vchar, intbuf );
			}
		}
	}

	if ( !delayed ) {
		//Set structures
		l->res->clen = clen;
		http_set_status( l->res, status );
		http_set_ctype( l->res, ctype );
		http_set_content( l->res, content, clen );

		//Return finalized content
		zhttp_t *rr = http_finalize_response( l->res, l->err, LD_ERRBUF_LEN );
		lt_free( rt ), free( rt );
		if ( file_i > -1 || prepped_own_content ) {
			free( content );
		}
	}

	return ( !delayed ) ? 1: 2;
}


//Compare the path against the instance routes
int find_matching_route ( struct luadata_t *l ) {
	ztable_t *t = NULL;
	struct route_t p =  { 0 };

	if ( lt_geti( l->zconfig, "routes" ) > -1 ) {
		//Create a mini table
		p.src = t = lt_copy_by_key( l->zconfig, "routes" );

		//Loop through the routes...
		lt_exec_complex( t, 1, t->count, &p, make_route_list );

		//Loop through the routes
		l->pp.depth = 1;
		int notfound = 1;
		for ( struct iroute_t **lroutes = p.iroute_tlist; *lroutes; lroutes++ ) {
			if ( route_resolve( l->apath, (*lroutes)->route ) ) {
				memcpy( (void *)l->rroute, (*lroutes)->route, strlen( (*lroutes)->route ) );
				ztable_t * croute = lt_copy_by_index( t, (*lroutes)->index );
				lt_exec_complex( croute, 1, croute->count, &l->pp, make_mvc_list );
				l->zroute = croute;
				notfound = 0;
				lt_free( t ), free( t );
				break;
			}
		}

		//Free the route list
		free_route_list( p.iroute_tlist );
		return !notfound;
	}

	return 0;
}


int has_views( struct imvc_t **list ) {
	for ( struct imvc_t **l = list; l && *l; l++ ) {
		if ( *(*l)->file == 'v' ) return 1;
	}
	return 0;
}



/**
 * char *pdirname ( char *path )
 *
 * Returns the directory name of a path in [dirpath].
 *
 */
static char *pdirname ( const char *path, char *dirpath, int dplen ) {
	int len = strlen( path );
	char *p = NULL;

	// If too big, stop
	if ( len > dplen ) {
		return NULL;
	}

	// Still count from the back
	p = (char *)path + len;
	for ( ; len > 0 && *p != '/'; p--, --len );

	// If *p == '/', copy to dirpath, otherwise return p
	if ( *p == '/' ) {
		memcpy( dirpath, path, len );
		return dirpath;
	}

	return p;
}


/**
 * static char *prnodes( const char *path, char *dirpath, int dplen )
 *
 * Returns the part of a node (or string) that succeeds a specific character.
 *
 */
static char *prnodes( const char *src, char *dest, int dplen, char c, int count, int keep ) {

	int len = strlen( src );
	int clen = 0;
	int nodescount = 0;
	char *p = (char *)src;
	char *d = dest;

	// If either is blank, stop
	if ( !src || !dest ) {
		return NULL;
	}

	// If too big, stop
	if ( len > dplen ) {
		return NULL;
	}

	// If count is not positive stop
	if ( count < 1 ) {
		return NULL;
	}

#if 0
	// Move through the src and trim to only return what we asked for.
	for ( ; len > 0 && *p; p++, len-- ) {
		if ( *p == c ) {
			if ( ++nodescount == count ) break;
		}
		*(d++) = *p;
	}
#endif

	// Still count from the back
	p = (char *)src + len;
	for ( ; len > 0; --p, --len, ++clen ) {
		if ( *p == c ) {
			if ( ++nodescount == count ) break;
		}
	}

	// If len == 0, return src
	if ( len == 0 ) {
		return (char *)src;
	}

	// If !keep, modify
	if ( !keep ) {
		p++, clen--;
	}

	// If *p == '/', copy to dirpath, otherwise return p
	memcpy( dest, p, clen );
	return dest;
}




/**
 * static char *pnodes( const char *path, char *dirpath, int dplen )
 *
 * Returns the part of a node (or string) that precedes a specific character.
 *
 */
static char *pnodes( const char *src, char *dest, int dplen, char c, int count, int lead ) {

	int len = strlen( src );
	int nodescount = 0;
	char *p = (char *)src;
	char *d = dest;

	// If either is blank, stop
	if ( !src || !dest ) {
		return NULL;
	}

	// If too big, stop
	if ( len > dplen ) {
		return NULL;
	}

	// If count is not positive stop
	if ( count < 1 ) {
		return NULL;
	}

	// Move through the src and trim to only return what we asked for.
	for ( ; len > 0 && *p; p++, len-- ) {
		*(d++) = *p;
		if ( *p == c ) {
			if ( ++nodescount == count ) break;
		}
	}

	if ( !lead ) {
		*(--d) = '\0';
	}

	return dest;
}



/**
 * char *peval( struct luadata_t *ld, struct imvc_t *m )
 *
 * Evaluates special characters in route listings.
 *
 */
char *peval( char *dest, int dlen, struct luadata_t *ld, struct imvc_t *m ) {
	// This should all be split out now...
	char fname[ PATH_MAX ];
	memset( fname, 0, PATH_MAX );
	int fnlen = 0;

	// Copy each over
	for ( char *f = fname, *mm = (char *)m->file; fnlen < sizeof( fname ) && *mm; ) {
		*f = *mm;
		int len = 0;

		// Do a simple replacement with the active route name
		if ( *f == '@' ) {
			len = strlen( ld->aroute );
			memcpy( f, ld->aroute, len );
			f += len, mm++, fnlen += len;
			continue;
		}

		// Do a simple replacement with the root node of "dirname"
		else if ( *f == '^' ) {
			//multiple $'s define how far up to look at the chain for the replacement
			char *dp = NULL, dirname[ PATH_MAX ];
			memset( dirname, 0, PATH_MAX );
			if ( !( dp = pnodes( ld->rroute, dirname, PATH_MAX, '/', 2, 0 ) ) ) {
			// sET ERROR STRING
				return NULL;
			}
			dp++;
			len = strlen( dp );
			memcpy( f, dp, len );
			f += len, mm++, fnlen += len;
			continue;
		}

		// Do a simple replacement with the "dirname" of the current route path
		else if ( *f == '#' ) {
			int count = 0;
			char *dp = NULL, dirname[ PATH_MAX ];
			memset( dirname, 0, PATH_MAX );
	#if 0
			if ( memcmp( f, "###", 3 )	)
				count = 3;
			else if ( memcmp( f, "##", 2 )	)
				count = 2;
			else {
				count = 1;
			}
	#endif

			if ( !( dp = pdirname( ld->rroute, dirname, PATH_MAX ) ) ) {
			// sET ERROR STRING
				return NULL;
			}
			dp++;
			len = strlen( dp );
			memcpy( f, dp, len );
			f += len, mm++, fnlen += len;
			continue;
		}

		f++, mm++, fnlen++;
	}

	snprintf( dest, dlen, "%s/%s", ld->root, fname );
	return dest;
}




/**
 * const int filter_lua( const server_t *serv, conn_t *conn )
 *
 * The entry point for a Lua application.
 *
 */
const int filter_lua( const server_t *serv, conn_t *conn ) {

	// Define variables and error positions...
	int ccount = 0;
	int clen = 0;
	int model = 0;
	int tcount = 0;
	int view = 0;
	struct luadata_t ld = {0};
	struct lconfig *host = conn->config;
	ztable_t zc = {0};
	ztable_t zm = {0};
	unsigned char *content = NULL;

	// Initialize the request address buffer
	snprintf( ld.ipv4, LD_IPV4_LEN, "%s", conn->ipv4 );

	// Prepare the response
	memset( conn->res, 0, sizeof( zhttp_t ) );

	// Initialize Lua data structure
	ld.req = conn->req;
	ld.res = conn->res;
	ld.res->atype = ZHTTP_MESSAGE_MALLOC;

	// Check that the directory was specified
	if ( !host->dir ) {
		return http_error( conn->res, 500, "%s", "No directory path specified for this site." );
	}

	// Then copy it over
	memcpy( (void *)ld.root, host->dir, strlen( host->dir ) );

	// Then initialize the Lua state
	if ( !( ld.state = luaL_newstate() ) ) {
		return http_error( conn->res, 500, "%s", "Failed to initialize Lua environment." );
	}

	// Load the standard libraries first
	luaL_openlibs( ld.state );

	// Then load the extensions
	if ( !lua_loadlibs( ld.state, functions ) ) {
		free_ld( &ld );
		return http_error( conn->res, 500, "Failed to initialize Lua standard libs." );
	}

	// Set custom package path (in addition to Lua's regular include path)
	if ( lua_retglobal( ld.state, "package", LUA_TTABLE ) ) {
		// Get the path of whatever we're talking about
		char ppath[ PATH_MAX / 2 ] = { 0 }, cpath[ PATH_MAX / 2 ] = {0};
		const char *lpath = lua_getv( ld.state, "path", 1 );
		snprintf( ppath, sizeof( ppath ) - 1, extfmt, lpath, ld.root, ld.root );
		lua_pop( ld.state, lua_gettop( ld.state ) - 1 );

		const char *lcpath = lua_getv( ld.state, "cpath", 1 );
		snprintf( cpath, sizeof( cpath ) - 1, libcfmt, lcpath, ld.root );
		lua_pop( ld.state, lua_gettop( ld.state ) - 1 );

		// Re-add to the table
		#if 1
		lua_setstrstr( ld.state, "path", ppath, 1 );
		lua_setstrstr( ld.state, "cpath", cpath, 1 );
		#else
		lua_pushstring( ld.state, "path" );
		lua_pushstring( ld.state, ppath );
		lua_settable( ld.state, 1 );
		lua_pushstring( ld.state, "cpath" );
		lua_pushstring( ld.state, cpath );
		lua_settable( ld.state, 1 );
		#endif
		lua_setglobal( ld.state, "package" );
	}

	//Then start loading our configuration
#if 1
	if ( !load_lua_config( &ld ) ) {
		free_ld( &ld );
		return http_error( conn->res, 500, "%s\n", ld.err );
	}
#else
	// This just got a hell of a lot longer...
#endif

	//Need to delegate to static handler when request points to one of the static paths
	if ( path_is_static( &ld ) ) {

		// TODO: All of this will be moving to the top when time permits
		char err[ 2048 ] = {0};
		char fpath[ 2048 ] = {0};
		char *cache_header = NULL;
		int status = 0;
		struct stat sb = {0};
		const char *xuri = conn->req->path;
		int da = 0, *disallowed = &da;
		const confkey_t skeys[] = {
			{ "cache",    ZTABLE_TBL, 0, get_cache_header },
			/* TODO: Support this: ZTABLE_TBL || ZTABLE_TXT */
			{ "disallow", ZTABLE_TBL, 0, get_disallowed_paths },
			#if 0
			{ "redirect", ZTABLE_TBL, 0, get_redirect },
			#endif
			{ NULL },
		};

	#if 1
		FPRINTF( "*** config dir is '%s'***\n", conn->config->dir );
		FPRINTF( "*** path should be '%s'***\n", conn->req->path );

	#endif

		// Concat
		if ( !concat( fpath, sizeof( fpath ) - 1, "%s/%s", host->dir, xuri + 1 ) ) {
			const char fmt[] = "Failed to generate full path to requested page";
			return http_error( conn->res, 500, "%s", fmt );
		}

		// Check for the existence of file on server
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

		// Finally, check that the current user has read access to the file
		if ( access( fpath, R_OK ) == -1 ) {
			return http_error( conn->res, 401, "%s", "Unauthorized" );
		}

		#if 0
		// TODO: This check should NEVER be needed here, b/c only explicitly
		// defined static paths are allowed in the first place
		for ( const char **dname = def_disallowed_paths; dname && *dname; dname++ ) {
			if ( memstrat( fname, *dname, strlen(fname) ) == 0 ) {
				FPRINTF( "--- Caught disallowed path '%s' ~= %s ---\n", *dname, fname );
				// TODO: Log the attempt
				return http_error( conn->res, 401, "Unauthorized" );
			}
		}
		#endif

		#if 1
		// Search for disallow
		if ( extr_key( ld.zconfig, "disallow", keys, xuri, &disallowed, err, sizeof( err ) ) && *disallowed ) {
			FPRINTF( "--- disallow = %s ***\n", err );
			return http_error( conn->res, 401, "%s", "Unauthorized" );
		}

		#if 0
		// Search for redirect
		if ( extr_key( conf, "redirect", keys, xuri, &redir_header, err, sizeof( err ) ) && redir_header ) {
			// May have to make the entire message
			FPRINTF( "--- redirect = %s ***\n", redir_header );
			//return http_error( conn->res, 401, "%s", "Allocation failure." );
		}
		#endif

		// Search for cache
		if ( extr_key( ld.zconfig, "cache", keys, xuri, &cache_header, err, sizeof( err ) ) && cache_header ) {
			//http_set_header( conn->res, "Cache-control", cache_header );
			http_copy_header( conn->res, "Cache-control", cache_header );
			free( cache_header );
		}
		#endif

		// Send it on
		if ( !send_static( conn->res, fpath, err, sizeof( err ) ) ) {
			return http_error( conn->res, 500, "%s", err );
		}

		// Free any resources
		free_ld( &ld );
		return 1;
	}

	//req->path needs to be modified to return just the path without the ?
	if ( !getpath( conn->req->path, (char *)ld.apath, LD_LEN ) ) {
		free_ld( &ld );
		return http_error( conn->res, 500, "%s", "Failed to extract path info into Lua userspace - Path too long, try increasing LD_LEN to fix this." );
	}

	//TODO: this needs some work.  Mostly just precedence.
	if ( !find_matching_route( &ld ) ) {
		free_ld( &ld );
		return http_error( conn->res, 404, "Couldn't find path at %s\n", ld.apath );
	}

	//Loop through the structure and add read-only structures to Lua,
	//you could also add the libraries, but that is a different method
	for ( struct lua_readonly_t *t = lua_readonly; t->name; t++ ) {
		if ( !t->exec( &ld ) ) {
			free_ld( &ld );
			return http_error( conn->req, ld.status, ld.err );
		}
		lua_setglobal( ld.state, t->name );
	}

#if 0
	//NOTE: 'Set package path' from above was done here previously, check first for issues
#endif

	//Execute each model
	for ( struct imvc_t **m = ld.pp.imvc_tlist; m && *m; m++ ) {
		//Define
		char err[2048] = {0};
		char msymname[1024] = {0};
		char mpath[ 2192 ] = {0};

		//Check for a file
		if ( *(*m)->file == 'a' ) {
			//TODO: Consider using single matches
			//No special characters found, execute the model.
			if ( !strchr( (*m)->base, '@' ) && !strchr( (*m)->base, '#' ) && !strchr( (*m)->base, '^' ) ) {
				snprintf( mpath, sizeof( mpath ), "%s/%s", ld.root, (*m)->file );
			}
			else if ( !peval( mpath, sizeof( mpath ), &ld, (*m) ) ) {
				return http_error( conn->res, 500, "An error occurred processing route: %s", ld.rroute );
			}

			//Actually load and execute said file...
			FPRINTF( "Executing model %s\n", mpath );
			if ( !lua_exec_file( ld.state, mpath, ld.err, sizeof( ld.err ) ) ) {
				free_ld( &ld );
				return http_error( conn->res, 500, "Error occurred: %s", ld.err );
			}

			//Get name of model file in question
			memcpy( msymname, &(*m)->file[4], strlen( (*m)->file ) - 8 );

			//Get a count of the values which came from the model
			tcount += ccount = lua_gettop( ld.state );

			//Check the stack here and make sure that a TABLE was returned...
			FPRINTF( "Checking the stack here after execution of '%s'...\n", mpath );
			FPRINTF( "(%d values on stack)\n", ccount );

			// After exec of anything w/ nil, we still fail...
			// If any values are NOT a table, fail
			for ( int i = 1; i <= ccount; i++ ) {
				if ( lua_isnone( ld.state, i ) )
					FPRINTF( "Only exec, no values.  This is fine.\n" );
				else if ( lua_istable( ld.state, i ) )
					FPRINTF( "Got table.  This is fine.\n" );
				else {
					// For what we're doing, it makes little sense to return anything else...
					const char fmt[] =
						"Executing file %s results in a primitive value.  "
						"(Please try refactoring the code in this file to return values in a key-value table.)"
					;
					free_ld( &ld );
					snprintf( ld.err, sizeof( ld.err ), fmt, mpath );
					return http_error( conn->res, 500, "Error occurred: %s", ld.err );
				}
			}

			//Merge previous models
			if ( tcount > 1 ) {
				lua_getglobal( ld.state, modelkey );

				// No model? Get rid of whatever was added and keep going
				if ( lua_isnil( ld.state, -1 ) ) {
					lua_pop( ld.state, 1 );
				}

				// Previous model exists?, go ahead and merge
				else {
					//When/if a merge fails, error out and don't try again
					if ( !lua_rmerge( ld.state, 1, 2, ld.err, sizeof( ld.err ) ) ) {
						free_ld( &ld );
						snprintf( ld.err, sizeof( ld.err ), "Executing file %s results", mpath );
						return http_error( conn->res, 500, "Error occurred: %s", ld.err );
					}
					lua_remove( ld.state, 2 ); // lua_pop( L, 1 ) should also work
				}
				lua_setglobal( ld.state, modelkey );
			}
			else if ( ccount ) {
				lua_setglobal( ld.state, modelkey );
			}
			model = 1;
		}

		// Stop if the user specifies a 'response' table that's not empty...
		if ( lua_retglobal( ld.state, CKEY_RESPONSE, LUA_TTABLE ) ) {
			FPRINTF( "Evaluating response table.\n" );
			int eres = return_as_response( &ld );

			if ( !eres ) {
				free_ld( &ld );
				FPRINTF( "Error when evaluating response table\n" );
				return http_error( conn->res, 500, ld.err );
			}
			else if ( eres == 1 ) {
				lua_pop( ld.state, 1 );
				free_ld( &ld );
				FPRINTF( "Content return completed\n" );
				return 1;
			}

			lua_pop( ld.state, 1 );
			FPRINTF( "Finished evaluating delayed response table...\n" );
		}
	}

	// Can we simply check if config exists in _G?
	if ( has_views( ld.pp.imvc_tlist ) && lua_retglobal( ld.state, configkey, LUA_TTABLE ) ) {
		FPRINTF( "Adding config table to model...\n" );
		
		// Make a config key and do some stuff...
		lua_newtable( ld.state );
		lua_pushstring( ld.state, configkey );
		lua_pushnil( ld.state );
		lua_copy( ld.state, 1, 4 );
		lua_remove( ld.state, 1 );
		lua_settable( ld.state, 1 );

		// Reference the model from Lua
		lua_getglobal( ld.state, modelkey );

		// There was no model executed here
		if ( lua_isnil( ld.state, -1 ) ) {
			lua_pop( ld.state, 1 );
		}
		// This should be a table, but I think it will be...
		//else if ( !lua_rmerge( ld.state, 0, ld.err, sizeof( ld.err ) ) ) {
		else {
			if ( !lua_rmerge( ld.state, 1, 2, ld.err, sizeof( ld.err ) ) ) {
				free_ld( &ld );
				return http_error( conn->res, 500, "Error when merging config and model: %s", ld.err );
			}
			lua_remove( ld.state, 2 ); // lua_pop( L, 1 ) should also work
		}

		//Add it back to the model after successful merge
		lua_setglobal( ld.state, modelkey );
		FPRINTF( "Adding config done...\n" );
	}

	//TODO: Optionally can add other modules here (like date, etc)
	//...

	//Could be either a table or string... so account for this
	if ( lua_retglobal( ld.state, modelkey, LUA_TTABLE ) ) {
		FPRINTF( "Converting model to ztable...\n" );
		const char **c = ctype_tags;
		char tkey[ 1024 ] = { 0 }, *key = lt_retkv( ld.zroute, 0 )->key.v.vchar;
		int count = lua_count( ld.state, 1 ), ksize = sizeof( tkey );

		//Initialize a table
		if ( !( ld.zmodel = lt_make( ( count = ( count < 1 ) ? 16 : count ) * 2 ) ) ) {
			free_ld( &ld );
			return http_error( conn->res, 500, "Couldn't allocate table." );
		}
		
		//Convert the model
		if ( !lua_to_ztable( ld.state, 1, ld.zmodel ) ) {
			free_ld( &ld );
			return http_error( conn->res, 500, "Error in model conversion." );
		}

		//TODO: Check for an inherited content-type then a default content-type
		for ( int index = -1; *c; c++ ) {
			memset( tkey, 0, ksize ), snprintf( tkey, ksize - 1, "%s.%s", key, *c );
			if ( ( index = lt_geti( ld.zroute, tkey ) ) > -1 ) {
				//Get Content-Type
				char *ctype = lt_text_at( ld.zroute, index );
				for ( ctype_t *cc = ctypes_serializable; cc->ctypename != NULL; cc++ ) {
					if ( !strcasecmp( ctype, cc->ctypename ) ) {
						//Throw your own response in JSON?
						if ( !return_as_serializable( &ld, cc ) ) {
							char err[ LD_ERRBUF_LEN ] = { 0 };
							memcpy( err, ld.err, strlen( ld.err ) );
							free_ld( &ld );
							return http_error( conn->res, 500, "%s", err );
						}
						free_ld( &ld );
						return 1;
					}
				}
			}
		}

		//Finally, check if there is a view specified
		memset( tkey, 0, ksize ), snprintf( tkey, ksize - 1, "%s.%s", key, "view" );
		if ( lt_geti( ld.zroute, tkey ) == -1 ) {
			if ( !return_as_serializable( &ld, &ctypes_serializable[ CTYPE_JSON ] ) ) {
				char err[ LD_ERRBUF_LEN ] = { 0 };
				memcpy( err, ld.err, strlen( ld.err ) );
				free_ld( &ld );
				return http_error( conn->res, 500, "%s", err );
			}
			free_ld( &ld );
			return 1;
		}
		lua_pop( ld.state, 1 );
		lt_lock( ld.zmodel );
		FPRINTF( "Done with model...\n" );
	}

	//TODO: routes with no special keys need not be added
	//Load all views
	for ( struct imvc_t **v = ld.pp.imvc_tlist; v && *v; v++ ) {
		if ( *(*v)->file == 'v' ) {
			int len = 0, renlen = 0;
			char vpath[ 2192 ] = {0};
			unsigned char *src, *render;
			zRender * rz = zrender_init();
			zrender_set_default_dialect( rz );
			zrender_set_fetchdata( rz, ld.zmodel );
			
			if ( *(*v)->base != '@' )
				snprintf( vpath, sizeof( vpath ), "%s/%s", ld.root, (*v)->file );
			else {
				snprintf( vpath, sizeof( vpath ), "%s/%s/%s.%s", ld.root, "views", ld.aroute, (*v)->ext );
			}

			FPRINTF( "Loading view at: %s\n", vpath );
			if ( !( src = read_file( vpath, &len, ld.err, LD_ERRBUF_LEN )	) || !len ) {
				zrender_free( rz ), free( src ), free_ld( &ld );
				return http_error( conn->res, 500, "Error opening view '%s': %s", vpath, ld.err );
			}

			if ( !( render = zrender_render( rz, src, strlen((char *)src), &renlen ) ) ) {
				char errbuf[ 2048 ] = { 0 };
				snprintf( errbuf, sizeof( errbuf ), "%s", rz->errmsg );
				zrender_free( rz ), free( src ), free_ld( &ld );
				return http_error( conn->res, 500, "%s", errbuf );
			}

			zhttp_append_to_uint8t( &content, &clen, render, renlen );
			zrender_free( rz ), free( render ), free( src );
			view = 1;
		}
	}

	//Fail out when neither model or view is specified
	if ( !model && !view ) {
		//free( content );
		free_ld( &ld );
		return http_error( conn->res, 500, "Neither model nor view was specified for '/%s'.", ld.aroute );
	}

	//Set needed info for the response structure
	conn->res->clen = clen;
	http_set_status( conn->res, 200 );
	#if 0
	// TODO: Something wonky lurks here.
	http_set_ctype( res, ctype_def );
	#else
	char *ctype = zhttp_dupstr( ctype_def );
	conn->res->ctype = ctype;
	#endif
	http_set_content( conn->res, content, clen );

	//Return the finished message if we got this far
	if ( !http_finalize_response( conn->res, ld.err, LD_ERRBUF_LEN ) ) {
		snprintf( conn->err, sizeof( conn->err ),
			"Failed to finalize HTTP response: %s", ld.err );
		FPRINTF( "Failed to finalize HTTP response: %s", ld.err );
		free_ld( &ld );
		return 0;
	}

	//Destroy model & Lua
	free( ctype );
	free_ld( &ld );
	free( content );
	return 1;
}
