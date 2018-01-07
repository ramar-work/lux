/*
* agg.c 
* =============
*
* Tests Lua table aggregation.
*
* Check that the lua_stackdump function works like it should.  Most of the
* implementations that are easily found online don't really show as much
* information as I would like.
*
* To really get this job done, this test randomly generates values and 
* throws them into tables.  The stak will ALWAYS be wiped at the end of
* each aggregation.
*
*/

#include "../vendor/single.h"
#include "../vendor/http.h"
#include "../bridge.h"

#if 0
#define MIST(a)
#else
#define MIST(a) fprintf( stderr, "%s:%d => ", __FILE__, __LINE__ ); lua_stackdump(a)
#endif

#define SD(...) \
	fprintf( stderr, "%s:%d ", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ ); \
	lua_stackdump( L ); \
	getchar()

static int depth = 0;
static int currentIndex = 1;
static const char *ltype[] = { "table", "number", "string" };
const char table_is_full[] = "Table is full.";
const char wouldHave[] = "Would have added a table here.";

int _int ( lua_State *L )
{
	lua_pushnumber( L, atoi( rand_numbers( 4 ) ) );
	return 0;
}


char *_char ( lua_State *L )
{
	char *mal = malloc( 256 );	
	memset( mal, 0, 256 );
	memcpy( mal, rand_chars( 255 ), 254 ); 	
	lua_pushstring( L, mal );
	return mal;
}


int _table ( lua_State *L, char **tc, int *tcIndex )
{
	//Only populate if the current depth is less than 5
	if ( depth > 4 ) {
SD( "Stack too high: adding nothing...\n" );
		return 0;
	}
	else
	{
		//Generate some random number as the index (these won't be bigger than 10)
		int tmax = atoi( rand_numbers( 1 ) ) * 2;
		int ttype = 0;

		//Get index of newest table on stack
		lua_newtable( L );
		int ind = lua_gettop( L );

SD( "Stack after adding new empty table via _table( ... )...\n"
		"This new table will have %d values.\n", tmax / 2 );
#if 1	
		//Now I'm one level deeper	
		depth++;

		//Should probably double tmax so that a symmetrical amount of things are on the table
		while ( tmax-- ) 
		{
			fprintf( stderr, "Current value of tmax: %d\n", tmax );
			if (( ttype = atoi( rand_numbers( 2 ) ) % 3 ) == 0 && tmax % 2 == 0 )
			{
SD( "Adding level %d table to stack.\n", depth );
				lua_newtable(L); 
SD( "Done with level %d table.\n", depth );
			}
			else if ( ttype == 1 ) 
			{
				int a = atoi( rand_numbers( 4 ) );
				lua_pushnumber( L, a );
SD( "Added number %d to stack.\n", a );
			}
			else if ( ttype == 2 ) {
				char *freeMe = rand_chars( 64 );
				lua_pushstring( L, freeMe );
SD( "Added string %s to stack.\n", freeMe );
			}
			else {
				if (atoi(rand_numbers( 1 )) % 2) 
					lua_pushnumber( L, atoi( rand_numbers( 3 )) );
				else {
					char *freeMe = rand_chars( 64 );
					lua_pushstring( L, freeMe );
				}
SD( "Couldn't add table as key, so added another random value.\n" );
			}
			
			if ( tmax % 2 == 0 ) {
SD( "Adding last two values to table at %d.\n", ind );
				lua_settable( L, ind );
SD( "Inspect that values were added to table correctly..." );
			}
		}
	
		depth--;		
#endif
	}

	return 1;	
}




