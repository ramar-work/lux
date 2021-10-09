/* ------------------------------------------- * 
 * configs.c
 * =========
 * 
 * Summary 
 * -------
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * - 
 * ------------------------------------------- */
#include "configs.h"

//Free config tables
static void free_t( zTable *t ) {
	if ( t ) {
		lt_free( t );
		free( t );
	}
}

void dump_sconfig ( struct sconfig *c ) {
	//fprintf( stderr, "path: %s\n", c->path );
	fprintf( stderr, "wwwroot: %s\n", c->wwwroot );
	fprintf( stderr, "hosts: %p\n", c->hosts );
	fprintf( stderr, "src: %p\n", c->src );
}

//A hosts handler
static int hosts_iterator ( zKeyval * kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct lconfig ***hosts = f->userdata;
	zTable *st = NULL, *nt = NULL;

	//If current index is a table
	if ( kv->key.type == ZTABLE_TXT && kv->value.type == ZTABLE_TBL && f->depth == 2 ) {
		struct lconfig *w = malloc( sizeof( struct lconfig ) );
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
		//FPRINTF( "NAME: %s, COUNT OF ELEMENTS: %d\n", kv->key.v.vchar, count ); 
		nt = loader_shallow_copy( st, i+1, i+count );
		memset( w, 0, sizeof( struct lconfig ) );
		const struct rule rules[] = {
			{ "alias", "s", .v.s = &w->alias },
			{ "dir", "s", .v.s = &w->dir },
			{ "filter", "s", .v.s = &w->filter },
			{ "root_default", "s", .v.s = &w->root_default },
			{ "ca_bundle", "s", .v.s = &w->ca_bundle },
			{ "cert_file", "s", .v.s = &w->cert_file },
			{ "key_file", "s", .v.s = &w->key_file },
			{ NULL }
		};

		w->name = kv->key.v.vchar;
		loader_run( nt, rules );
		lt_free( nt );
		free( nt );
		add_item( hosts, w, struct lconfig *, &f->len );
	}
	return 1;
}


//Find a host
struct lconfig * find_host ( struct lconfig **hosts, char *hostname ) {
	char host[ 2048 ] = { 0 };
	int pos = memchrat( hostname, ':', strlen( hostname ) );
	memcpy( host, hostname, ( pos > -1 ) ? pos : strlen(hostname) );
	while ( hosts && *hosts ) {
		struct lconfig *req = *hosts;
		if ( req->name && strcmp( req->name, host ) == 0 )  {
			return req;	
		}
		if ( req->alias && strcmp( req->alias, host ) == 0 ) {
			return req;	
		}
		hosts++;
	}
	return NULL;
}


//Build a list of valid hosts
static struct lconfig ** build_hosts ( zTable *t ) {
	struct lconfig **hosts = NULL;
	const struct rule rules[] = {
		{ "hosts", "t", .v.t = (void ***)&hosts, hosts_iterator }, 
		{ NULL }
	};

	if ( !loader_run( t, rules ) ) {
		//...
	}
	return hosts;
}


//Free hosts list
void free_hosts ( struct lconfig ** hlist ) {
	struct lconfig **hosts = hlist;
	while ( hosts && (*hosts) ) {
		FPRINTF( "Freeing host %s\n", (*hosts)->name );
		free( (*hosts)->alias );
		free( (*hosts)->dir );
		free( (*hosts)->filter );
		free( (*hosts)->root_default );
		free( (*hosts)->ca_bundle );
		free( (*hosts)->cert_file );
		free( (*hosts)->key_file );
		free( (*hosts) );
		hosts++;
	}
	free( hlist );
}


