#include "render.h"

const char *files[] = {
	"multi", 
#if 0
	"castigan", 

	"african", 
	"roche", 
	"tyrian"
#endif
};


int main (int argc, char *argv[]) {
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };

	//A good test would be to modify this to where either files*[] can be run or a command line specified file.
	for ( int i=0; i < sizeof(files)/sizeof(char *); i++ ) {
		Table *t = NULL; 
		int br = 0;
		int fd = 0;
		char ren[ 10000 ] = { 0 };
		char *m = strcmbd( "/", "tests/render-data", files[i], files[i], "lua" );
		char *v = strcmbd( "/", "tests/render-data", files[i], files[i], "tpl" );
		m[ strlen( m ) - 4 ] = '.';
		v[ strlen( v ) - 4 ] = '.';

		//Choose a file to load
		char fileerr[2048] = {0};
		char *f = m;
		int lerr;
		int status = 0;
		fprintf( stderr, "Attempting to load and execute model file: %s\n", f );
		if ( !( status = lua_exec_file( L, f, fileerr, sizeof(fileerr) ) ) ) {
			fprintf(stderr, "Error loading/executing model file: %s, %s\n", f, fileerr);
			return 0;
		}
		fprintf( stderr, "SUCCESS!\n" );

		//Dump the stack
		//lua_stackdump( L );

		//Allocate a new "Table" structure...
		if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 1024 )) {
			fprintf( stderr, "MALLOC ERROR FOR TABLE: %s\n", strerror( errno ) );
			exit( 1 );
		}

		//Convert Lua to Table
		if ( !lua_to_table( L, 1, t ) ) {
			fprintf( stderr, "%s\n", err );
			//goto cleanit;
		}

		//Show the table after conversion from Lua
		if ( 0 ) {
			lt_dump( t );
		}

		uint8_t *buf = NULL;
		int blen = 0;
		if ( !( buf = read_file( v, &blen, fileerr, sizeof(fileerr) ) ) ) {
			fprintf(stderr, "Error reading template file: %s, %s\n", f, fileerr );
			return 0;
		}

		//Finding the marks is good if there is enough memory to do it
		int renderLen = 0;
		uint8_t *rendered = table_to_template( t, buf, blen, &renderLen );
		if ( rendered ) {
			write( 2, rendered, renderLen );
		}

		//Free things
	#if 0
cleanit:
		if ( fd ) { 
			close(fd); 
			fd = 0; 
		}
		lua_settop( L, 0 );
		free( m );
		free( v );
	#endif
	}
	return 0;
}