int main (int argc, char *argv[])
{
	//Create a stack last and make sure it has enough space for some intense testing
	lua_State *L = luaL_newstate();
	
#if 0
	for ( int n=0; n<10; n++ )
	{
		//Define values that the test can reuse later.
		int testInt = 0;
		char *testChars[100] = {0};
		int testCharIndex = 0;
		Table testTable = {0};

		//Define how many entries should be in the "root" table
		int rootTableCount = atoi( rand_numbers( 2 ) ); 
		rootTableCount >>= 2;

		//Repeat if the value is 0
		if ( rootTableCount > 0 )
			fprintf( stderr, "Generating a table with %d entries.\n", rootTableCount );
		else
		{
			n--;
			continue;
		}

		//Looping
		while ( rootTableCount-- ) 
		{
			//Define how many entries should be in the "root" table
			int ltypeInt = atoi( rand_numbers( 2 ) ) % 3;
			//fprintf( stderr, "Will generate index %d with value %s.\n", rootTableCount, ltype[ ltypeInt % 3 ] );

			//Add the appropriate type
			if ( ltypeInt == 0 )
				_table( L, testChars, &testCharIndex );
			else if ( ltypeInt == 1 )
				_int  ( L );
			else if ( ltypeInt == 2 ) 
			{
				testChars[ testCharIndex++ ] = _char ( L );//, &testChars[ testCharIndex ] );
			}
		}

		//There is an absolutely massive table sitting on the stack now.  Dump it.
		lua_stackdump( L );

		//Then put everything in one via lua_agg.
		
		//Clean up
		while ( testCharIndex-- ) {
			free( testChars[ testCharIndex ] );
		}
	}

#else
	//Clear the stack
	fprintf( stderr, "Clearing the stack...\n" );
	lua_settop( L, 0 );
	MIST( L );

	//Add a bunch of random garbage that's not a table and see how it works...
	fprintf( stderr, "Adding test rows...\n" );
	lua_pushstring( L, "weedeating" );
	lua_pushnumber( L, 1321231 );
	lua_pushstring( L, "michael jackson" );
	lua_pushstring( L, "roblox and come" );
	lua_pushnumber( L, 12213 );
	MIST( L );

	//Add a new table and add three key-value pairs to it
	fprintf( stderr, "Adding new table containing three key-value pairs.\n" );
	lua_newtable( L ); 
	lua_pushstring( L, "singer" );
	lua_pushstring( L, "bon jovi" );
	lua_settable( L, 6 );	
	lua_pushstring( L, "color" );
	lua_pushstring( L, "blue" );
	lua_settable( L, 6 );	
	lua_pushinteger( L, 77 );
	lua_pushstring( L, "randomly high index" );
	lua_settable( L, 6 );
	MIST( L );

	//Nested table
	fprintf( stderr, "Adding new table containing two key-value pairs and one numeric key-value pair.\n" );
	lua_pushstring( L, "jazzy" ); //The new table will have this as a key name
	lua_newtable( L ); // 8
	MIST( L );
	
	lua_pushstring( L, "singer" );
	lua_pushstring( L, "bruce springsteen" );
	lua_settable( L, 8 );	
	lua_pushstring( L, "color" );
	lua_pushstring( L, "orange" );
	lua_settable( L, 8 );	
	lua_pushinteger( L, 999 );
	lua_pushstring( L, "randomly high index" );
	lua_settable( L, 8 );	
	MIST( L );

	//again
	fprintf( stderr, "Moving newly created table to table at index 6.\n" );
	lua_settable( L, 6 );	
	MIST( L );
	MIST( L );
	MIST( L );
	MIST( L );
	MIST( L );

	//A regular string to round things off
	fprintf( stderr, "Adding a regular string to table to test switching value types.\n" );

	lua_pushstring( L, "You workin' again, John?" );
	MIST( L );

	lua_pushstring( L, "Possibly..." );
	MIST( L );

	//...
	fprintf( stderr, "Population is completed." );
#endif

#if 0
	//Add some other stuff
	char err[2048] = {0};
	const char *ff[] = { "tests/agg-data/ad1.test", "tests/agg-data/ad2.test", "tests/agg-data/ad3.test", "tests/agg-data/ad4.test", NULL };

	for ( int f=0; f < sizeof( ff ) / sizeof ( char * ); f++ )	
		if ( !lua_load_file( L, *ff, err ) ) err( 3, "Could not load file %s\n", *ff );

	//Should add a table and tell Lua where I want the values explicitly.	
#endif
	lua_aggregate( L );
	return 0;
}
