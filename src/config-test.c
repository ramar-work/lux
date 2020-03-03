#include "luabind.h"
#include "config.h"

//this is the best way to set config keys
struct keyset { const char *name; int (*fp)( void *p ); }; 

struct keyset global_config[] = {
#if 1
	NULL
#else
	{ 
#endif
};


struct keyset individual_config[] = {
};

void dump_hosts ( struct host **set ) {
	struct host **r = set;
		fprintf( stderr, "\t%p => ", set );
	while ( r && *r ) {
		fprintf( stderr, "\t%p => ", *r );
		fprintf( stderr, "%s => \n", (*r)->name );
		fprintf( stderr, "%s => \n", (*r)->alias );
		fprintf( stderr, "%s => \n", (*r)->dir );
		fprintf( stderr, "%s => \n", (*r)->filter );
		r++;
	}
}

void dump_routes ( struct route **set ) {
	struct route **r = set;
	fprintf( stderr, "%p =>\n", r );
	while ( r && *r ) {
		fprintf( stderr, "\t%p => ", *r );
		fprintf( stderr, "%s => \n", (*r)->routename );
		for ( int ii=0; ii < (*r)->elen; ii++ ) {
			struct routehandler *t = (*r)->elements[ ii ];
			fprintf( stderr, "\t\t{ %s=%s }\n", get_route_key_type(t->type), t->filename );
		}
		r++;
	}	
}

//Loads some random files
int main (int argc, char *argv[]) {

	const char *files[] = {
		"www/config.lua",  //global config example (won't have routes)
		//"www/def.lua",		 //local config example (will have routes)
		//"www/dafoodsnob/config.lua", //local config example (w/ routes, but no complex models)
	};

	for ( int i=0; i < sizeof(files)/sizeof(const char *); i++ ) {

		//Pull some keys from the config file?
		lua_State *L = luaL_newstate();
		const char *f = files[i];
		char err[2048] = {0};
		Table *t = NULL;
		fprintf( stderr, "Attempting to load and parse %s\n", f );

		//Loading a config file is awful difficult...
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

		if ( 0 ) {
			lt_dump( t );
		}

		//This isn't superflexible now...
		struct host **hostlist = build_hosts( t );
		//What if there were no printf.  Just strings? (How sick would that be?)
		//"Contains hosts: "
		fprintf( stderr, "Contains hosts:\n" );
		dump_hosts( hostlist );
#if 0
		struct route **routelist = build_routes( t );
		int item = get_int_value( t, "number", -1 ); 
		char *wash = get_char_value( t, "wash" );

		fprintf( stderr, "(%s).number: %d\n", f, item );
		fprintf( stderr, "(%s).wash: %s\n", f, wash );
		fprintf( stderr, "(%s).hosts:  %p\n", f, hostlist );
		fprintf( stderr, "(%s).routes: %p\n", f, routelist );
		dump_routes( routelist );
#endif
	}
	return 0;
}
