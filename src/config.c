//config.c
//Parses Lua config files.
//Compile me with: 
//gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
#include "config.h"


//Build global configuration
struct config * build_config ( char *file, char *err, int errlen ) {
	FPRINTF( "Configuration parsing started...\n" );

	const char *funct = "build_config";
	struct config *config = NULL; 
	lua_State *L = luaL_newstate();
	Table *t = NULL;

	if ( ( config = malloc( sizeof ( struct config ) ) ) == NULL ) {
		snprintf( err, errlen, "Could not initialize memory when parsing config at: %s\n", file );
		return NULL;
	}

	//After this conversion takes place, destroy the environment
	if ( !lua_exec_file( L, file, err, sizeof(err) ) ) {
		goto freeres;
		return NULL;
	}

	//Allocate a table for the configuration
	if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 2048 ) ) {
		snprintf( err, errlen, "Could not initialize table when parsing config at: %s\n", file );
		goto freeres;
		return NULL;
	}

	//Convert configuration into a table
	if ( !lua_to_table( L, 1, t ) ) {
		snprintf( err, errlen, "Failed to convert Lua config data to table.\n" );
		goto freeres;
		return NULL;
	}

	//Build hosts
	if ( ( config->hosts = build_hosts( t ) ) == NULL ) {
		//Build hosts fails with null, I think...
		snprintf( err, errlen, "Failed to bulid hosts table from: %s\n", file );
		goto freeres;
		return NULL;
	}

#if 0
	//This is the global root default
	if ( ( config->root_default = get_char_value( t, "root_default" ) ) ) {
		config->root_default = strdup( config->root_default );
	} 
#endif

freeres:
	//Destroy lua_State and the tables...
	lt_free( t );
	free( t );
	lua_close( L );
	FPRINTF( "Configuration parsing complete.\n" );
	return config;
}


//Get integer value from a table
int get_int_value ( Table *t, const char *key, int notFound ) {
	int i = lt_geti( t, key );
	LiteRecord *p = NULL;
	if ( i == -1 ) {
		return notFound;
	}

	if (( p = lt_ret( t, LITE_INT, i ))->vint == 0 ) {
		return notFound;
	}

	return p->vint;
}


//Get string value from a table
char * get_char_value ( Table *t, const char *key ) {
	int i = lt_geti( t, key );
	LiteRecord *p = NULL;
	if ( i == -1 ) {
		return NULL;
	}

	if (( p = lt_ret( t, LITE_TXT, i ))->vchar == NULL ) {
		return NULL;
	}

	return p->vchar;
}



//Destroy our config file.
void free_config( struct config *config ) {
	struct host **hosts = config->hosts;
	while ( hosts && *hosts ) {
		struct host *h = *hosts;
		( h->name ) ? free( h->name ) : 0;
		( h->alias ) ? free( h->alias ) : 0 ;
		( h->dir ) ? free( h->dir ) : 0;
		( h->filter ) ? free( h->filter ) : 0 ;
		( h->root_default ) ? free( h->root_default ) : 0;
		free( h );
		hosts++;
	}
	free( config->hosts );
	free( config );
}

