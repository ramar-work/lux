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
};

char *CCtypes[] = {
	[CC_NONE]  = "None",
	[CC_MODEL] = "Model",
	[CC_VIEW]  = "View",
	[CC_FUNCT] = "Function",
};

//...
char *printCCtype ( CCtype cc )
{
	switch (cc)
	{
		case CC_NONE:
			return CCtypes[ cc ];
		case CC_MODEL: 
			return CCtypes[ cc ];
		case CC_VIEW: 
			return CCtypes[ cc ];
		case CC_FUNCT:
			return CCtypes[ cc ];
		default:
			return NULL;
	}
}


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

	//Dump the file	
	lt_dump( &t );

	//Loop through each Test
	Test *tt = routes;

	//
	struct loader {
		int   type;      //model or view or something else...	
		char *filename;  //Most of the time it's a file, but it really should execute...
		void *funct;     //This can be used for userdata (which are just Lua functions here)
	} Loader[ 10 ];

	//Watch this for obvious reasons...
	memset( Loader, 0, sizeof( Loader ));

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
		struct loader *LL = Loader;

		//Break the url down by searching forwards for '/'
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
			printf( "new user url: %s\n", us );

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
				else if ( type == LITE_USR )
					err( 0, "type not accepted...\n" );//A Lua function most likely...not ready yet
				else if ( type == LITE_TXT || type == LITE_BLB ) 
					;//LL->type = Run[ii].type, LL++;	
				else if ( type == LITE_TBL ) {
					//Check for 'expects', 'model(s)' and 'view(s)'
					for ( int ii=0; ii<sizeof(Run)/sizeof(struct Run); ii++ ) {
						char *filetype = Run[ ii ].filetype;
						char *locate   = strcmbd( ".", us, filetype );
						int m    = lt_get_long_i( &t, (uint8_t *)locate, strlen( locate ));
						LL->type = Run[ii].type;	
						//printf( "filetype: %s\n", filetype );printf( "Hash of '%s' is %d\n", locate, m );

						//Error out if hashes aren't there (for now)
						if ( m == -1 )
							printf( "Key not found: %s\n", locate );
						else {
							int mtype = lt_rettype( &t, 1, m  );
							//Then check the type and do something
							if ( mtype != LITE_TXT && mtype != LITE_TBL /* && mtype != LITE_USR */ )
								//LL->funct = lt_userdata_at( &t, m ); 
 								printf( "stupid wrong type...\n" ); //Throw an error if this is the case
							else if ( mtype == LITE_TXT )
								LL->filename = lt_text_at( &t, m ), LL++;
							else if ( mtype == LITE_TBL ) {
int dh=1;
								//There is no looping feature yet, increase m by 1 until you hit a terminator
								while ( lt_rettype( &t, 0, ++m ) != LITE_TRM ) {
									//lt_rettype( &t, 0, m ) should NEVER be anything but a number, stop if not
									int kt = lt_rettype( &t, 0, m ), vt = lt_rettype( &t, 1, m );
									if ( vt != LITE_TXT )
										printf( "%s\n", "Value is not a string, fuck you and die :)" );
									else {
										printf( "match %d?\n", dh++ );
										LL->filename = lt_text_at( &t, m );	
										LL++;
									}
								} // while ( lt_rettype( ... ) != LITE_TRM )
							} // ( mtype == LITE_TBL )
						} // ( m == -1 )
					} // for ( ;; )
				}	// else ( type ==	LITE_NON )
			} // else if ( yh == -1 )

			//Add to the string for the next iteration
			us[ b - 1 ] = '.';	
			us[ b ] = '\0';
			printf( "url: '%s'\n", us );
		}

#if 1
		//This test is done.  I want to see the structure before moving on.
		//struct loader *FU = &LL[0];
		struct loader *FU = &Loader[0];
		while ( FU->type ) {
			printf( "%s -> %s\n", printCCtype( FU->type ), FU->filename );
			FU++;
		}
#endif

		//Move to the next test		
		getchar();
		fprintf( stderr, "%s\n\n", tt->method );	
		tt++;
	} // end while( tt->method )
	return 0;
}
