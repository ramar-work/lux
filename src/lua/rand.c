#include "rand.h"


static char ascii[] = 
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
;


static char numerics[] = 
	"0123456789"
;

static unsigned char * generate ( char *str, unsigned int size ) {
	unsigned char * buf = NULL;
	struct timespec t = { 0 };
	if ( clock_gettime( CLOCK_REALTIME, &t ) == -1 ) {
		return NULL;
	}

	if ( size > 65536 /*Short max on most systems*/ ) {
		return NULL;
	}

	if ( !( buf = malloc( size ) ) || !memset( buf, 0, size ) ) {
		return NULL;
	}

	//Use the current timestamp as a random seed for now...
	srand( t.tv_nsec );
	for ( int i = 0, len = strlen( str ); i < size - 1; i++ ) {
		buf[ i ] = str[ rand() % len ];
	}

	return buf;
}


int rand_str ( lua_State *L ) {
	luaL_checknumber( L, 1 );
	unsigned char *buf = NULL;
	int bfsize = lua_tonumber( L, 1 );
	if ( !( buf = generate( ascii, bfsize - 1 ) )  ) {
		return luaL_error( L, "rand.str failed." );
	}
	lua_pushstring( L, ( char * )buf );
	return 1;
}


int rand_seq ( lua_State *L ) {
	luaL_checknumber( L, 1 );
	int bfsize = lua_tonumber( L, 1 );
	unsigned char *buf = NULL;
	if ( !( buf = generate( ascii, bfsize - 1 ) )  ) {
		return luaL_error( L, "rand.str failed." );
	}
	lua_pushstring( L, ( char * )buf );
	return 1;
}


int rand_nums ( lua_State *L ) {
	luaL_checknumber( L, 1 );
	int bfsize = lua_tonumber( L, 1 );
	unsigned char *buf = NULL;
	if ( !( buf = generate( ascii, bfsize - 1 ) )  ) {
		return luaL_error( L, "rand.str failed." );
	}
	lua_pushstring( L, ( char * )buf );
	return 1;
}


struct luaL_Reg rand_set[] = {
 	{ "nums", rand_nums }
,	{ "str", rand_str }
,	{ "seq", rand_seq }
//,	{ "number", fs_write }
,	{ NULL }
};
