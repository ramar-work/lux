/*
* chains.c 
* ========
*
* Test Hypno's concept of 'chains'.  
* 
* Try running some set of files and putting a bunch
* of data on the stack.  Combine said data and render
* it in a view.  Should work consistently...
* 
*/
#include "../vendor/single.h"
#include "../vendor/http.h"
#include "../bridge.h"

#define TEST_DIR "tests/chains-data/"

Loader ld[] = {
	//The entire path should be resolved.  I don't see a reason to do it twice.
	//Additional, you can check it seperately and make sure that all files are available

	//Sadly though, this is slightly more complicated than it should be...
	{ CC_MODEL, TEST_DIR "model1.lua" },
	{ CC_MODEL, TEST_DIR "model2.lua" },
	{ CC_MODEL, TEST_DIR "model3.lua" },
	{ CC_VIEW , TEST_DIR "view1.tpl" },
	{ CC_VIEW , TEST_DIR "view2.tpl" },
	{ 0, NULL }
};


// Resolving directories can happen first
int resolve_chain ( )
{
	return 0;
}


Option opts[] = 
{
	//Debugging and whatnot
	{ "-d", "--dir",       "Choose this directory for serving web apps.",'s' },
	{ "-f", "--file",      "Try running a file and seeing its results.",'s' },
	{ .sentinel = 1 }
};


int main (int argc, char *argv[])
{
#if 0
	//Values
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);
#endif

	//...
	Buffer bc;
	Render R;
	Table t; 
	char err[ 2048 ] = { 0 };
	lua_State *L = luaL_newstate();

	//A buffer would typically be initialized here.
	if ( !bf_init( &bc, NULL, 1 ) )
		return err( 2, "Buffer failed to initialize." );

	//Set a pointer to the test data.
	Loader *ll = ld;
	fprintf( stdout, "Evaluating all models...\n" );

	/*
	To get rendering to work properly there are two ways to approach it.
	A) take all the agg values and put them in one table...
		( assuming that lua_aggregate( L ) is the last function called, add
		a new table, a string to the stack and either move or copy the table
		that's at the beginning to the end.  the stack would look like:
		[0] => { ... },
		[1] => "model",
		[2] => { } 

		set it via lua_settable()
		[0] => { "model" = { ... } }

	B) each file name can be a "scope"
		so before aggregation, add a string, then hit lua_load_file
		do lua_settable before moving to the next file:

		[0] = { ... }
		[1] = <filename>
		[2] = { <results of evaluated file> }
	
		at aggregate, you'll have one table with all "scopes":
		[0] = {
			file1 = {
				{ ... }	
			},
			file2 = {
				{ ... }	
			}
		}

	*/

#if 0
	//The end result here returns a buffer.
	while ( ll->type ) 
	{
		//If anything fails, return NULL and throw a 500. 
		if ( ll->type == CC_MODEL )
		{
			fprintf( stdout, "%s\n", ll->content );	

			//Successful calls will put results on the stack. 
			if ( !lua_load_file( L, ll->content, err ) ) 
			{
				//The lua_load_file function SHOULD modify the error message
				return err( 3, err );
			}
		}
		ll++;
	}

	//Aggregate all values on the stack.
	lua_aggregate( L );
lua_stackdump(L);
#else
	char *ff = "tests/chains-data/ezmodel.lua";
	if ( !lua_load_file( L, ff, err ) )
		return err( 134, "Failed to load file: %s\n", ff ); 
#endif

	//Initialize the table structure
	lt_init( &t, NULL, 1024 );

	//Convert Lua to Table
	if ( !lua_to_table( L, 1, &t ) )
		err( 5, "Failed to convert lua_stack values to table...\n" );

	//Show the table after conversion from Lua
	lt_dump( &t );

	//Rewind pointer
	ll = ld;
	fprintf( stdout, "Evaluating all views...\n" );

	//Then render all the views
	while ( ll->type )
	{
		if ( ll->type == CC_VIEW )
		{
			//Load up the file for the rendering engine
			int fd = open( ll->content, O_RDONLY );
			int br = 0;
			uint8_t *ren = NULL;
			struct stat sb;
		
			if ( fd == -1 )
				err( 4, "Failed to open file %s: %s\n", ll->content, strerror( errno ) );

			if ( stat( ll->content, &sb ) == -1 )
				err( 5, "Failed to stat file %s: %s\n", ll->content, strerror( errno ) );

			//Can use malloc or something else...
			if ( !( ren = malloc( sb.st_size ) ) )
				err( 6, "Failed to allocate needed file size.\n" ); 
	
			memset( ren, 0, sb.st_size );
				
			if (( br = read( fd, ren, sb.st_size - 1 )) == -1 )
				err( 4, "Failed to read file %s: %s\n", ll->content, strerror( errno ) );
				
			//Allocate space for rendering
			if ( !render_init( &R, &t ) )
				err( 32, "render_init failed...\n" );

			//"Score" the block to render
			if ( !render_map( &R, (uint8_t *)ren, br ) )
				err( 33, "render_mapping failed...\n" );

			//Start rendering
			if ( !render_render( &R ) )
				err( 34, "render_render failed...\n" );

			//Write to some buffer, and just keep adding
			Buffer *bd = render_rendered( &R ); 
			if ( !bf_append( &bc, bf_data(bd), bf_written(bd)) )
				err( 35, "Failed to append to buffer...\n" );
	
			//
			render_free( &R );
			free( ren );
			close( fd );
		}

		//Next one
		ll++;
	}

	//Dump the final version of the content.
	write( 1, bf_data( &bc ), bf_written( &bc ) );
	fflush( stdout );

	return 0;
}
