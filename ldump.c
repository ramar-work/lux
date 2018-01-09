/*
ldump.c - 

Quick utility for testing dumps...

This should be in some other form...


*/
#include "bridge.h"

const char usage[] = 
	"%s: No files specified.\n\n"	
	"Usage:\n"
	"-m, --models <lua files>\n"
	"-v, --views <template files>\n"
;


//
Option opts[] = 
{
	{ "-m", "--models",       "Choose this directory for serving web apps.",'s' },
	{ "-v", "--views" ,       "Choose this directory for serving web apps.",'s' },

	{ .sentinel = 1 }
};


//
int main (int argc, char *argv[])
{
	lua_State *L = luaL_newstate();
	char errm[2048] = { 0 };
	Loader ld[ 100 ];
	Loader *ly = ld;
	memset( ld, 0, sizeof( Loader ) * 100 ); 

	if ( argc <= 1 )
		return err( 1, usage, argv[0] );

	//Loop through without using options...
	while ( *argv ) 
	{
		//the line after the next is stupid... get rid of that shit...
		//strmatch( *argv, "--models", "-m", "-v", "--views" ) -> put all this crap in, and move the pointer for speed
		if ( strcmp( *argv, "--models" ) == 0 || strcmp( *argv, "--views" ) == 0 ||
				 strcmp( *argv, "-m" ) == 0 || strcmp( *argv, "-v" ) == 0 )
		{
			char type = (*argv)[1];
			argv++;

			while ( *argv ) 
			{
				if ( *(*argv) == '-' ) 
					break;

				ly->type = ( type == 'v' ) ? CC_VIEW : CC_MODEL;	
				ly->content = strdup( *argv );
				argv++, ly++;
			}
			continue;
		}
		argv++;
	}

	//Models
	ly = ld;
	while ( ly->content ) {
		if ( ly->type == CC_MODEL ) {
			fprintf( stderr,"try to run %s\n", ly->content );
			if ( !lua_load_file ( L, ly->content, errm ) ) {
				return err( 13, "lua load file fucked up son: %s\n", ly->content );	
			}
		}
		ly++;
	}

	//Views
	ly = ld;
	Table t;
	Render R;
	Buffer bc;
	memset( &bc, 0, sizeof( Buffer ) );

	if ( !lt_init( &t, NULL, 1024 ) )
		return err( 1, "lt_init failed..." );

	while ( ly->content ) {
		if ( ly->type == CC_VIEW ) {
			fprintf( stderr, "view: %s\n", ly->content );

			//Load up the file for the rendering engine
			int fd = open( ly->content, O_RDONLY );
			int br = 0;
			uint8_t *ren = NULL;
			struct stat sb;
	
			//Check that the opened file doesn't just fail...
			if ( fd == -1 )
				return err( 4, "Failed to open file %s: %s\n", ly->content, strerror( errno ) );
		
			//Check something and do something
			if ( stat( ly->content, &sb ) == -1 )
				return err( 5, "Failed to stat file %s: %s\n", ly->content, strerror( errno ) );

			//Can use malloc or something else...
			if ( !( ren = malloc( sb.st_size ) ) )
				return err( 6, "Failed to allocate needed file size.\n" ); 
	
			if ( !memset( ren, 0, sb.st_size ) )
				return err( 6, "memset( ... ) failed, because your computer and possibly your life suck...\n" ); 
				
			//Read a template file in	
			if (( br = read( fd, ren, sb.st_size - 1 )) == -1 )
				return err( 4, "Failed to read file %s: %s\n", ly->content, strerror( errno ) );
				
			//Allocate space for rendering
			if ( !render_init( &R, &t ) )
				return err( 32, "render_init failed...\n" );

			//"Score" the block to render
			if ( !render_map( &R, (uint8_t *)ren, br ) )
				return err( 33, "render_mapping failed...\n" );

			//Start rendering
			if ( !render_render( &R ) )
				return err( 34, "render_render failed...\n" );

			//Write to some buffer, and just keep adding
			Buffer *bd = render_rendered( &R ); 
			if ( !bf_append( &bc, bf_data(bd), bf_written(bd)) )
				return err( 35, "Failed to append to buffer...\n" );
	
			//...
			render_free( &R );
			free( ren );
			close( fd );
		}
		ly++;
	}

	fprintf( stderr, "written: %d\n", bf_written( &bc )); 
	write(1, bf_data( &bc ), bf_written( &bc )); 
	//lua_stackdump( L );
	return 0;
}
