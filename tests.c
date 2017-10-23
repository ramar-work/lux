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
	{ "GET"    , "/ab/cd/effeg" },    // NO 
	{ "POST"   , "/ab/cd/effeg" },    // NO 
	{ "PUT"    , "/ab/cd/effeg" },    // NO 
	{ "OPTIONS", "/ab/cd/effeg" },    // NO 
	{ "TRACE"  , "/ab/cd/effeg" },    // NO 
	{ "HEAD"   , "/ab/cd/effeg" },    // NO 
	{ NULL     , NULL           },    // END
};



//To simulate a real structure 
char **url_breaker( const char *url, char **urlroutes )
{

	return NULL; 
}



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
	while ( tt->method ) 
	{
/*
i must be extremely tired... 

always start at the source, then go to the next move...

 */

		//Break the url down by searching forwards for /
		//parser lib can be used for this...

#if 0
		//Send it on...
		if ( !route_controller( tt->url, &t  ) )
		{


		}
#endif
	
		fprintf( stderr, "%s\n", tt->method );	
		tt++;
	}
	return 0;
}
