#include "filter-lua.h"


#define lua_pushustrings(STATE,KEY,VAL,VLEN) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, VLEN )


#define lua_pushstrings(STATE,KEY,VAL) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, strlen( VAL ))


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
	snprintf( lconfpath, sizeof( lconfpath ), "%s%s", config->path, lconfname );  
	if ( !lua_exec_file( L, lconfpath, err, sizeof( err ) ) )
		return http_set_error( res, 500, err );

	if ( !( c = malloc( sizeof(Table) ) ) || !lt_init( c, NULL, 1024 ) )
		return http_set_error( res, 500, "Failed to allocate config table." );

	if ( !lua_to_table( L, 1, c )  )
		return http_set_error( res, 500, "Failed to convert config table." );

	//lua_pop( L, 1 );
	//lua_stackdump( L );
	//lt_dump( c );
#if 0
	//First, check that we don't have a static path.
	if ( static ) {
		if ( !filter_static( rq, rs, ctx ) ) {
			//filter_static should have prepared this...
			return 0;
		}
	}
#endif
	lua_pop(L, 1);

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

	handlers = (*routes)->elements;	
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
	//Additionally, the framework methods and whatnot are also needed...
	//lua_set_methods( L, ... );
#endif

	//Assign this globally
	lua_setglobal( L, "newenv" );	
	lua_stackdump( L );

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

fprintf( stderr, "SOURCE\n=========\n" );
write(2,fbuf,flen);

			//Then do the render ( model.? )
			if ( !( rendered = table_to_uint8t( t, fbuf, flen, &rendered_len ) ) ) {
				fprintf( stderr, "Failed to render model according to view file: %s\n", fpath );
				return http_set_error( res, 500, err );
			}

fprintf( stderr, "RENDERED\n=========\n" );
write(2,rendered,rendered_len);
getchar();
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
