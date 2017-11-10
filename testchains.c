/* ---------------------------------------------------
hypno-chain.c 

Hypno's power is based off of a chain of data, this 
just runs the chain/evaluator part. 

Technically, the thing can keep running things in 
whatever order.  But CC_VIEW should always be the 
last thing evaluated.

Rendering is VERY easy if all of the data is put into
Lua.  The data is converted to table and rendering is
done.

HOWEVER, 
- Rendering needs to be battle tested.
(Try for at least 50 and make sure that things work)

- BIG ASS TABLES need to be tested.
(You may have up 1000 keys., Also test this)

 * --------------------------------------------------- */
#include "vendor/single.h"
#include "vendor/http.h"
#include "bridge.h"


Loader l[] = {
	//The entire path should be resolved.  I don't see a reason to do it twice.
	//Additional, you can check it seperately and make sure that all files are available

	//Sadly though, this is slightly more complicated than it should be...
#if 0
	{ CC_MODEL, "example/non/app/cycle-a.lua" },
	{ CC_MODEL, "example/non/app/cycle-b.lua" },
#else
	{ CC_MODEL, "cycle-a" },
	{ CC_MODEL, "cycle-b" },
#endif
	{ 0, NULL }
};


// Resolving directories can happen first
void show_chain ( Loader *ld )
{
	Loader *ll = ld;	
	while ( ll->type ) 
	{
		fprintf( stderr, "%s: %s\n", printCCtype( ll->type ), ll->content );
		ll++;	
	}
}


// Prepare the chain
Loader *prepare_chain ( Loader *ld, const char *reldir )
{
	//Loop through, create a full path and check if those files exist
	
	//The engine would return NULL and probably throw a 404 if the file didn't exist
	//This could also be a server error, since technically the symbolic file exists, but
	//the workings underneath do not.

	const char *exts[] = { "lua", "tpl" };
	const char *dirs[] = { "app", "views" };
	Loader *ll = ld;	

	while ( ll->type ) 
	{
		int in = ( ll->type == CC_MODEL ) ? 0 : 1;
		char *tmp = strcmbd( "/", reldir, dirs[ in ], ll->content, exts[ in ]);
		tmp[ strlen( tmp ) - 4 ] = '.';
		fprintf( stderr, "dir: %s\n", tmp );

		//free( ll->content );
		ll->content = tmp;
		ll++;
	}

	return ll;
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
	{ "-f", "--file",      "Run this Lua file.",'s' },
	{ .sentinel = 1 }
};


int main (int argc, char *argv[])
{
	//Values
	if (argc > 2) 
		opt_eval(opts, argc, argv);

	//...
	Buffer bc;
	char *file = ( opt_set( opts, "--file" ) ) ? opt_get( opts, "--file" ).s : "d.lua";
	char err[ 2048 ] = { 0 };
	lua_State *L = luaL_newstate();

	//
	if ( !prepare_chain( l, "example/non" ) )
		return err( 2, "Failed to prepare chain..." );	

	if ( 1 )
		show_chain( l );

#if 1
	char *e[] = { 
	#if 0
		"f.lua", "g.lua", "h.lua", "d.lua" 
	#else
		"f.lua", "d.lua", "g.lua", "e.lua", "i.lua"
	#endif
	};	

	//Load all models here for now...
	for ( int i=0; i < sizeof(e)/sizeof(char *); i++ ) 
	{
		char *f = strcmbd( "/", "tests", e[ i ]);
		if ( !lua_load_file( L, f, err ) )
			return err( 1, "Failed to load file %s\n%s", file, err );
		free( f );
	}

	//Dump stack just for my knowledge
	lua_stackdump( L );
	//lua_stackclear( L );

	//Aggregate...
	//lua_aggregate( L ) 
	//lua_to_table( L, 0, &t );
#else

	//Run each model ( run_models )
	while ( lt->type )
	{
		if ( lt->type == CC_MODEL )
		{
			fprintf( stderr, "%s\n", lt->content );	

			//Successful calls will put results on the stack. 
			if ( !lua_load_file( L, lt->content, err ) ) 
			{
				//Unsuccessful will return NULL, but the stack has the error
				//The lua_load_file function SHOULD modify the error message
				return NULL;
			}
		}
		lt++; //next file
	}

	//Aggregate what's on the stack
	rewind( lt );
	lua_aggregate( L, "name-of-table" );
	lua_to_table( L, 0, &t );

	//Render each view ( run_views )
	while ( lt->type ) 
	{
		if ( lt->type == CC_VIEW ) 
		{
			Render R;
			Table t; 
			int fd = 0, br = 0;
			char *ren = NULL;
			struct stat sb;
		
			//Message
			fprintf( stderr, "Rendering view file %s\n", lt->content );

			//Check for the file and get its size.
			if ( stat( lt->content, &sb ) == -1 )
				return err( 1, "Failed to get file and all..." );

			//Make some space for the file's contents
			if ( !(ren = malloc( sb.st_size + 1 )) )
				return err( 1, "Failed to allocate size for file dump..." );

			//Then open up the file 
			if (( fd = open( lt->content, O_RDONLY ) ) == -1 )
				return err( 1, "Failed to open file to render, are your permissions OK?" );
				
			//And load it for the rendering engine	
			if (( br = read( fd, ren, sizeof( ren ) - 1 )) == -1 )
				{ fprintf( stderr, "loading file '%s' failed...\n", v); goto cleanit ; }

			//Prepare the rendering engine
			if ( !render_init( &R, &t ) )
				{ fprintf( stderr, "render_init failed...\n"); goto cleanit ; }

		#if 0
			//Dump the block (for testing)
			if ( 1 )
				write( 2, ren, br );
		#endif
			
			//"Score" the block to render
			if ( !render_map( &R, (uint8_t *)ren, br ) )
				{ fprintf( stderr, "render mapping failed...\n"); goto cleanit ; }

			//Start rendering
			if ( !render_render( &R ) )
				{ fprintf( stderr, "render_init failed...\n"); goto cleanit ; }
			
		#if 0
			//Send results to stdout or somewhere else...
			if ( 1 )
				write( 2, bf_data( render_rendered( &R ) ), bf_written( render_rendered( &R )) );
			else 
		#endif
			{
				bf_append( HTTP_BUF, bf_data( render_rendered( &R ) ), bf_written( render_rendered( &R )) );
			}

			//Clean up
cleanUp:
			render_free( &R );
			lt_free( &t );
			free( vf );
			close(fd); 
			fd = 0; 
		}

		lt++; //next file
	}

 #if 0	
	//A buffer would typically be initialized here.
	if ( !bf_init( &bc, NULL, 1 ) )
		return err( 2, "Buffer failed to initialize." );

	//Then run the chain.
	if ( !run_chain( l, &bc, L, err ) )
		return err( 1, "Chain running failed on block: %s", err );
 #endif
#endif

	return 0;
}
