//config.c
//Parses Lua config files.
//Compile me with: 
//gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
#include "config.h"


//free the table
void free_t( Table *t ) {
	if ( t ) {
		lt_free( t );
		free( t );
	}
}


//Build global configuration
struct config * build_config ( char *file, char *err, int errlen ) {
	FPRINTF( "Configuration parsing started...\n" );

	struct config *config = NULL; 
	Table *t = NULL;
	lua_State *L = NULL;

	//Allocate Lua
	if ( ( L = luaL_newstate() ) == NULL ) {
		snprintf( err, errlen, "Could not initialize Lua environment.\n" );
		return NULL;
	}

	//Allocate config
	if ( ( config = malloc(sizeof(struct config)) ) == NULL ) {
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
	if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 2048 ) ) {
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
	if ( !lua_to_table( L, 1, t ) ) {
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

#if 0
	//This is the global root default
	if ( ( config->root_default = get_char_value( t, "root_default" ) ) ) {
		config->root_default = strdup( config->root_default );
	} 
#endif

	//Destroy lua_State and the tables...
	free_t( t );
	lua_close( L );
	FPRINTF( "Configuration parsing complete.\n" );
	return config;
}


//Destroy our config file.
void free_config( struct config *config ) {
	if ( config->hosts ) {
		free_hosts( config->hosts );	
	}
	free( config );
}


//Add a configuration function...
void * add_config (struct config *config, void **(cexec)(Table *), void(cfree)(void **)) {
	return NULL;
}


