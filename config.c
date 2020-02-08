//Compile me with: 
//gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
#include "vendor/single.h"
#include "luabind.h"

#define FPRINTF(...) \
	fprintf( stderr, "DEBUG: %s[%d]: ", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ );

#define ADDITEM(TPTR,SIZE,LIST,LEN) \
	if (( LIST = realloc( LIST, sizeof( SIZE ) * ( LEN + 1 ) )) == NULL ) { \
		fprintf (stderr, "Could not reallocate new rendering struct...\n" ); \
		return NULL; \
	} \
	LIST[ LEN ] = TPTR; \
	LEN++;

//struct route { char *routename, **elements; };
struct config { char *item; int (*fp)( char * ); };
struct routetype { char *filename; int type; };
struct route { char *routename; char *parent; int elen; struct routetype **elements; };
struct routeset { int len; struct route **routes; };

struct config conf[] = {
	{ "engine", NULL }
,	{ "db"    , NULL }
,	{ "routes", NULL }
,	{ NULL, NULL }
};

const char *keys[] = {
	"returns"
,	"content-type"
,	"query"
,	"model"
,	"view"
,	"routes"
,	"hint"
,	"auth"
,	NULL
};

const char *keysstr = 
	"returns" \
	"content-type" \
	"query" \
	"model" \
	"view" \
	"routes" \
 	"hint" \
	"auth"
;

int c = 0;
int b = 0;
char *parent[100] = { NULL };

int buildRoutes ( LiteKv *kv, int i, void *p ) {
	struct routeset *r = (struct routeset *)p;

	//Right side should be a table.
	//FPRINTF( "Index: %d\n", i );
	char *name = NULL; 
	if ( kv->key.type == LITE_TXT ) {
		//FPRINTF( "Left: %s => ", kv->key.v.vchar );
		name = kv->key.v.vchar;
	}
	else if ( kv->key.type == LITE_TRM ) {
		//Safest to add a null member to the end of elements
		b--;
		if ( b == 0 ) {
			return 0;	
		}
		else { 
			if ( b == 1 ) {
				for ( int i=0; i < sizeof( parent ) / sizeof( char * ); i++ ) {
					parent[ i ] = NULL;
				}	
			}
		}
	}

	if ( kv->value.type == LITE_TBL ) {
		//FPRINTF( "Right: %s\n", lt_typename( kv->value.type ) );
		//Only add certain keys, other wise, they're routes...
		if ( name ) {
			if( !memstr( keysstr, name, strlen(keysstr) ) ) {
				struct route *rr = malloc( sizeof(struct route) );
				char buf[ 1024 ] = {0};
				char *par = parent[ b - 1 ];
				rr->elen = 0;
				rr->elements = NULL;
				if ( !par ) {
					buf[ 0 ] = '/';
					memcpy( &buf[ 1 ], name, strlen( name ) );
					rr->routename = strdup( buf );
				}
				else {
					int slen = 0;
					memcpy( buf, par, strlen( par ) );
					slen += strlen( par );
					memcpy( &buf[ slen ], "/", 1 );
					slen += 1;
					memcpy( &buf[ slen ], name, strlen( name ) );
					slen += strlen( name );
					rr->routename = strdup( buf );
				}
				parent[ b ] = rr->routename;
				//FPRINTF( "Got route name (%s), c. parent (%s), n.parent (%s)\n", name, par, parent[ b ] );
				ADDITEM( rr, struct route *, r->routes, r->len );  
				c = 0;
			}
			else {
				FPRINTF( "Got prepared key: %s\n", name );
				//Get the last set one
				if ( r->len > 0 ) {
					struct route *rr = r->routes[ r->len - 1 ];
					struct routetype *t = malloc( sizeof( struct routetype ) );
					t->filename = strdup( name );
					t->type = 0;
					ADDITEM( t, struct routetype *, rr->elements, rr->elen ); 
				}

				//Mark which one you got somehow...
				c = 1;
			}
			b++;
		}
	}
	//Keep track of all the sentinels....
	//FPRINTF( "Depth: %d\n", b );
	//getchar();
	return 1;
}

int main (int argc, char *argv[]) {

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

	//The stack SHOULD contain the set of routes and handlers...
	//lua_stackdump( L );

	//There is more than a route table here.
	//db config, and much more is probably going to be here...

	//Build static route list?
	//1. Use inherent hash table prims (map /route to [route.0.etc])
	//	This method is MUCH faster...  but not as flexible
	//	(can't reallly support wildcards, eithers or parameters)
	//	(I would need to resolve part of the hash, and that's not really helpful)
	//2. Create full route strings out of each of the keys
	//   Doing this will allow me to make a model and view (and whatever else)
	//   out of a route table
	//So it will look something like:
	//	- /route/x -> { model: ..., view: ..., etc: ... }
	//Then I can just load from there provided that a matching route was found 

	Table *t = malloc( sizeof(Table) );
	lt_init( t, NULL, 1024 );
	if ( !lua_to_table( L, 1, t ) ) {
		FPRINTF( "Failed to convert Lua data to table.\n" );
		exit(0);
	}
	lt_dump( t );

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
		FPRINTF( "[%d] %s\n", i, rr->routename );
	}
	
	return 0;
}
