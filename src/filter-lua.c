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

#if 1
	//Pick up the local Lua config
	snprintf( lconfpath, sizeof( lconfpath ), "%s%s", config->path, lconfname );  
	if ( !lua_exec_file( L, lconfpath, err, sizeof( err ) ) )
		return http_set_error( res, 500, err );

	if ( !( c = malloc( sizeof(Table) ) ) || !lt_init( c, NULL, 1024 ) )
		return http_set_error( res, 500, "Failed to allocate config table." );

	if ( !lua_to_table( L, 1, c ) )
		return http_set_error( res, 500, "Failed to convert config table." );

#if 0
	lua_stackdump( L );
	return http_set_error( res, 200, "Lua stopped after config..." );
#endif
	if ( c ) {
		lua_pop( L, 1 );
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
	}
#endif

	return http_set_error( res, 200, "Lua stopped after config..." );

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
	return http_set_error( res, 200, "Lua probably ran fine..." );

	//Loop through whatever was left of routes...
	while ( (*routes)->elements && ( *(*routes)->elements ) {

		(*routes)->elements++; 
	}

#if 0
	//Try running a few files and see what the stack looks like
#if 0
	f = <what is on this side?>;
	while ( f && *f ) {
#else
	char *files[] = { "www/main.lua", "www/etc.lua", "www/def.lua" };
	for ( int i=0; i<3;i++ ) {
		char *f = files[i];
#endif
	#if 0
		//Check the models first
		if ( (*f)->type == L_MODEL && !lua_exec_file( L, *files, err, sizeof(err) ) ) {
			return http_error( 500, err );
		}   
	#else
		int lerr = 0;  //Follow errors with this
		char fileerr[2048] = {0};
		memset( fileerr, 0, sizeof(fileerr) );

		//Load the file first 
		fprintf( stderr, "Attempting to load file: %s\n", f );
		if (( lerr = luaL_loadfile( L, f )) != LUA_OK ) { 
			int errlen = 0;
			if ( lerr == LUA_ERRSYNTAX )
				errlen = snprintf( fileerr, sizeof(fileerr), "Syntax error at file: %s", f );
			else if ( lerr == LUA_ERRMEM )
				errlen = snprintf( fileerr, sizeof(fileerr), "Memory allocation error at file: %s", f );
			else if ( lerr == LUA_ERRGCMM )
				errlen = snprintf( fileerr, sizeof(fileerr), "GC meta-method error at file: %s", f );
			else if ( lerr == LUA_ERRFILE ) {
				errlen = snprintf( fileerr, sizeof(fileerr), "File access error at: %s", f );
			}
			//WRITE_HTTP_500( fileerr, (char *)lua_tostring( L, -1 ) );
			lua_pop( L, lua_gettop( L ) );
			break;
		}

		//Then execute
		fprintf( stderr, "Attempting to execute file: %s\n", f );
		if (( lerr = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
			if ( lerr == LUA_ERRRUN ) 
				snprintf( fileerr, sizeof(fileerr), "Runtime error at: %s", f );
			else if ( lerr == LUA_ERRMEM ) 
				snprintf( fileerr, sizeof(fileerr), "Memory allocation error at file: %s", f );
			else if ( lerr == LUA_ERRERR ) 
				snprintf( fileerr, sizeof(fileerr), "Error while running message handler: %s", f );
			else if ( lerr == LUA_ERRGCMM ) {
				snprintf( fileerr, sizeof(fileerr), "Error while runnig __gc metamethod at: %s", f );
			}
			//fprintf(stderr, "LUA EXECUTE ERROR: %s, stack top is: %d\n", fileerr, lua_gettop(L) );
			//WRITE_HTTP_500( fileerr, (char *)lua_tostring( L, -1 ) );
			lua_pop( L, lua_gettop( L ) );
			break;
		}
	#endif
	}

	//If we're here, and the stack has values, start running renders
	//This is the model.
	lua_stackdump( L );

	//Convert these results to Table
#if 0
	if ( !( t = malloc( sizeof( Table )) ) || !lt_init( t, NULL, 2048 ) ) {
		return http_err( 500, "Failed to allocate space for results table" );
	}

	if ( !lua_aggregate( L, 1 ) || !lua_to_table( L, 1, t ) ) {

	}
#endif

	//Evaluate views (if any)
#if 0
	f = <what is on this side?>;
	while ( f && *f ) {
#else
	//char *files[] = { "www/main.lua", "www/etc.lua", "www/def.lua" };
	for ( int i=0; i<3;i++ ) {
		char *f = files[i];
#endif
	#if 0
		//Check the models first
		if ( (*f)->type == L_VIEW && !table_to_uint8t( t, msg, 0, &mlen ) ) {
			return http_error( 500, err );
		}   
	#endif
	}

	//Prepare a message with the new thing...
	//http_set_content( res, msg, mlen );

	//Close Lua environment
	lua_close( L );	

#if 1
	if ( 0 /*!http_finalize_response( ... )*/ ) {
		return http_set_error( res, 500, "Problem finalizing response." );
	}
#else
	//Write the response (this should really be a thing of its own)
	if ( write( fd, http_200_fixed, strlen(http_200_fixed)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}
#endif
#endif

	return 0;
}
