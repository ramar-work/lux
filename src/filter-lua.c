#include "filter-lua.h"
#include "filter-static.h"

#define FILTER_LUA_DEBUG

#ifndef FILTER_LUA_DEBUG
 #define FILTER_LUA_PRINT(...)
#else
 #define FILTER_LUA_PRINT(...) \
	fprintf( stderr, "[%s:%d]", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ )
#endif

#define lua_pushustrings(STATE,KEY,VAL,VLEN) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, VLEN )


#define lua_pushstrings(STATE,KEY,VAL) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, strlen( VAL ))

#if 0
struct config_key_handler { 
	const char *name;
	int (*fp)( Table *t, void *p );
	void *p;
};

//easiest way is loop through and find the key and return it
//it should handle functions too...
struct config_key_handler {
/*{ name,     variable,   function } */
	{ "static", static_dir, get_char_key },
	{ "routes", static_dir, build_routes },
	{ "static", static_dir },
	{ "static", static_dir },
	{ "static", static_dir },
};
#endif


//All of the keys I need to parse will go here for now...
//I'll also just make a config object...
int parse_config ( Table *t ) {
	return 0;
}


int check_static( Table *t, const char *path ) {
	char *statdir = get_char_value( t, "static" );

	if ( !path || !statdir || strlen(path) < strlen(statdir) ) { 
		return 0;
	}

	if ( memcmp( statdir, ++path, strlen( statdir ) ) != 0 ) {
		return 0;
	} 

	return 1;
}


//filter-lua.c - Run HTTP messages through a Lua handler
//Any processing can be done in the middle.  Since we're in another "thread" anyway
int filter_lua ( struct HTTPBody *req, struct HTTPBody *res, void *ctx ) {

	Table *c = NULL, *t = NULL;
	const char *lconfname = "/config.lua";
	struct config *config = NULL; 
	struct config *lconfig = NULL; 
	struct route **routes = NULL;
	struct routehandler **handlers = NULL;
	uint8_t *buf = NULL;
	char *statdir = NULL;
	int buflen = 0;
	char lconfpath[ 2048 ] = { 0 };
	char err[ 2048 ] = { 0 };
	struct n { const char *name; struct HTTPRecord **records; } **ttt = 
	(struct n *[]){
		&(struct n){ "headers", req->headers },
		&(struct n){ "get", req->url },
		&(struct n){ "post", req->body },
		NULL
	};

	//Check for config (though this is probably already done...)
	if ( !(config = (struct config *)ctx) )
		return http_set_error( res, 500, "No global config is present." );

	//Initialize Lua environment and add a global table
	lua_State *L = luaL_newstate();
	luaL_openlibs( L );

	//Pick up the local Lua config
	if ( snprintf( lconfpath, sizeof( lconfpath ), "%s%s", config->path, lconfname ) == -1 ) 
		return http_set_error( res, 500, "Could not get full config path." );

	if ( !lua_exec_file( L, lconfpath, err, sizeof( err ) ) )
		return http_set_error( res, 500, err );

	if ( !( c = malloc( sizeof(Table) ) ) || !lt_init( c, NULL, 1024 ) )
		return http_set_error( res, 500, "Failed to allocate config table." );

	if ( !lua_to_table( L, 1, c )  )
		return http_set_error( res, 500, "Failed to convert config table." );

	//Clean up the stack after loading config file...
	lua_pop(L, 1);

	//Check for and serve any static files 
	//TODO: This should be able to serve a list of files matching a specific type
	if ( check_static( c, req->path ) )
		return filter_static( req, res, config );

	//Check that the path resolves to anything. 404 if not, I suppose
	routes = build_routes( c );
	while ( routes && *routes ) {
		if ( resolve( (*routes)->routename, req->path ) ) break;
		routes++;
	}

	if ( !(*routes) ) {
		snprintf( err, sizeof(err), "Path %s does not resolve.", req->path );
		return http_set_error( res, 404, err );
	}

#if 0
	//Errors can be handled... Don't know how yet...
	//The required keys need to be pulled out.
	config_set_path( c, "path" );	
	config_set_path( c, "db" );	
	config_set_path( c, "template_engine" );	
#endif

	//Put all of the HTTP data on the stack
	lua_newtable( L ); 
	while ( ttt && *ttt ) {
		lua_pushstring( L, (*ttt)->name ); 
		//TODO: Nils should probably be nils, not empty tables...
		lua_newtable( L );
		while ( (*ttt)->records && (*(*ttt)->records)->field ) {
			struct HTTPRecord *r = (*(*ttt)->records);
		#if 0
			lua_pushustrings( L, r->field, r->value, r->size );
		#else
			lua_pushstring( L, r->field );
			lua_pushlstring( L, (char *)r->value, r->size );
		#endif
			lua_settable( L, 3 );
			(*ttt)->records++;
		}
		lua_settable( L, 1 );
		ttt++;
	}

	//Push the path as well
#if 0
	lua_pushstrings( L, "url", req->path );
#else
	lua_pushstring( L, "url" );
	lua_pushstring( L, req->path );
#endif
	lua_settable( L, 1 );

#if 0
	//Set framework methods here...
	//lua_set_methods( L, "newenv", libs );
#else
	//Put whatever is now on the stack in 'newenv'
	//lua_set_methods( L, "newenv"... );
	lua_setglobal( L, "newenv" );	
	lua_stackdump( L );
#endif

	//Models
	handlers = (*routes)->elements;
	while ( handlers && *handlers ) {
		char filename[ 2048 ] = { 0 };
		snprintf( filename, sizeof( filename ), "%s/app/%s.lua", config->path, (*handlers)->filename );
		if ( (*handlers)->type == BD_MODEL && !lua_exec_file( L, filename, err, sizeof(err) ) ) {
			fprintf( stderr, "Failed to execute model filename: %s\n", filename );
			return http_set_error( res, 500, err );
		} 
		handlers++;	
	}

	//Aggregate all of the model data.
	if ( !( t = malloc( sizeof( Table )) ) || !lt_init( t, NULL, 2048 ) ) {
		return http_set_error( res, 500, "Failed to allocate space for results table" );
	}

	if ( !lua_combine( L, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err );
	}

	if ( !lua_to_table( L, 1, t ) ) {
		return http_set_error( res, 500, "Failed to re-align Lua data" );
	}

	//Views
	handlers = (*routes)->elements;
	while ( handlers && *handlers ) {
		if ( (*handlers)->type == BD_VIEW ) { 
			uint8_t *fbuf, *rendered;
			int rendered_len = 0, flen = 0;
			char fpath[ 2048 ] = { 0 };
			snprintf( fpath, sizeof( fpath ), "%s/views/%s.%s", config->path, (*handlers)->filename, "tpl" );

			//Read the entire file to render in and fail...
			if ( !( fbuf = read_file( fpath, &flen, err, sizeof( err ) ) ) ) {
				fprintf( stderr, "Failed to read entire view file: %s\n", fpath );
				return http_set_error( res, 500, err );
			}

			//Then do the render ( model.? )
			if ( !( rendered = table_to_uint8t( t, fbuf, flen, &rendered_len ) ) ) {
				fprintf( stderr, "Failed to render model according to view file: %s\n", fpath );
				return http_set_error( res, 500, err );
			}

			//Append to the whole message
			append_to_uint8t( &buf, &buflen, rendered, rendered_len );
		} 
		handlers++;	
	}

	http_set_status( res, 200 );
	http_set_ctype( res, "text/html" );
	http_set_content( res, buf, buflen );
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err );
	}

	return http_set_error( res, 200, "Lua probably ran fine..." );
	return 1;
}
