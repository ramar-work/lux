/* ---------------------------------- * 
tests.c
-------------

There are a lot of tests that will ship 
with hypno.  Right now, all I really need 
to do is beat on the URLs.  There will be
more later...

- router
- maybe filename scopes 
------------------------------------- */
#include "bridge.h"


//Types of items
typedef struct
{
	const 
	char   *method  ,
			   *url     ;
	int    estatus ;
	Loader result[10];
} Test;


/*
Below is a list of tests organized using the Test 
structure above.  Each test specifies: 
	- a method (for testing the 'expects' key)
	- a URL (to simulate receiving a URL from the user)
	- a status (the expected status according to what I've received)
	- a result table (when the router is done, a data structure is 
		returned to describe how the payload is assembled)
 */
Test routes[] = 
{
	//TEST 1
	{ "GET"    , "/falafel" , 400, {
		{ CC_MODEL, "falafel", NULL },
	}
	},    // NO 

	//TEST 2
	{ "GET"    , "/spaghetti/sauce" , 200, {
		{ CC_MODEL, "spaghetti", NULL },
		{ CC_STR  , "sauce is too sweet!", NULL },
	}
	},    // NO 

#if 0
	{ "GET"    , "/spaghetti/sauce"  , 200  },    // NO 
	{ "POST"   , "/spaghetti/cd/effeg" },    // NO 
	{ "PUT"    , "/ab/cd/effeg" },    // NO 
	{ "OPTIONS", "/ab/cd/effeg" },    // NO 
	{ "TRACE"  , "/ab/cd/effeg" },    // NO 
	{ "HEAD"   , "/ab/cd/effeg" },    // NO 
#endif
	{ NULL     , NULL           },    // END
};






//...


//...


