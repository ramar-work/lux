
//Loads some random files
int main (int argc, char *argv[]) {

#if 0	
	int lerr;
	lua_State *L = luaL_newstate();
	const char *f = "www/def.lua";
	char fileerr[2048] = {0};
#if 1
//START lua_load_file	
	//Load route table file
	fprintf( stderr, "Attempting to parse file: %s\n", f );
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
		
		fprintf(stderr, "LUA LOAD ERROR: %s, %s", fileerr, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		exit( 1 );
	}
	fprintf( stderr, "SUCCESS!\n" );

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

		fprintf(stderr, "LUA EXEC ERROR: %s, %s", fileerr, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		exit( 1 );
	}
	fprintf( stderr, "SUCCESS!\n" );
//END lua_load_file	
#endif

	Table *t = malloc( sizeof(Table) );
	lt_init( t, NULL, 1024 );
	if ( !lua_to_table( L, 1, t ) ) {
		FPRINTF( "Failed to convert Lua data to table.\n" );
		exit(0);
	}
	//lt_dump( t );

	//Find the routes index, use that as start and move?
	//struct route **routelist = NULL; //This could be static too... less to clean
	struct routeset r = { 0, NULL };
	int routesIndex = lt_geti( t, "routes" );
	int count = lt_counti( t, routesIndex );
	FPRINTF( "routes key at: %d\n", routesIndex );

	//Combine the routes and save each combination (strcmbd?)
	if ( !lt_exec_complex( t, routesIndex, t->count, (void *)&r, buildRoutes ) ) {
		FPRINTF( "At end of route data.\n" );
	}

	for ( int i=0; i < r.len; i++ ) {
		struct route *rr = r.routes[i];
		fprintf( stderr, "[%d] %s => ", i, rr->routename );
		fprintf( stderr, "route composed of %d files.\n", rr->elen );
		for ( int ii=0; ii < rr->elen; ii++ ) {
			struct routehandler *t = rr->elements[ ii ];
			fprintf( stderr, "\t{ %s=%s }\n", DUMPTYPE(t->type), t->filename );
		}
	}
#endif	
	return 0;
}
