/* ---------------------------------------------------
testrender.c 

Test out rendering...
 * --------------------------------------------------- */
#include "vendor/single.h"
#include "bridge.h"

const char *files[] = {
	"african", 
	"castigan", 
#if 0
	"roche", 
	"tyrian"
#endif
};


int main (int argc, char *argv[])
{
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };

	//A good test would be to modify this to where either files*[] can be run or a command line specified file.

	for ( int i=0; i < sizeof(files)/sizeof(char *); i++ )
	{
		//Filename
		Render R;
		Table t; 
		int fd = 0;
		int br = 0;
		char ren[ 10000 ] = { 0 };
		char *m = strcmbd( "/", "tests/render", files[i], files[i], "lua" );
		char *v = strcmbd( "/", "tests/render", files[i], files[i], "tpl" );
		m[ strlen( m ) - 4 ] = '.';
		v[ strlen( v ) - 4 ] = '.';

		//Choose a file to load
		fprintf( stderr, "Loading model file %s\n", m );
		if ( !lua_load_file( L, m, err ) )
		{
			fprintf( stderr, "%s\n", err );
			goto cleanit;
		}

		//Convert Lua to Table
		lt_init( &t, NULL, 1024 );
		lua_stackdump( L );
		if ( !lua_to_table( L, 1, &t ) )
		{
			fprintf( stderr, "%s\n", err );
			goto cleanit;
		}

		//Show the table after conversion from Lua
		if ( 1 )
			lt_dump( &t );
		
		//Prepare the rendering engine
		fprintf( stderr, "Rendering against view file %s\n", v );
		if ( !render_init( &R, &t ) )
			{ fprintf( stderr, "render_init failed...\n"); goto cleanit ; }

		//Load up the file for the rendering engine
		fd = open( v, O_RDONLY );
		if (( br = read( fd, ren, sizeof( ren ) - 1 )) == -1 )
			{ fprintf( stderr, "loading file '%s' failed...\n", v); goto cleanit ; }

		//Dump the block
		if ( 1 )
			write( 2, ren, br );
		
		//"Score" the block to render
		if ( !render_map( &R, (uint8_t *)ren, br ) )
			{ fprintf( stderr, "render mapping failed...\n"); goto cleanit ; }

		//Start rendering
		if ( !render_render( &R ) )
			{ fprintf( stderr, "render_init failed...\n"); goto cleanit ; }

		//Show the results
		if ( 1 )
			write( 2, bf_data( render_rendered( &R ) ), bf_written( render_rendered( &R )) );

		//Clean up
		render_free( &R );
		lt_free( &t );

		//Free things
cleanit:
		if ( fd ) { 
			close(fd); 
			fd = 0; 
		}
		lua_settop( L, 0 );
		free( m );
		free( v );
	}
}