//ERR_TABLE_IS_EMPTY - A route has a table as a value, but no nothing (this could be an implied rule)
//Could also just put nothing on the other side... although a blank table makes more sense... 
int main ( int argc, char *argv[] )
{
	Table t;
	char err[ 2048 ];
	lua_State *L = luaL_newstate(); 

	//Check that Lua initialized here
	if ( !L )
		return 0;
	else {
		luaL_openlibs( L );
		lua_newtable( L );
	}

	//Load the file with Lua and convert results to Table
	if ( !lua_load_file( L, "a.lua", err ) )
		return err( 1, "everything is not working...\n" );

	if ( !lt_init( &t, NULL, 666 ) )
		return err( 2, "table did not initialize...\n" );
			
	if ( !lua_to_table( L, 2, &t ) )
		return err( 3, "could not convert Lua table ...\n" );

	//Dump the file	
	lt_dump( &t );

	//This is here.
	static struct Run {
		char  *filetype;
		CCtype type;
	} Run[] = {
		{ "model",  CC_MODEL }, 
		{ "view" ,  CC_VIEW  }, 
		{ "string" ,  CC_STR }, 
		{ "function" ,  CC_FUNCT }, 
	};

	//Loop through each Test
	Test *tt = routes;
	while ( tt->method ) 
	{
		//Define a bunch of crap and print stuff to get the URL breakdown ready
		Mem a;
		struct Run *r = Run;		
		memset( &a, 0, sizeof(Mem));
		int b = 0;
		char us[ 512 ];
		memset( us, 0, sizeof(us) );
		printf( "%s\n=========================\n", tt->url ); 

		//Initialize the loader structure that the router should return 
		Loader loader[ 10 ];
		memset( loader, 0, sizeof( Loader ) * 10 );
		Loader *LL = loader;

	#if 1
		//Check for ? and & (query string stuff) and adjust string length 
		int len = strlen( tt->url );
		if ( memchrat(tt->url, '?', strlen(tt->url)) || memchrat(tt->url, '&', strlen(tt->url)) )
		{
			int ma = memchrat(tt->url, '?', strlen(tt->url));
			int na = memchrat(tt->url, '&', strlen(tt->url));
			len -= ( na != -1 ) ? na : 0;
			len -= ( ma != -1 ) ? ma : 0;
		}
	#endif

		//Break the url down by searching forwards for '/'
		while ( memwalk( &a, (uint8_t *)tt->url, (uint8_t *)"/", len, 1 ) )
		{
			//Skip no matches
			if ( !a.size ) continue;

			//Recycle the same buffer for each "merge" of the URL
			memcpy( &us[ b ], &tt->url[ a.pos ], a.size );
			b += a.size;
			us[ b ] = '\0';
			b += 1;
			//printf( "new user url: %s\n", us );

			//Check the hash and tell me what it is...
			int yh = lt_get_long_i( &t, (uint8_t *)us, strlen( us ) ); 

			//Check each URL to see if it's a route match
			if ( yh == -1 ) 
				printf( "val not found '%s'...\n", us );
			else 
			{
				//Get the type of the element that's there
				int type = lt_rettype( &t, 1, yh );
				//Depending on type, the router action will change
				if ( type == LITE_NON || type == LITE_TRM || type == LITE_INT || type == LITE_FLT || type == LITE_NUL )
					err( 0, "type not accepted...\n" );
				else if ( type == LITE_USR )
				{
					//A Lua function most likely...not ready yet
					err( 0, "function type not accepted (yet)...\n" );
				}
				else if ( type == LITE_BLB ) 
				{
					//Binary data..., unsure how this will work now
					err( 0, "binary type not accepted (yet)...\n" );
				}
				else if ( type == LITE_TXT ) 
				{
					LL->type = CC_STR; 
					LL->content = lt_text_at( &t, yh ); 
					LL++;	
				}
				else if ( type == LITE_TBL ) 
				{
					//Check for 'expects'
					//{...}

					//Check for 'model(s)' and 'view(s)'
					char *fname[2] = { "model", "view" };
					int ctype[2] = { CC_MODEL, CC_VIEW };
					int m = -1;
	
					for ( int ii=0; ii < 2; ii++ ) 
					{
						char *locate = strcmbd( ".", us, fname[ ii ] );
						if ((m = lt_get_long_i( &t, (uint8_t *)locate, strlen( locate ))) > -1 )
						{
							int mtype = lt_rettype( &t, 1, m  );
							//Then check the type and do something
							if ( mtype != LITE_TXT && mtype != LITE_TBL /* && mtype != LITE_USR */ )
							{
								//LL->funct = lt_userdata_at( &t, m ); 
 								printf( "stupid wrong type...\n" ); //Throw an error if this is the case
							}
							else if ( mtype == LITE_TXT ) 
							{
								LL->content = lt_text_at( &t, m ); 
								LL->type = ctype[ ii ];
								LL++;	
							}
							else if ( mtype == LITE_TBL ) 
							{
								//Increment current table index until you hit a terminator
								while ( lt_rettype( &t, 0, ++m ) != LITE_TRM ) 
								{
									//lt_rettype( &t, 0, m ) should NEVER be anything but a number, stop if not
									int kt = lt_rettype( &t, 0, m ), vt = lt_rettype( &t, 1, m );
									if ( vt != LITE_TXT )
										printf( "%s\n", "Value not yet supported..." );
									else 
									{
										LL->content = lt_text_at( &t, m );	
										LL->type = ctype[ ii ];
										LL++;
									}
								} // while ( lt_rettype( ... ) != LITE_TRM )
							} // ( mtype == LITE_TBL )
						} // ( m == -1 )
						free( locate );
					} // for ( ;; )
				}	// else ( type ==	LITE_NON )
			} // else if ( yh == -1 )

			//Add to the string for the next iteration
			us[ b - 1 ] = '.';	
			us[ b ] = '\0';
			printf( "next iteration of URL: '%s'\n", us );
		}

		//This test is done.  I want to see the structure before moving on.
		printf( "Execution path for route: %s\n", tt->url ); 
		Loader *Ld = &loader[0];
		while ( Ld->type ) {
			printf( "%s -> %s\n", printCCtype( Ld->type ), Ld->content );
			Ld++;
		}

		//Move to the next test		
		//fprintf( stderr, "%s\n\n", tt->method );	
		tt++;
	} // end while( tt->method )

	lt_free( &t );
	return 0;
}
// vim: tabstop=2 number
