/* ------------------------------------------- * 
 * filter-lua.c 
 * =============
 * 
 * Summary 
 * -------
 * - 
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include "filter-lua.h"

static const char rname[] = "route";

static const char def[] = "default";

static const char confname[] = "config.lua";

static const char mkey[] = "model";

static const char rkey[] = "routes";

static const char ctype_def[] = "text/html";

struct ab {
	int msglen;
	unsigned char *msg;	
};


static int make_c ( zKeyval *kv, int i, void *p ) {
	struct ab *ab = (struct ab *)p;
	if ( kv->key.type == ZTABLE_TXT )
		;
	else if ( kv->key.type == ZTABLE_INT ) {

	}

	if ( kv->value.type == ZTABLE_INT ) 
		;
	else if ( kv->value.type == ZTABLE_FLT ) 
		;
	else if ( kv->value.type == ZTABLE_TXT ) 
		zhttp_append_to_uint8t( &ab->msg, &ab->msglen, (unsigned char *)kv->value.v.vchar, strlen( kv->value.v.vchar )); 
	else if ( kv->value.type == ZTABLE_BLB ) {
		zhttp_append_to_uint8t( &ab->msg, &ab->msglen, kv->value.v.vblob.blob, kv->value.v.vblob.size ); 
	} 
	return 1;	
}

//Create a message out of a zTable
struct ab * ztable_to_msg ( zTable *t ) {
	struct ab *ab = NULL;
	lt_exec_complex( t, 1, t->count, &ab, make_c );
	return ab;
}


//HTTP error
int http_error( struct HTTPBody *res, int status, char *fmt, ... ) { 
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


//TODO: This is an incredibly difficult way to make things 
//read-only...  Is there a better one?
static const char read_only_block[] = " \
	function make_read_only( t ) \
		local tt = {} \
		local mt = { \
			__index = t	 \
		,	__newindex = function (t,k,v) error( 'something went wrong', 1 ) end \
		}	\
		setmetatable( tt, mt ) \
		return tt \
	end \
";


static struct mvcmeta_t { 
	const char *dir; 
	const char *ext;
	const char *reserved; 
} mvcmeta [] = {
	{ "app", "lua", "model,models" }
,	{ "sql", "sql", "query,queries" }
,	{ "views", "tpl", "view,views" } 
,	{ NULL, NULL, "content-type" }
//,	{ NULL, "inherit", NULL }
};



//....
int run_lua_buffer( lua_State *L, const char *buffer ) {
	//Load a buffer	
	luaL_loadbuffer( L, buffer, strlen( buffer ), "make-read-only-function" );
	if ( lua_pcall( L, 0, LUA_MULTRET, 0 ) != LUA_OK ) {
		fprintf( stdout, "lua string exec failed: %s", (char *)lua_tostring( L, -1 ) );
		//This shouldn't fail, but if it does you should stop...
		return 0;
	}
	return 1;
}



//In lieu of a C-only way to make members read only, use this
int make_read_only ( lua_State *L, const char *table ) {
	//Certain tables (and their children) need to be read-only
	int err = 0;
	const char fmt[] = "%s = make_read_only( %s )";
	char execbuf[ 256 ] = { 0 };
	snprintf( execbuf, sizeof( execbuf ), fmt, table, table );

	//Load a buffer	
	luaL_loadbuffer( L, execbuf, strlen( execbuf ), "make-read-only" );
	if ( ( err = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
		fprintf( stdout, "lua string exec failed: %s", (char *)lua_tostring( L, -1 ) );
		//This shouldn't fail, but if it does you should stop...
		return 0;
	}
	return 1;
}



//Should return an error b/c there are some situations where this does not work.
int lua_loadlibs( lua_State *L, struct lua_fset *set ) {
	//Now load everything written elsewhere...
	for ( ; set->namespace; set++ ) {
//fprintf( stderr, "namespace: %s\n", set->namespace );
		lua_newtable( L );
		for ( struct luaL_Reg *f = set->functions; f->name; f++ ) {
			lua_pushstring( L, f->name );
			lua_pushcfunction( L, f->func );	
			lua_settable( L, 1 );
		}
		lua_setglobal( L, set->namespace );
	}

	//And finally, add some functions that we'll need later (if this fails, meh)
	if ( !run_lua_buffer( L, read_only_block ) ) {
		return 0;
	}
	return 1;
}



//checking args is putting me in a weird zone... 
int lua_getarg() {
	return ZLUA_NO_ERROR;
}



//Check if there is a reserved keyword being requested
static int is_reserved( const char *a ) {
	for ( int i = 0; i < sizeof( mvcmeta ) / sizeof( struct mvcmeta_t ); i ++ ) {
		zWalker w = {0};
		for ( ; strwalk( &w, mvcmeta[i].reserved, "," ); ) {
			char buf[64];
			memset( buf, 0, sizeof( buf ) );
			memcpy( buf, w.src, ( w.chr == ',' ) ? w.size - 1 : w.size );
			if ( strcmp( a, buf ) == 0 ) return 1;
		}
	}
	return 0;
}



//Make a route list
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



//Create a list of resources (an alternate version of this will inherit everything) 
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



//Free MVC list
void free_mvc_list ( void ***list ) {
	for ( void **l = *list; l && *l; l++ ) {
		free( *l );
	} 
	free( *list ), *list = NULL;
}



//Free route list
static void free_route_list ( struct iroute_t **list ) {
	for ( struct iroute_t **l = list; *l; l++ ) {
		free( (*l)->route ), free( *l );
	}
	free( list );
}



//Load Lua configuration
int load_lua_config( struct luadata_t *l ) {
	char *db, *fqdn, cpath[ 2048 ] = { 0 };
	DIR *dir = NULL;
	struct dirent *d = NULL;

	//If this fails, do something
	if ( !lt_init( l->zconfig, NULL, 1024 ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Couldn't initalize configuration table." );
		return 0;
	}

	//Create a better path
	snprintf( cpath, sizeof(cpath) - 1, "%s/%s", l->root, confname );

	//Open the configuration file
	if ( !lua_exec_file( l->state, cpath, l->err, LD_ERRBUF_LEN ) ) {
		return 0;
	}

	//If it's anything but a Lua table, we're in trouble
	if ( !lua_istable( l->state, 1 ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Configuration is not a Lua table." );
		return 0;
	}

	//Convert the Lua values to real values for extraction.
	if ( !lua_to_ztable( l->state, 1, l->zconfig ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "Failed to convert Lua to zTable" );
		return 0;
	}

	//Get the index (IF we need it)
	//int rindex = lt_geti( l->zconfig, rkey );
	lt_lock( l->zconfig );

	//Need to make a different table, that stays in Lua
	if ( !( l->zroutes = lt_copy_by_key( l->zconfig, rkey ) ) ) {
		snprintf( l->err, LD_ERRBUF_LEN, "'routes' key not present in config!" );
		return 0;
	}

	lt_lock( l->zroutes );
	//Pop the remaining Lua stuff
	lua_pop( l->state, 1 );

	//Create a directory string
	snprintf( cpath, sizeof(cpath) - 1, "%s/%s", l->root, rkey );
	fprintf( stderr, "%s\n", cpath );

	//Do a list of the directory
	if ( ( dir = opendir( cpath ) ) ) {
		//Put the ztable on the stack
		if ( !ztable_to_lua( l->state, l->zroutes ) ) {
			snprintf( l->err, LD_ERRBUF_LEN, "Failed to move routing table into Lua userspace!" );
			return 0;
		}
	
		//The solution is decouple the string and the table
		//and place the table back on the stack, (deleting the string)
		//then merge SHOULD work...
		//You may also be able to add a string and a table, after merging, then
		//you should have a routes -> { aggregate of all route files + what is in the config } 
		lua_pushnil( l->state );
		lua_next( l->state, 1 );
		lua_remove( l->state, 1 );
		lua_remove( l->state, 1 );

		//Load each route file and combine it with the route table
		for ( int dlen; ( d = readdir( dir ) ) ; ) {
			if ( ( dlen = strlen( d->d_name )) <= 2 && ( d->d_name[1] == '.' || d->d_name[0] == '.' ) ) {
				continue;
			}

			//Only deal with regular Lua files (eventually can support symbolic links)
			if ( strlen( d->d_name ) > 4 && strstr( d->d_name, ".lua" ) && d->d_type == DT_REG ) { 
				snprintf( cpath, sizeof(cpath) - 1, "%s/%s/%s", l->root, "routes", d->d_name );
				fprintf( stderr, "dirname: %s\n", cpath );

				//Open each file in the directory?
				if ( !lua_exec_file( l->state, cpath, l->err, LD_ERRBUF_LEN ) ) {
					//snprintf( stderr, "Lua error: %s\n", l->err );
					return 1;
				}

				//If we made it this far, then merge it with the routes table...
				lua_merge( l->state );
				//lua_settable( l->state, 1 );
			}
		}

		//Close the directory
		closedir( dir );

		//Finally, combine all of these into one
		lua_newtable( l->state );
		lua_insert( l->state, 1 ); 
		lua_pushstring( l->state, rkey );
		lua_insert( l->state, 2 ); 
		lua_settable( l->state, 1 ); 

		//destroy the original zroutes
		lt_free( l->zroutes ), free( l->zroutes ); 

		//and add them back
#if 0
		l->zroutes = malloc( sizeof( zTable ) );
		memset( l->zroutes, 0, sizeof( zTable ) );
		lt_init( l->zroutes, 1024 );
#else
		l->zroutes = lt_make( 1024 );
#endif
		if ( !lua_to_ztable( l->state, 1, l->zroutes ) ) {
			snprintf( l->err, LD_ERRBUF_LEN, "Something somewhere failed because of something." );
			return 0;
		}

		lt_lock( l->zroutes );
	}

	//Set other keys here
	if ( ( db = lt_text( l->zconfig, "db" ) ) ) {
		memcpy( (void *)l->db, db, strlen( db ) ); 
	}

	if ( ( fqdn = lt_text( l->zconfig, "fqdn" ) ) ) {
		memcpy( (void *)l->fqdn, fqdn, strlen( fqdn ) ); 
	}

	return 1;
}



//Checking for static paths is important, also need to check for disallowed paths
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



//Send a static file
static const int send_static ( struct HTTPBody *res, const char *dir, const char *uri ) {
	//Read_file and return that...
	struct stat sb;
	int dlen = 0;
	char err[ 2048 ] = { 0 }, spath[ 2048 ] = { 0 };
	unsigned char *data;
	const struct mime_t *mime;
	memset( spath, 0, sizeof( spath ) );
	snprintf( spath, sizeof( spath ) - 1, "%s/%s", dir, ++uri );

	//check if the path is there at all (read-file can't do this)
	if ( stat( spath, &sb ) == -1 ) {
		return http_error( res, 404, "static read failed: %s", err );
	}
	//read_file and send back
	if ( !( data = read_file( spath, &dlen, err, sizeof( err ) ) ) ) {
		return http_error( res, 500, "static read failed: %s", err );
	}

	//Gotta get the mimetype too	
	if ( !( mime = zmime_get_by_filename( spath ) ) ) {
		mime = zmime_get_default();
	}

	//Send the message out
	res->clen = dlen;
	http_set_status( res, 200 ); 
	http_set_ctype( res, mime->mimetype );
	http_set_content( res, data, dlen );
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_error( res, 500, err );
	}

	free( data );
	return 1;	
}


//...
void dump_records( struct HTTPRecord **r ) {
	int b = 0;
	for ( struct HTTPRecord **a = r; a && *a; a++ ) {
		fprintf( stderr, "%p: %s -> ", *a, (*a)->field ); 
		b = write( 2, (*a)->value, (*a)->size );
		b = write( 2, "\n", 1 );
	}
}


static char * getpath( char *rpath, char *apath, int plen ) {
	if ( plen < strlen( rpath ) ) return NULL;
	for ( char *p = apath, *path = rpath; *path && *path != '?'; ) *(p++) = *(path++);
	return apath;
}


#if 0
static int setroutes ( struct luadata_t *l ) {
	struct route_t p = { 0 };
	struct mvc_t pp = {0};
	zTable *croute = NULL;

	//Get the routes from the config file.
	if ( !( l->zroutes = lt_copy_by_key( l->zconfig, rkey ) ) ) {
		l->status = 500;
		snprintf( l->err, LD_ERRBUF_LEN, "%s", "Failed to copy routes from config." );
		return 0;
	}

	//Turn the routes into a list of strings, and search for a match
	//p.src = zroutes;
	lt_exec_complex( p.src = l->zroutes, 1, l->zroutes->count, &p, make_route_list );
	
	//Loop through each route and find the thing 
	l->pp.depth = 1;
	for ( struct iroute_t **lroutes = p.iroute_tlist; *lroutes; lroutes++ ) {
		if ( ( l->rroute = route_resolve( l->apath, (*lroutes)->route ) ) ) {
			croute = lt_copy_by_index( l->zroutes, (*lroutes)->index );
			lt_exec_complex( croute, 1, croute->count, &pp, make_mvc_list );
			break;
		}
	}

	//Die when unavailable...
	if ( !croute ) {
		free_route_list( p.iroute_tlist );	
		l->status = 404;
		snprintf( l->err, LD_ERRBUF_LEN, "Couldn't find path at %s\n", l->apath );
		return 0;
	}

#if 0
	//If a route was found, break it up
	if ( !( l->zroute = getproutes( rpath, rroute ) ) ) {
		free_route_list( p.iroute_tlist );	
		l->status = 404;
		snprintf( l->err, LD_ERRBUF_LEN, "Couldn't initialize route map.\n" );
		return 0;
	}

	//Mark the active route from here and leave
	l->aroute = lt_text( l->zroute, "route.active" );	
#endif
	free_route_list( p.iroute_tlist );	
	return 1;
}
#endif



//Initialize routes in Lua
int init_lua_routes ( struct luadata_t *l ) {
	zWalker w = {0}, w2 = {0};
	const char *active = NULL, *path = l->apath + 1, *resolved = l->rroute + 1;
	char **routes = { NULL };
	int index = 0, rlen = 0, pos = 1;
	
	//Add a table.
	lua_newtable( l->state );

	//Handle root requests
	if ( !*path ) {
		lua_pushinteger( l->state, 1 ), lua_pushstring( l->state, def ); 
		lua_settable( l->state, pos );
		lua_pushstring( l->state, "active" ), lua_pushstring( l->state, def ); 
		lua_settable( l->state, pos );
		memcpy( (void *)l->aroute, def, strlen (def) );
		return 1;
	} 
	
	//Loop twice to set up the map
	for ( char stub[1024], id[1024]; strwalk( &w, path, "/" ); ) {
		//write the length of the block between '/'
		memset( stub, 0, sizeof(stub) );
		memcpy( stub, w.src, ( w.chr == '/' ) ? w.size - 1 : w.size );
		for ( ; strwalk( &w2, resolved, "/" ); ) {
			int size = ( w2.chr == '/' ) ? w2.size - 1 : w2.size;
			//if there is an equal, most likely it's an id
			if ( *w2.src != ':' )
				lua_pushinteger( l->state, ++index );	
			else {
				//Find the key/id name
				for ( char *p = (char *)w2.src, *b = id; *p && ( *p	!= '=' || *p != '/' ); ) {
					*(b++) = *(p++);
				}
				//Check that id is not active, because that's a built-in
				if ( strcmp( id, "active" ) == 0 ) {
					return 0;
				}
				
				//Add a numeric key first, then a text key
				lua_pushinteger( l->state, ++index );
				lua_pushstring( l->state, stub );
				lua_settable( l->state, pos );
				lua_pushstring( l->state, id );
			}
			break;
		}
		//copy the value (stub) to value in table
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



//Initialize HTTP in Lua
int init_lua_http ( struct luadata_t *l ) {
	//Loop through all things
	const char *str[] = { "headers", "url", "body" };
	struct HTTPRecord **ii[] = { l->req->headers, l->req->url, l->req->body };

	//Add one table for all structures
	lua_newtable( l->state );

	//Add general request info
	lua_pushstring( l->state, "contenttype" );
	lua_pushstring( l->state, l->req->ctype );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "ctype" );
	lua_pushstring( l->state, l->req->ctype );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "length" );
	lua_pushinteger( l->state, l->req->clen );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "path" );
	lua_pushstring( l->state, l->req->ctype );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "method" );
	lua_pushstring( l->state, l->req->method );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "status" );
	lua_pushinteger( l->state, l->req->status );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "protocol" );
	lua_pushstring( l->state, l->req->protocol );
	lua_settable( l->state, 1 );
	lua_pushstring( l->state, "host" );
	lua_pushstring( l->state, l->req->host );
	lua_settable( l->state, 1 );
	

	//Add simple keys for headers and URL
	for ( int pos=3, i = 0; i < 3; i++ ) {
		struct HTTPRecord **r = ii[i];
		if ( r && *r ) {
			lua_pushstring( l->state, str[i] ), lua_newtable( l->state );
			for ( ; r && *r; r++ ) {
				lua_pushstring( l->state, (*r)->field );
			#if 0
				lua_pushlstring( l->state, ( char * )(*r)->value, (*r)->size );
			#else
				lua_newtable( l->state );
				lua_pushstring( l->state, "value" ); 
				lua_pushlstring( l->state, ( char * )(*r)->value, (*r)->size );
				lua_settable( l->state, pos + 2 ); 	
				lua_pushstring( l->state, "size" ); 
				lua_pushinteger( l->state, (*r)->size );
				lua_settable( l->state, pos + 2 ); 	
			#endif
				lua_settable( l->state, pos );
			}
			lua_settable( l->state, 1 );
		}
	}

	//Set global name
	return 1;
}


int init_lua_shadowpath ( struct luadata_t *l ) {
	lua_pushstring( l->state, l->root );
	return 1;	
}


int init_lua_config ( struct luadata_t *l ) {
	return ztable_to_lua( l->state, l->zconfig ); 
}



//Both of these functions return zTables, and this is heavy on memory and does a useless context switch
//Doing it all in Lua makes more sense...
struct lua_readonly_t {
	const char *name;
	int (*exec)( struct luadata_t * );
	//int (*exec)( lua_State *, struct HTTPBody *, const char *, const char * );	
} lua_readonly[] = {
	{ "request", init_lua_http }
, { "route", init_lua_routes }
, { "config", init_lua_config }
, { "shadow", init_lua_shadowpath }
, { NULL }
};

#if 0
//consider this instead, but it will be extern...
struct lua_lib_t {
	...
} lua_libs[] = {
	{ "some_lib", libs }
, { NULL }
};
#endif


int free_ld ( struct luadata_t *l ) {
	lua_close( l->state );
	lt_free( l->zconfig );
	if ( l->zroutes ) {
		lt_free( l->zroutes );
		free( l->zroutes );
	}
	lt_free( l->zmodel );
	free_mvc_list( (void ***)&(l->pp.imvc_tlist) );
	return 1;
}


//...
int return_as_response ( struct luadata_t *l ) {

	zTable *rt = NULL;
	int count = 0, status = 200, clen = 0;
	char *ctype = "text/html";
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
		//This can fail...
		snprintf( l->err, LD_ERRBUF_LEN, lt_strerror( rt ) );
		return 0;
	}

	//Get the status
	int status_i = 0;
	if ( ( status_i = lt_geti( rt, "status" ) ) > -1 ) {
		status = lt_int_at( rt, status_i );
	}
	
	//Get the content-type (if there is one)
	int ctype_i = 0;
	if ( ( ctype_i = lt_geti( rt, "ctype" ) ) > -1 )
		ctype = zhttp_dupstr( lt_text_at( rt, ctype_i ) ); 
	else {
		ctype = zhttp_dupstr( ctype );
	}

	//Get the content-length (if there is one)
	int clen_i = 0;
	if ( ( clen_i = lt_geti( rt, "clen" ) ) > -1 ) {
		clen = lt_int_at( rt, clen_i ); 
	}

	//Get the content
	int content_i = 0;
	if ( ( content_i = lt_geti( rt, "content" ) ) > -1 ) {
		if ( clen_i > -1 )
			content = lt_blob_at( rt, content_i ).blob;
		else {
			content = (unsigned char *)lt_text_at( rt, content_i );
			clen = strlen( (char *)content );
		} 
	}

	//Set structures
	l->res->clen = clen;
fprintf( stderr, "CLEN IS %d\n", clen );
	http_set_status( l->res, status ); 
	http_set_ctype( l->res, ctype );
	http_set_content( l->res, content, clen ); 

	//Return finalized content
	//lt_free( rt ), free( rt );
#if 0
	return http_finalize_response( l->res, l->err, LD_ERRBUF_LEN ) ? 1 : 0;
#else
	int s = http_finalize_response( l->res, l->err, LD_ERRBUF_LEN ) ? 1 : 0;

print_httpbody( l->res );
	return s;
#endif
}


//The entry point for a Lua application
const int filter_lua( int fd, zhttp_t *req, zhttp_t *res, struct cdata *conn ) {

	//Define variables and error positions...
	zTable zc = {0}, zm = {0};
	struct luadata_t ld = {0};
	int clen = 0;
	unsigned char *content = NULL;

	//Initialize the data structure
	memset( res, 0, sizeof( zhttp_t ) );
	ld.req = req, ld.res = res;
	ld.zconfig = &zc, ld.zmodel = &zm, ld.zroutes = NULL;
	memcpy( (void *)ld.root, conn->hconfig->dir, strlen( conn->hconfig->dir ) );

	//Then initialize the Lua state
	if ( !( ld.state = luaL_newstate() ) ) {
		return http_error( res, 500, "%s", "Failed to initialize Lua environment." );
	}

	//Load the standard libraries first
	luaL_openlibs( ld.state );

	//Then start loading our configuration
	if ( !load_lua_config( &ld ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s\n", ld.err );
	}

//excess path handling has to be done here...
//return http_error( res, 200, "nothing at all" );

	//Need to delegate to static handler when request points to one of the static paths
	if ( path_is_static( &ld ) ) {
		free_ld( &ld );
		return send_static( res, ld.root, req->path );
	}

	//req->path needs to be modified to return just the path without the ?
	if ( !getpath( req->path, (char *)ld.apath, LD_LEN ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s", "Failed to extract path." );
	}

	//Get the routes from the config file.
	if ( !( ld.zroutes = lt_copy_by_key( ld.zconfig, rkey ) ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s", "Failed to copy routes from config." );
	}

	//Turn the routes into a list of strings, and search for a match
	lt_lock( ld.zroutes );
	//lt_kfdump( ld.zroutes, 1 );
	struct route_t p = { .src = ld.zroutes };
	lt_exec_complex( ld.zroutes, 1, ld.zroutes->count, &p, make_route_list );

	//Loop through the routes
	ld.pp.depth = 1;
	int notfound = 1;
	for ( struct iroute_t **lroutes = p.iroute_tlist; *lroutes; lroutes++ ) {
		if ( route_resolve( ld.apath, (*lroutes)->route ) ) {
			memcpy( (void *)ld.rroute, (*lroutes)->route, strlen( (*lroutes)->route ) );
			zTable * croute = lt_copy_by_index( ld.zroutes, (*lroutes)->index );
			lt_exec_complex( croute, 1, croute->count, &ld.pp, make_mvc_list );
			lt_free( croute );
			free( croute );
			notfound = 0;
			break;
		}
	}

	//Die when unavailable...
	if ( notfound ) {
		free_route_list( p.iroute_tlist );	
		free_ld( &ld );
		return http_error( res, 404, "Couldn't find path at %s\n", ld.apath );
	}

#if 0
	//If a route was found, break it up
	if ( !( zroute = getproutes( ld.apath, rroute ) ) ) {
		return http_error( res, 500, "Couldn't initialize route map.\n" );
	}
#endif

	//Destroy anything having to do with routes 
	free_route_list( p.iroute_tlist );	

	//Loop through the structure and add read-only structures to Lua, 
	//you could also add the libraries, but that is a different method
	for ( struct lua_readonly_t *t = lua_readonly; t->name; t++ ) {
		if ( !t->exec( &ld ) ) {
			free_ld( &ld );
			return http_error( req, ld.status, ld.err );
		}
		lua_setglobal( ld.state, t->name );
	}

	//Load standard libraries
	if ( !lua_loadlibs( ld.state, functions ) ) {
		free_ld( &ld );
		return http_error( res, 500, "Failed to initialize Lua standard libs." ); 
	}


	//Execute each model
	int ccount = 0, tcount = 0, model = 0;
	for ( struct imvc_t **m = ld.pp.imvc_tlist; m && *m; m++ ) {
		//Define
		char err[2048] = {0}, msymname[1024] = {0}, mpath[2048] = {0};

		//Check for a file
		if ( *(*m)->file == 'a' ) {
			//Open the file that will execute the model
			if ( *(*m)->base != '@' )
				snprintf( mpath, sizeof( mpath ), "%s/%s", ld.root, (*m)->file );
			else {
				snprintf( mpath, sizeof( mpath ), "%s/%s/%s.%s", ld.root, "app", ld.aroute, (*m)->ext );
			}

			//...
			FPRINTF( "Executing model %s\n", mpath );
			if ( !lua_exec_file( ld.state, mpath, ld.err, sizeof( ld.err ) ) ) {
				free_ld( &ld );
				return http_error( res, 500, "%s", ld.err );
			}

			//Get name of model file in question 
			memcpy( msymname, &(*m)->file[4], strlen( (*m)->file ) - 8 );

			//Get a count of the values which came from the model
			tcount += ccount = lua_gettop( ld.state );

			//Merge previous models
			if ( tcount > 1 ) {
				lua_getglobal( ld.state, mkey );
				( lua_isnil( ld.state, -1 ) ) ? lua_pop( ld.state, 1 ) : 0;
				lua_merge( ld.state );	
				lua_setglobal( ld.state, mkey );
			} 
			else if ( ccount ) {
				lua_setglobal( ld.state, mkey );
			}
			model = 1;
		}
	}

	//In the case of no model, initialize one anyway
	if ( !lt_init( ld.zmodel, NULL, 8193 ) ) {
		free_ld( &ld );
		return http_error( res, 500, "Could not allocate table for model." );
	}

	//Push whatever model is there
	lua_getglobal( ld.state, mkey ); 
	if ( lua_isnil( ld.state, -1 ) )
		lua_pop( ld.state, 1 );
	else { 
		if ( !lua_to_ztable( ld.state, 1, ld.zmodel ) ) {
			free_ld( &ld );
			return http_error( res, 500, "Error in model conversion." );
		}
	}

	//Push whatever model is there
	lua_getglobal( ld.state, "response" ); 
	if ( lua_isnil( ld.state, -1 ) )
		lua_pop( ld.state, 1 );
	else {
		FPRINTF( "Attempting alternate content return.\n" );
		if ( !return_as_response( &ld ) ) {
			free_ld( &ld );
print_httpbody( ld.res );
			return http_error( res, 500, ld.err );
		}
print_httpbody( ld.res );
		free_ld( &ld );
		FPRINTF( "We got to a successful point.\n" );
		return 1;
	}

	//Load all views
	lt_lock( ld.zmodel );

#if 1
	//lt_kfdump( ld.zmodel, 2 );
#else
	struct timespec tt = {0};
	clock_gettime( CLOCK_REALTIME, &tt ); 
	srand( tt.tv_nsec );
	char fb[ 1024 ] = {0};
	snprintf( fb, sizeof(fb), "/tmp/modeltest-%d", rand() );
	int mfd = open( fb, O_CREAT | O_RDWR, 0755 );
	if ( mfd == -1 ) {
		fprintf( stderr, "%s\n", strerror( errno ) );
		exit( 1 );
	}
	lt_kfdump( ld.zmodel, mfd );
	close( mfd );
	free_ld( &ld );
	exit(0);
#endif

	//TODO: routes with no special keys need not be added

	int view = 0;
	for ( struct imvc_t **v = ld.pp.imvc_tlist; v && *v; v++ ) {
		if ( *(*v)->file == 'v' ) {
			int len = 0, renlen = 0;
			char vpath[ 2048 ] = {0};
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
				return http_error( res, 500, "Error opening view '%s': %s", vpath, ld.err );
			}

			if ( !( render = zrender_render( rz, src, strlen((char *)src), &renlen ) ) ) {
				char errbuf[ 2048 ] = { 0 };
				snprintf( errbuf, sizeof( errbuf ), "%s", rz->errmsg );
				zrender_free( rz ), free( src ), free_ld( &ld );
				return http_error( res, 500, "%s", errbuf );
			}

			zhttp_append_to_uint8t( &content, &clen, render, renlen ); 
			zrender_free( rz ), free( render ), free( src );
			view = 1;
		}
	}

	//If key names are mispelled, this can cause the engine to completely skip rendering or interpreting a moidel
	//Fail out in this case
	if ( !model && !view ) {
		free( content );
		free_ld( &ld );
		return http_error( res, 500, "Neither model nor view was specified for '/%s'.", ld.aroute );
	}
	else if ( !view ) {
		free( content );
		free_ld( &ld );
		return http_error( res, 500, "No view was specified for resource '/%s'.", ld.aroute );
	}

	//Set needed info for the response structure
	res->clen = clen;
	http_set_status( res, 200 ); 
	http_set_ctype( res, "text/html" );
	http_set_content( res, content, clen ); 

	//Return the finished message if we got this far
	if ( !http_finalize_response( res, ld.err, LD_ERRBUF_LEN ) ) {
		free_ld( &ld );
		return http_error( res, 500, "%s", ld.err );
	}

	//Destroy model & Lua
	free_ld( &ld ), free( content );
	return 1;
}



#ifdef RUN_MAIN
int main (int argc, char *argv[]) {
	struct HTTPBody req = {0}, res = {0};
	char err[ 2048 ] = { 0 };

	//Populate the request structure.  Normally, one will never populate this from scratch
	req.path = zhttp_dupstr( "/books" );
	req.ctype = zhttp_dupstr( "text/html" );
	req.host = zhttp_dupstr( "example.com" );
	req.method = zhttp_dupstr( "GET" );
	req.protocol = zhttp_dupstr( "HTTP/1.1" );

	//Assemble a message from here...
	if ( !http_finalize_request( &req, err, sizeof( err ) ) ) {
		fprintf( stderr, "%s\n", err );
		return 1; 
	}

	//run the handler
	if ( !lua_handler( &req, &res ) ) {
		fprintf( stderr, "lmain: HTTP funct failed to execute\n" );
		write( 2, res.msg, res.mlen );
		http_free_request( &req );
		http_free_response( &res );
		return 1;
	}

	//Destroy res, req and anything else allocated
	http_free_request( &req );
	http_free_response( &res );

	//After we're done, look at the response
	return 0;
}
#endif
