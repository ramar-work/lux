#include "luabind.h"
#include "config.h"

struct keyset { const char *name; int (*fp)( void *p ); } keyset[] = {
	NULL
};

//Loads some random files
int main (int argc, char *argv[]) {

	//Pull some keys from the config file?
	int lerr;
	lua_State *L = luaL_newstate();
	const char *f = "www/def.lua";
	char err[2048] = {0};
	Table *t = NULL;

	if ( !lua_exec_file( L, f, err, sizeof(err) ) ) {
		fprintf( stderr, "%s\n", err );
		return 1;
	}

	if ( !( t = malloc( sizeof(Table) ) ) || !lt_init( t, NULL, 2048 ) ) {
		fprintf( stderr, "Failed to allocate space for new Table data.\n" );
		return 1;
	}

	if ( !lua_to_table( L, 1, t ) ) {
		fprintf( stderr, "Failed to convert Lua data to table.\n" );
		return 1;	
	}

	lt_dump( t );


	//struct route **hostlist = NULL;
	//get_values( t, "hosts", (void *)hostlist, host_table_iterator );

	struct route **routelist = NULL;
	get_values( t, "routes", (void *)routelist, route_table_iterator );

#if 0	
	//Loop through the keys specified and get something
	getkey( t, "routes" );	
	getkey( t, "db" );	
	getkey( t, "template_engine" );	

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