//Dump the host list
void dump_hosts ( struct lconfig **hosts ) {
	struct lconfig **r = hosts;
	fprintf( stderr, "Hosts:\n" );
	while ( r && *r ) {
		fprintf( stderr, "\t%p => ", *r );
		fprintf( stderr, "%s =>\n", (*r)->name );
		fprintf( stderr, "\t\tdir = %s\n", (*r)->dir );
		fprintf( stderr, "\t\talias = %s\n", (*r)->alias );
		fprintf( stderr, "\t\tfilter = %s\n", (*r)->filter );
		r++;
	}
}


//...
static int check_server_root( char **item, char *err, int errlen ) {
	struct stat sb;
	char *rp = NULL;

	//If null item
	if ( !*item ) {
		//It can be blank
		return 1;
	}

	//If it starts with a '/', assume its absolute
	if ( **item == '/' ) { 
		if ( stat( *item, &sb ) > -1 )
			return 1;
		else {
			snprintf( err, errlen, "wwwroot '%s' does not exist or is not accessible.", *item ); 
			return 0;
		}
	}

	//If it doesn't, then get the realpath
	if ( ( rp = realpath( *item, NULL ) ) ) {
		free( *item );
		*item = rp;
	}
	else {
		//Make a filename
		snprintf( err, errlen, "wwwroot realpath() failure: %s.", strerror( errno ) );
		return 0;
	}

	return 1;
}


//build_server_config or get_server_config
struct sconfig * build_server_config ( const char *file, char *err, int errlen ) {
	FPRINTF( "Configuration parsing started...\n" );

	struct sconfig *config = NULL; 
	zTable *t = NULL;
	lua_State *L = NULL;

	//Allocate Lua
	if ( ( L = luaL_newstate() ) == NULL ) {
		snprintf( err, errlen, "Could not initialize Lua environment.\n" );
		return NULL;
	}

	//Allocate config
	if ( ( config = malloc( sizeof( struct sconfig ) ) ) == NULL ) {
		snprintf( err, errlen, "Could not initialize memory when parsing config at: %s\n", file );
		return NULL;
	}

	//After this conversion takes place, destroy the environment
	if ( !lua_exec_file( L, file, err, errlen ) ) {
		free( config );
		lua_close( L );
		return NULL;
	}

	//Allocate a table for the configuration
	if ( !(t = malloc(sizeof(zTable))) || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, errlen, "Could not initialize table when parsing config at: %s\n", file );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//Check the stack and make sure that it's a table.
	if ( !lua_istable( L, 1 ) ) {
		snprintf( err, errlen, "Configuration is not a table.\n" );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//Convert configuration into a table
	if ( !lua_to_ztable( L, 1, t ) || !lt_lock( t ) ) {
		snprintf( err, errlen, "Failed to convert Lua config data to table.\n" );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//Build hosts
	if ( ( config->hosts = build_hosts( t ) ) == NULL ) {
		//Build hosts fails with null, I think...
		snprintf( err, errlen, "Failed to bulid hosts table from: %s\n", file );
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	}

	//This is the web root 
	config->wwwroot = dupstr( loader_get_char_value( t, "wwwroot" ) ); 

	//This is the global root default
	//config->root_default = strdup( loader_get_char_value( t, "root_default" ) ); 

	//Die if the webroot is inaccessible
	if ( config->wwwroot && !check_server_root( &config->wwwroot, err, errlen ) ) {
		free_t( t );
		free( config );
		lua_close( L );
		return NULL;
	} 

	//Destroy lua_State and the tables...
	config->src = t;	
	lua_close( L );
	FPRINTF( "Configuration parsing complete.\n" );
	return config;
}

//build_site_config or get_site_config

//Destroy the server oonfig file.
void free_server_config( struct sconfig *config ) {
	if ( !config ) {
		return;
	}

	if ( config->hosts ) {
		free_hosts( config->hosts );	
	}

#if 0
	if ( config->routes ) {
		free_routes( config->routes );
	}
#endif
	free( config->wwwroot );
	//free( config->root_default );
	//FPRINTF( "%p\n", config ); getchar();
	lt_free( config->src );
	free( config->src );
	free( config );
}
