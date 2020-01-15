/* ---------------------------------------------------
testrender.c 

Test out rendering...
 * --------------------------------------------------- */
#include "../vendor/single.h"
#include "../bridge.h"

const char *files[] = {
	"african", 
	"castigan", 
#if 0
	"roche", 
	"tyrian"
#endif
};


void error ( char *msg ) {
	fprintf( stderr, "testrender: %s\n", msg );
	exit( 1 );
}



int main (int argc, char *argv[]) {
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };

	//A good test would be to modify this to where either files*[] can be run or a command line specified file.

	for ( int i=0; i < sizeof(files)/sizeof(char *); i++ ) {
		//Filename
		Render R;
		Table t; 
		int fd = 0;
		int br = 0;
		char ren[ 10000 ] = { 0 };
		char *m = strcmbd( "/", "tests/render-data", files[i], files[i], "lua" );
		char *v = strcmbd( "/", "tests/render-data", files[i], files[i], "tpl" );
		m[ strlen( m ) - 4 ] = '.';
		v[ strlen( v ) - 4 ] = '.';

		//Choose a file to load
	#if 0
		fprintf( stderr, "Loading model file %s\n", m );
		if ( !lua_load_file( L, m, err ) ) {
			fprintf( stderr, "%s\n", err );
			goto cleanit;
		}
	#else
		char *f = m;
		fprintf( stderr, "Attempting to load file: %s\n", f );
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
			
			fprintf( 
			WRITE_HTTP_500( fileerr, (char *)lua_tostring( L, -1 ) );
			lua_pop( L, lua_gettop( L ) );
			break;
		}

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
			//fprintf(stderr, "LUA EXECUTE ERROR: %s, stack top is: %d\n", fileerr, lua_gettop(L) );
			WRITE_HTTP_500( fileerr, (char *)lua_tostring( L, -1 ) );
			lua_pop( L, lua_gettop( L ) );
			break;
		}

		//Convert Lua to Table
		lt_init( &t, NULL, 1024 );
		lua_stackdump( L );
		if ( !lua_to_table( L, 1, &t ) ) {
			fprintf( stderr, "%s\n", err );
			goto cleanit;
		}
	#endif

		//Show the table after conversion from Lua
		if ( 1 ) {
			lt_dump( &t );
		}
		
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
