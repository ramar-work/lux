#include "render.h"

#define TESTDIR "tests/render/"

struct block { const char *model, *view; } files[] = {
	{ TESTDIR "empty.lua", TESTDIR "empty.tpl" },
	{ TESTDIR "simple.lua", TESTDIR "simple.tpl" },
	{ TESTDIR "castigan.lua", TESTDIR "castigan.tpl" },
	{ TESTDIR "multi.lua", TESTDIR "multi.tpl" },
	{ NULL }
};

int main (int argc, char *argv[]) {
	char err[ 2048 ] = { 0 };
	struct block *f = files;

	while ( f->model ) {
		Table *t = NULL; 
		int blen = 0, rlen = 0;
		uint8_t *buf = NULL, *rendered = NULL;
		lua_State *L = luaL_newstate();

		//Run the Lua file
		if ( !lua_exec_file( L, f->model, err, sizeof( err ) ) ) {
			fprintf(stderr, "Error loading/executing model file: %s, %s\n", f->model, err );
			goto die;	
		}

		//Allocate a new "Table" structure...
		if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 2048 )) {
			fprintf( stderr, "MALLOC ERROR FOR TABLE: %s\n", strerror( errno ) );
			goto die;	
		}

		//Convert Lua to Table
		if ( !lua_to_table( L, 1, t ) ) {
			fprintf( stderr, "%s\n", err );
			goto die;	
		}

		if ( !( buf = read_file( f->view, &blen, err, sizeof( err ) ) ) ) {
			fprintf(stderr, "Error reading template file: %s, %s\n", f->view, err );
			goto die;	
		}

		//Finding the marks is good if there is enough memory to do it
		if (( rendered = table_to_uint8t( t, buf, blen, &rlen ) ) == NULL ) {
			fprintf(stderr, "Error rendering template file: %s\n", f->view );
			goto die;	
		}

		write( 2, rendered, rlen );

die:
		free( rendered );
		free( buf );
		if ( t ) { 
			lt_free( t );
			free( t );
		}
		lua_close( L );
		f++;
	}
	return 0;
}
