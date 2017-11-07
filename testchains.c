/*
//hypno-chain.c 
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

*/
#include "vendor/single.h"
#include "vendor/http.h"
#include "bridge.h"


Loader l[] = {
	//The entire path should be resolved.  I don't see a reason to do it twice.
	//Additional, you can check it seperately and make sure that all files are available

	//Sadly though, this is slightly more complicated than it should be...
	{ CC_MODEL, "example/non/app/cycle-a.lua" },
	{ CC_MODEL, "example/non/app/cycle-b.lua" },
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
	char err[ 2048 ] = { 0 };
	lua_State *L = luaL_newstate();

#if 1
	//An extremely fast dump for the purposes of
	int in = 0;
	int sd = 0;
	lua_load_file( L, "c.lua", 0 );
	lua_stackdump( L, &in, &sd );
#endif

	//A buffer would typically be initialized here.
	if ( !bf_init( &bc, NULL, 1 ) )
		return err( 2, "Buffer failed to initialize." );

	//Then run the chain.
	if ( !run_chain( l, &bc, L, err ) )
		return err( 1, "Chain running failed on block: %s", err );

	return 0;
}
