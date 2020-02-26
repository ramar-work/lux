#include "filter-lua.h"

#define lua_pushustrings(STATE,KEY,VAL,VLEN) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, VLEN )

#define lua_pushstrings(STATE,KEY,VAL) \
	lua_pushstring( STATE, KEY ) && lua_pushlstring( STATE, (char *)VAL, strlen( VAL ))

//filter-lua.c - Run HTTP messages through a Lua handler
//Any processing can be done in the middle.  Since we're in another "thread" anyway
int filter_lua ( struct HTTPBody *req, struct HTTPBody *res, void *ctx ) {

#if 1
	Table *t = NULL;
	uint8_t *msg = NULL;
	int mlen = 0;
	char err[ 2048 ] = { 0 };
	const char *names[] = { "headers", "get", "post" };
#if 0
	struct HTTPRecord ***t = { &req->headers, &req->url, &req->body };
#else
	struct HTTPRecord **tt[] = { req->headers, req->url, req->body };
#endif
#else
	//...
	char *err = malloc( 2048 );
	memset( err, 0, 2048 );
	struct stat sb =  {0};

	//Check that the directory exists
	if ( stat( "www", &sb ) == -1 ) {
		//WRITE_HTTP_500( "Could not locate www/ directory", strerror( errno ) );
		return 0;
	}

	//Check for a primary framework file
	memset( &sb, 0, sizeof( struct stat ) );
	if ( stat( "www/main.lua", &sb ) == -1 ) {
		//WRITE_HTTP_500( "Could not find file www/main.lua", strerror( errno ) );
		return 0;
	}
#endif

	//Initialize Lua environment and add a global table
	//FPRINTF( "Initializing Lua env.\n" );
	lua_State *L = luaL_newstate();
	luaL_openlibs( L );
	lua_newtable( L ); 

	//Put all of the HTTP data on the stack
#if 0
	while ( **t ) {
#else
	for ( int i=0; i < sizeof(t)/sizeof(struct HTTPRecord **); i++ ) {
#endif
		//Add the new name first
		lua_pushstring( L, names[ i ] ); 
		lua_newtable( L );

		//Add each value
		struct HTTPRecord **w = tt[i];
		while ( *w ) {
		#if 0
			lua_pushustrings( L, (*w)->field, (*w)->value, (*w)->size );
		#else
			lua_pushstring( L, (*w)->field );
			lua_pushlstring( L, (char *)(*w)->value, (*w)->size );
		#endif
			lua_settable( L, 3 );
			w++;
		}
	
		//Set this table and key as a value in the global table
		lua_settable( L, 1 );
	}

	//Push the path as well
#if 0
	lua_pushstrings( L, "url", req->path );
#else
	lua_pushstring( L, "url" );
	lua_pushstring( L, req->path );
#endif
	lua_settable( L, 1 );

	//Additionally, the framework methods and whatnot are also needed...
	//lua_set_methods( L, ... );

	//Assign this globally
	lua_setglobal( L, "newenv" );	

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

	return 0;
}
