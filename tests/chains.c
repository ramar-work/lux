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
	{ CC_MODEL, TEST_DIR "model2.lua" },
	{ CC_VIEW , TEST_DIR "view1.lua" },
	{ CC_VIEW , TEST_DIR "view2.lua" },
	{ 0, NULL }
};


// Resolving directories can happen first
int resolve_chain ( )
{
	return 0;
}


// Prepare the chain
Loader *prepare_chain ( Loader *ld, char *reldir )
{
	//Loop through, create a full path and check if those files exist
	
	//The engine would return NULL and probably throw a 404 if the file didn't exist
	//This could also be a server error, since technically the symbolic file exists, but
	//the workings underneath do not.
	return NULL;
}



// void *data is a possible data structure that may keep track of a bunch of data
// like relative directories or something...
Buffer *run_chain ( Loader *ld, Buffer *dest, lua_State *L, char *err )
{
	Loader *ll = ld;
	//printf( "addr of buffer: %p\n", b );

	//The end result here returns a buffer.
	while ( ll->type ) 
	{
		//Header
		fprintf( stderr, "Evaluating %s:\n%s\n", 
			printCCtype( ll->type ), "===================" );

		//Do based on ll->type 
		//If anything fails, return NULL and throw a 500. 
		if ( ll->type == CC_MODEL )
		{
			fprintf( stderr, "%s\n", ll->content );	

			//Successful calls will put results on the stack. 
			if ( !lua_load_file( L, ll->content, err ) ) 
			{
				//Unsuccessful will return NULL, but the stack has the error
				//The lua_load_file function SHOULD modify the error message
				return NULL;
			}
		}
		else if ( ll->type == CC_VIEW )
		{
			//Dump Lua stack for debugging
			
			//Convert Lua to table
			
			//Then render, which should work in one pass...
			fprintf( stderr, "%s\n", ll->content );	
		}

		//Add to buffer depending on choice
		
		//Next one
		ll++;
	}

	return dest;
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

#if 0
	//Then run the chain.
	if ( !run_chain( l, &bc, L, err ) )
		return err( 1, "Chain running failed on block: %s", err );
#else
	//Set a pointer to the test data.
	Loader *ll = ld;

	//The end result here returns a buffer.
	while ( ll->type ) 
	{
		//Header
		fprintf( stderr, "Evaluating %s:\n%s\n", 
			printCCtype( ll->type ), "===================" );

		//Do based on ll->type 
		//If anything fails, return NULL and throw a 500. 
		if ( ll->type == CC_MODEL )
		{
			fprintf( stderr, "%s\n", ll->content );	

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
	lua_stackdump( L ); getchar();
	lua_aggregate( L );
	lua_stackdump( L );

	//Convert Lua to Table
	lt_init( &t, NULL, 1024 );
	lua_stackdump( L );
	if ( !lua_to_table( L, 1, &t ) )
		err( 5, "Failed to convert lua_stack values to table...\n" );

	//Show the table after conversion from Lua
	lt_dump( &t );

	//Rewind pointer
	ll = ld;

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
				
			if (( br = read( fd, ren, sizeof( ren ) - 1 )) == -1 )
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
	write( 2, bf_data( &bc ), bf_written( &bc ) );
#endif

	return 0;
}
