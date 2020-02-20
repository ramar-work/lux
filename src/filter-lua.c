//filter-lua.c - Run HTTP messages through a Lua handler
//Any processing can be done in the middle.  Since we're in another "thread" anyway
int h_proc ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
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

	//Initialize Lua environment and add a global table
	//fprintf( stderr, "Initializing Lua env.\n" );
	lua_State *L = luaL_newstate();
	luaL_openlibs( L );
	lua_newtable( L ); 

	//Put all of the HTTP data on the stack
	const char *names[] = { "headers", "get", "post" };
	struct HTTPRecord **t[] = { rq->headers, rq->url, rq->body };
	for ( int i=0; i < sizeof(t)/sizeof(struct HTTPRecord **); i++ ) {
		//Add the new name first
		lua_pushstring( L, names[ i ] ); 
		lua_newtable( L );

		//Add each value
		struct HTTPRecord **w = t[i];
		while ( *w ) {
			lua_pushstring( L, (*w)->field );
			lua_pushlstring( L, (char *)(*w)->value, (*w)->size );
			lua_settable( L, 3 );
			w++;
		}
	
		//Set this table and key as a value in the global table
		lua_settable( L, 1 );
	}

	//Push the path as well
	lua_pushstring( L, "url" );
	lua_pushstring( L, rq->path );
	lua_settable( L, 1 );

	//Additionally, the framework methods and whatnot are also needed...
	//...

	//Assign this globally
	lua_setglobal( L, "newenv" );	

	//Try running a few files and see what the stack looks like
	char *files[] = { "www/main.lua", "www/etc.lua", "www/def.lua" };
	for ( int i=0; i<3;i++ ) {
		char *f = files[i];
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
	}

	//If we're here, and the stack has values, start running renders
	//This is the model.
	lua_stackdump( L );

	//View

	//Close Lua environment
	//fprintf( stderr, "Killing Lua env.\n" );
	lua_close( L );	

#if 1
	//Write the response (this should really be a thing of its own)
	if ( write( fd, http_200_fixed, strlen(http_200_fixed)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}
#endif

	return 0;
}
