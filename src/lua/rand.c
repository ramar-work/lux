#include "rand.h"


static char ascii[] = 
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	" !\"#$%&'()*+,-./0123456789"
	":;<=>?@[]^_`{|}~"
;


static char numerics[] = 
	"0123456789"
;

static unsigned char * generate ( unsigned char *str, unsigned int len, unsigned int size ) {
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
	for ( int i = 0; i < size - 1; i++ ) {
		buf[ i ] = str[ rand() % len ];
	}

	return buf;
}


int rand_str ( lua_State *L ) {
	luaL_checknumber( L, 1 );
	unsigned char *buf = NULL;
	int bfsize = lua_tonumber( L, 1 );
	if ( !( buf = generate( (unsigned char *)ascii, sizeof( ascii ) / sizeof( char ), bfsize - 1 ) )  ) {
		return luaL_error( L, "rand.str failed." );
	}

	lua_pushstring( L, ( char * )buf );
	free( buf );
	return 1;
}


int rand_seq ( lua_State *L ) {
	luaL_checknumber( L, 1 );
	int bfsize = lua_tonumber( L, 1 );
	unsigned char *buf = NULL, seq[ 256 ];

	//Generate a sequential buffer from 0 - 255
	memset( seq, 0, 256 );
	for ( int i = 0; i < 256; i++ ) seq[ i ];

	if ( !( buf = generate( (unsigned char *)seq, sizeof( seq ) / sizeof( char ), bfsize - 1 ) )  ) {
		return luaL_error( L, "rand.str failed." );
	}

	//Push w/ embedded zeros...
	lua_pushlstring( L, ( char * )buf, bfsize );
	free( buf );
	return 1;
}


int rand_nums ( lua_State *L ) {
	luaL_checknumber( L, 1 );
	int bfsize = lua_tonumber( L, 1 );
	unsigned char *buf = NULL;
	if ( !( buf = generate( (unsigned char *)numerics, sizeof( numerics ) / sizeof( char ), bfsize - 1 ) )  ) {
		return luaL_error( L, "rand.nums failed." );
	}
	lua_pushstring( L, ( char * )buf );
	free( buf );
	return 1;
}


struct luaL_Reg rand_set[] = {
 	{ "nums", rand_nums }
,	{ "str", rand_str }
,	{ "seq", rand_seq }
//,	{ "number", fs_write }
,	{ NULL }
};
