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


char *append ( char *str, const char **elements ) {
	int len = 0;
	int max = 2048;
	while ( *elements ) {
		memcpy( &str[ len ], *elements, strlen( *elements ) );
		len += strlen( *elements );
		memcpy( &str[ len ], "/", 1 );
		len += 1;
		fprintf(stderr, "%s\n", *elements );
		elements++;
	}

	elements--;
	fprintf( stderr, "%s\n", *elements );
	str[ len - strlen( *elements ) - 2 ] = '.';
	str[ len - 1 ] = '\0';
	return str;
}


int main (int argc, char *argv[]) {
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };

	//A good test would be to modify this to where either files*[] can be run or a command line specified file.
	for ( int i=0; i < sizeof(files)/sizeof(char *); i++ ) {
		Table *t = NULL; 
		char model[2048] = {0}, view[2048] = {0};
		char fileerr[2048] = {0};
		int blen = 0, rlen = 0;
		uint8_t *buf = NULL, *rendered = NULL;

		//Generate the model and view file names
		append( model, (const char *[]){ "tests/render-data", files[i], files[i], "lua", NULL } );
		append( view, (const char *[]){ "tests/render-data", files[i], files[i], "tpl", NULL } );

		//Run the Lua file
		if ( !lua_exec_file( L, model, fileerr, sizeof(fileerr) ) ) {
			fprintf(stderr, "Error loading/executing model file: %s, %s\n", model, fileerr);
			return 0;
		}

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

		if ( !( buf = read_file( view, &blen, fileerr, sizeof(fileerr) ) ) ) {
			fprintf(stderr, "Error reading template file: %s, %s\n", view, fileerr );
			return 0;
		}

		//Finding the marks is good if there is enough memory to do it
		if (( rendered = table_to_uint8t( t, buf, blen, &rlen ) ) == NULL ) {
			fprintf(stderr, "Error rendering template file: %s\n", view );
			return 0;
		}
		write( 2, rendered, rlen );
	}
	return 0;
}
