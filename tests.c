/*
tests.c
-------------

There are a lot of tests that will ship with hypno.
Right now, all I really need to do is beat on the URLs.
There will be more later...

- router
- maybe filename scopes 



*/
#include "bridge.h"

typedef struct Test {
	const char *method  ,
						 *url     ;
} Test; 



Test routes[] = {
	{ "GET"    , "/falafel/cd/effeg" },    // NO 
	{ "POST"   , "/spaghetti/cd/effeg" },    // NO 
	{ "PUT"    , "/ab/cd/effeg" },    // NO 
	{ "OPTIONS", "/ab/cd/effeg" },    // NO 
	{ "TRACE"  , "/ab/cd/effeg" },    // NO 
	{ "HEAD"   , "/ab/cd/effeg" },    // NO 
	{ NULL     , NULL           },    // END
};

//sigh... this is so silly...
typedef enum {
	CC_NONE, 
	CC_MODEL, 
	CC_VIEW, 
	CC_FUNCT
} CCtype ;

//this is pretty fucking silly too
struct Run {
	char  *filetype;
	CCtype type;
} Run[] = {
	{ "model",  CC_MODEL }, 
	{ "view" ,  CC_VIEW  }, 
	{ NULL   ,  CC_NONE  }
};


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

	//Load the file
	if ( !lua_load_file( L, "a.lua", err ) )
		return err( 1, "everything is not working...\n" );

	if ( !lt_init( &t, NULL, 127 ) )
		return err( 2, "table did not initialize...\n" );
			
	if ( !lua_to_table( L, 2, &t ) )
		return err( 3, "could not convert Lua table ...\n" );
	
	lt_dump( &t );


	//Loop through each Test
	Test *tt = routes;

	//Need some kind of data type...
	struct DChar {
		char *m, *v;
		char models[ 10 ][ 100 ];
		char views[ 10 ][ 100 ];
	};

	struct loader {
		int   type;      //model or view or something else...	
		char *filename;  //Most of the time it's a file, but it really should execute...
		void *funct;     //This can be used for userdata (which are just Lua functions here)
	};

	//Watch this for obvious reasons...
	struct loader Loader[10];
	memset( Loader, 0, sizeof( Loader ));
	struct loader *LL = Loader;

	while ( tt->method ) 
	{
		//Break the url down by searching forwards for '/'
		Mem a;
		int b = 0;
		char us[ 512 ];

		while ( memwalk( &a, (uint8_t *)tt->url, (uint8_t *)"/", strlen( tt->url ), 1 )  )
		{
			//Skip no matches
			if ( !a.size )
				continue;

			//Check for ? and & (query string stuff) (won't be in the real thing)
			
			//Recycle the same buffer for each "merge" of the URL
			memcpy( &us[ b ], &tt->url[ a.pos ], a.size );
			b += a.size;
			us[ b ] = '\0';
			b += 1;

			//Check each URL to see if it's a route match
			int yh = lt_get_long_i( &t, (uint8_t *)us, strlen( us ) ); 
			if ( yh == -1 ) 
				printf( "val not found '%s'...\n", us );
			else 
			{
				//Get the type of the element that's there
				int type = lt_rettype( &t, 1, yh );

				//Depending on type, the router action will change
				if ( type == LITE_NON || type == LITE_TRM || type == LITE_INT || type == LITE_FLT || type == LITE_NUL )
					err( 0, "type not accepted...\n" );
				else if ( type == LITE_TXT || type == LITE_BLB ) 
					; // text or not
				else if ( type == LITE_USR )
					; //A Lua function most likely...
				else if ( type == LITE_TBL ) 
				{
					//Check that the table is not empty, this is an error and should be rejected
					
					//Check for 'expects', 'model(s)' and 'view(s)'
					for ( int i = 0; i < 2; i++ ) 
					{
						//Check for this hash
						char *locate = strcmbd( ".", us, Run[i].filetype );
						int m = lt_get_long_i( &t, (uint8_t *)locate, strlen( locate ));
						printf( "Hash of '%s' is %d\n", locate, m );

						//Error out if hashes aren't there (for now)
						if ( m == -1 )
							printf( "Key not found: %s\n", locate );
						else 
						{
							//Set the type of element here for later
							LL->type = Run[i].type;	
							int mtype = lt_rettype( &t, 1, m  );

							//Then check the type and do something
							if ( /*mtype != LITE_USR &&*/ mtype != LITE_TXT && mtype != LITE_TBL )
 								printf( "stupid wrong type...\n" ); //Throw an error if this is the case
							else if ( mtype == LITE_USR )
								; //LL->funct = lt_userdata_at( &t, m ); 
							else if ( mtype == LITE_TXT )
								LL->filename = lt_text_at( &t, m ), LL++;
							else if ( mtype == LITE_TBL ) 
							{
								//printf( "%s: {}\n", s[i] );
								//There is no looping feature yet, increase m by 1 until you hit a terminator
								while ( lt_rettype( &t, 0, ++m ) != LITE_TRM )
								{
									//lt_rettype( &t, 0, m ) should NEVER be anything but a number, stop if not
									//Print key and value
									int mt = lt_rettype( &t, 1, m );
									printf( "type of value: %s\n", lt_typename( mt ) );
									if ( mt != LITE_TXT )
										printf( "%s\n", "Value is not a string, fuck you and die :)" );
									else {
										printf( "%s: '%s'\n", Run[i].filetype, lt_text_at( &t, m ) );	
										LL->filename = lt_text_at( &t, m );	
									}
									LL++;
								}
							}
						}
					}
				}

				//Add to the string
				us[ b - 1 ] = '.';	
				us[ b ] = '\0';
				printf( "url: '%s'\n", us );
			}
		
			//This test is done.  I want to see the structure before moving on.
			struct loader *FU = &LL[0];
			while ( FU->type ) {
				printf( "%d -> %s\n", FU->type, FU->filename );
				FU++;	
			}		
			
			fprintf( stderr, "%s\n", tt->method );	
		}
		printf( "Next tt->method\n" );
		tt++;
		getchar();
	} // end while( tt->method )
	return 0;
}
