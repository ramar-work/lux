#include "bridge.h"

char *CCtypes[] = {
	[CC_NONE]  = "None",
	[CC_MODEL] = "Model",     //Models are usually executed, and added to env 
	[CC_VIEW]  = "View",      //Views will be loaded, parsed, and sent to buffer
	[CC_FUNCT] = "Function",  //Functions are executed, payload sent to buffer
	[CC_STR]   = "String",    //Strings are just referenced and loaded to buffer
	[CC_MAX]   = NULL,        //A nul terminator
};




//Loop through a table in memory
void lua_loop ( lua_State *L )
{
	int a = lua_gettop( L );
	obprintf( stderr, "Looping through %d values.\n", a );

	for ( int i=1; i <= a; i++ )
	{
		obprintf( stderr, "%-2d: %s\n", i, lua_typename( L, lua_type( L, i )) );
		//lua_getindex(L, i );
	}
}




//Print CC type
char *printCCtype ( CCtype cc )
{
	switch (cc)
	{
		case CC_NONE:
		case CC_MODEL: 
		case CC_VIEW: 
		case CC_FUNCT:
		case CC_STR:
			return CCtypes[ cc ];
		default:
			return NULL;
	}
}



//
int lua_load_file( lua_State *L, const char *file, char **err  )
{
	if ( luaL_dofile( L, file ) != 0 )
	{
		fprintf( stderr, "Error occurred!\n" );
		//The entire stack needs to be cleared...
		if ( lua_gettop( L ) > 0 ) {
			fprintf( stderr, "%s\n", lua_tostring(L, 1) );
			return ( snprintf( *err, 1023, "%s\n", lua_tostring( L, 1 ) ) ? 0 : 0 );
		}
	}
	return 1;	
}


int lua_load_file2( lua_State *L, Table *t, const char *file, char *err  )
{
	if ( luaL_dofile( L, file ) != 0 )
	{
		fprintf( stderr, "Error occurred!\n" );
		//The entire stack needs to be cleared...
		if ( lua_gettop( L ) > 0 ) {
			fprintf( stderr, "%s\n", lua_tostring( L, 1 ) );
			snprintf( err, 1023, "%s\n", lua_tostring( L, 1 ) );	
			return 0;	
		}
	}
	return 1;	
}


//Convert Table to Lua table
int table_to_lua (lua_State *L, int index, Table *tt)
{
	int a = index;
	LiteKv *kv = NULL;
	lt_reset( tt );

	while ( (kv = lt_next( tt )) )
	{
		struct { int t; LiteRecord *r; } items[2] = {
			{ kv->key.type  , &kv->key.v    },
			{ kv->value.type, &kv->value.v  } 
		};

		int t=0;
		for ( int i=0; i<2; i++ )
		{
			LiteRecord *r = items[i].r; 
			t = items[i].t;
			obprintf( stderr, "%s\n", ( i ) ? lt_typename( t ) : lt_typename( t ));

			if ( i ) 
			{
				if (t == LITE_NUL)
					;
				else if (t == LITE_USR)
					lua_pushstring( L, "usrdata received" ); //?
				else if (t == LITE_TBL) 
				{
					lua_newtable( L );
					a += 2;
				}
			}
			if (t == LITE_NON)
				break;
			else if (t == LITE_FLT || t == LITE_INT)
				lua_pushnumber( L, r->vint );				
			else if (t == LITE_FLT)
				lua_pushnumber( L, r->vfloat );				
			else if (t == LITE_TXT)
				lua_pushstring( L, r->vchar );
			else if (t == LITE_TRM)
			{
				a -= 2;	
				break;
			}
			else if (t == LITE_BLB) 
			{
				LiteBlob *bb = &r->vblob;
				lua_pushlstring( L, (char *)bb->blob, bb->size );
			}
		}

		( t == LITE_NON || t == LITE_TBL || t == LITE_NUL )	? 0 : lua_settable( L, a );
		lua_loop( L );
	}

	obprintf( stderr, "Done!\n" );
	return 1;
}




//Convert Lua tables to regular tables
int lua_to_table (lua_State *L, int index, Table *t )
{
	static int sd;
	lua_pushnil( L );
	obprintf( stderr, "Current stack depth: %d\n", sd++ );

	//
	while ( lua_next( L, index ) != 0 ) 
	{
		int kt, vt;
		obprintf( stderr, "key, value: " );

		//This should pop both keys...
		obprintf( stderr, "%s, %s\n", lua_typename( L, lua_type(L, -2 )), lua_typename( L, lua_type(L, -1 )));

		//Keys
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			obprintf( stderr, "key: %lld\n", (long long)lua_tointeger( L, -2 ));
		else if ( kt  == LUA_TSTRING )
			obprintf( stderr, "key: %s\n", lua_tostring( L, -2 ));

		//Values
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			obprintf( stderr, "val: %lld\n", (long long)lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			obprintf( stderr, "val: %s\n", lua_tostring( L, -1 ));

		//Get key (remember Lua indices always start at 1.  Hence the minus.
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			lt_addintkey( t, lua_tointeger( L, -2 ) - 1);
		else if ( kt  == LUA_TSTRING )
			lt_addtextkey( t, (char *)lua_tostring( L, -2 ));

		//Get value
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			lt_addintvalue( t, lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			lt_addtextvalue( t, (char *)lua_tostring( L, -1 ));
		else if ( vt == LUA_TTABLE )
		{
			lt_descend( t );
			obprintf( stderr, "Descending because value at %d is table...\n", -1 );
			lua_loop( L );
			lua_to_table( L, index + 2, t ); 
			lt_ascend( t );
			sd--;
		}

		obprintf( stderr, "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING )
		lt_finalize( t );
		lua_pop(L, 1);
	}

	lt_lock( t );
	return 1;
}


void lua_stackclear ( lua_State *L )
{
	int top = lua_gettop( L );
	lua_pop( L, top );
}



void lua_dumptable ( lua_State *L, int *pos, int *sd )
{
	lua_pushnil( L );
	LUA_DUMPSTK( L );
	//fprintf( stderr, "*pos = %d\n", *pos );

	while ( lua_next( L, *pos ) != 0 ) 
	{
		//Fancy printing
		//fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t\t"[ 10 - *sd ] );
		PRETTY_TABS( *sd );
		LUA_DUMPSTK( L );
		fprintf( stderr, "[%3d:%2d] => ", *pos, *sd );

		//Print both left and right side
		for ( int i = -2; i < 0; i++ )
		{
			int t = lua_type( L, i );
			const char *type = lua_typename( L, t );
			if ( t == LUA_TSTRING )
				fprintf( stderr, "(%8s) %s", type, lua_tostring( L, i ));
			else if ( t == LUA_TFUNCTION )
				fprintf( stderr, "(%8s) %p", type, (void *)lua_tocfunction( L, i ) );
			else if ( t == LUA_TNUMBER )
				fprintf( stderr, "(%8s) %lld", type, (long long)lua_tointeger( L, i ));
			else if ( t == LUA_TBOOLEAN)
				fprintf( stderr, "(%8s) %s", type, lua_toboolean( L, i ) ? "true" : "false" );
			else if ( t == LUA_TTHREAD )
				fprintf( stderr, "(%8s) %p", type, lua_tothread( L, i ) );
			else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
				fprintf( stderr, "(%8s) %p", type, lua_touserdata( L, i ) );
			else if ( t == LUA_TNIL ||  t == LUA_TNONE )
				fprintf( stderr, "(%8s) %p", type, lua_topointer( L, i ) );
			else if ( t == LUA_TTABLE ) 
			{
				fprintf( stderr, "(%8s) %p\n", type, lua_topointer( L, i ) );
				(*sd) ++, (*pos) += 2;
				lua_dumptable( L, pos, sd );
				(*sd) --, (*pos) -= 2;
				PRETTY_TABS( *sd );
				fprintf( stderr, "}" );
			}
			fprintf( stderr, "%s", ( i == -2 ) ? " -> " : "\n" );
		}

		lua_pop( L, 1 );
		LUA_DUMPSTK( L );
	}
	return;
}




void lua_stackdump ( lua_State *L )
{
	//No top
	if ( lua_gettop( L ) == 0 )
		return;

	//Loop through all of the values that are on the stack
	LUA_DUMPSTK( L );

	//Loop again, but show the value of each key on the stack
	for ( int pos = 1; pos <= lua_gettop( L ); pos++ ) 
	{
		int t = lua_type( L, pos );
		const char *type = lua_typename( L, t );
		fprintf( stderr, "[%3d] => ", pos );

		if ( t == LUA_TSTRING )
			fprintf( stderr, "(%8s) %s", type, lua_tostring( L, pos ));
		else if ( t == LUA_TFUNCTION )
			fprintf( stderr, "(%8s) %p", type, (void *)lua_tocfunction( L, pos ) );
		else if ( t == LUA_TNUMBER )
			fprintf( stderr, "(%8s) %lld", type, (long long)lua_tointeger( L, pos ));
		else if ( t == LUA_TBOOLEAN)
			fprintf( stderr, "(%8s) %s", type, lua_toboolean( L, pos ) ? "true" : "false" );
		else if ( t == LUA_TTHREAD )
			fprintf( stderr, "(%8s) %p", type, lua_tothread( L, pos ) );
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
			fprintf( stderr, "(%8s) %p", type, lua_touserdata( L, pos ) );
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
		else if ( t == LUA_TTABLE ) 
		{
		#if 0
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
		#else
			fprintf( stderr, "(%8s) %p {\n", type, lua_topointer( L, pos ) );
			int sd = 1;
			LUA_DUMPSTK( L );
			lua_dumptable( L, &pos, &sd );
			fprintf( stderr, "}" );
		#endif
		}	
		fprintf( stderr, "\n" );
	}
	return;
}




//Lua dump....
void lua_tdump (lua_State *L) 
{
	int level = 0;	
	int ct = lua_gettop( L );

	//get the count
	//loop until you die... :D

	//Loop through each index
	for (int i=0; i <= ct; i++) 
	{
	// { a, b, c, d }
	// { 0, 2342, { }, 332 }
	// { a=b, b=c, c=f, d=e }
		printf( "%d\n", i );
		printf( "%s\n", lua_typename( L, lua_type( L, i )) );
#if 0
		LiteType vt = (t->head + i)->value.type;
		fprintf ( stderr, "[%-5d] %s", i, &"\t\t\t\t\t\t\t\t\t\t"[ 10 - level ]);
		lt_printindex( t->head + i, level );
		level += ( vt == LITE_NUL ) ? -1 : (vt == LITE_TBL) ? 1 : 0;
#endif
	}
}



//Parse the current route according to what was in the table.
//Loader *parse_route ( Loader *l, int lsize, Table *httpTable, Table *routeTable )
Loader *parse_route ( Loader *l, int lsize, HTTP *http, Table *routeTable )
{
	//Define and declare
	Loader *LL = l;
	Table *http_r = &http->request.table;
	char us[ 1024 ] = { 0 };
	int szof_us = sizeof( us );
	int b     = 0;
	int count = 0; 

	//Find the index of the URL key in HTTP 
	int ind   = lt_get_long_i( http_r, (uint8_t *)"URL", 3 );
	fprintf( stderr, "URL at: %d\n", ind );

	//This should be checked out of this function
	while ( lt_rettype( http_r, 0, ind++ ) != LITE_TRM )
		count++;

	//Fail if this happens, b/c the URL wasn't done right
	if ( count < 2 )
		return err( NULL, "URL doesn't exist." );

	//Reset count and set index to the top of the URL chain
	ind -= count;	
#if 0
	fprintf( stderr, "indexof lt_get_long_i is at %d\n", ind );	
	fprintf( stderr, "element count is %d\n", count );	
	fprintf( stderr, "ind is %d\n", ind );
#endif

#if 1
	//Now this loops through HTTP structure
	lt_dump( http_r );
	http_print_request( http );
	//lt_dump( routeTable );
#endif


	//Guess this was written at some other time...
	//Each one of these things is just a route... pretty easy for the most part...

	// TODO:
	// '/' always resolves to default, just makes sense...
	// '/[a-zA-Z*]' always resolves to the domain
	// combining once and just changing '/' to '.' should resolve much faster...
	// no support for regexp yet...

	// TODO some more:
	// Combine the URL
	char *fullRoute = NULL;
	fprintf (stderr, "Path: %s\n", http->request.path );
	fprintf (stderr, "Path: %d\n", (int)strlen( http->request.path ));

	//Root domain check...
	if ( (int)strlen( http->request.path ) == 1 && strcmp( http->request.path, "/" ) == 0 ) {
		fprintf (stderr, "Path requested is root.\n" );
		fullRoute = "routes.default";
	}
	//Do other domain-y things...
	else {
		fprintf (stderr, "Path requested is %s.\n", http->request.path );
		fullRoute = strcmbd( "/", "routes", &http->request.path[1] );
		for ( int i=0; i<strlen(fullRoute);i++ ) {
			( fullRoute[i] == '/' ) ? fullRoute[i] = '.' : 0;
		}
	}


	//Now check the hash against the thing and loop through shit...
	char 
		*modelTag = strcmbd( ".", fullRoute, "model" ),
		*viewTag = strcmbd( ".", fullRoute, "view" );

	int 
		mh = lt_get_long_i( routeTable, (uint8_t *)modelTag, strlen(modelTag) ),
		vh = lt_get_long_i( routeTable, (uint8_t *)viewTag, strlen(viewTag) );

#if 0
	fprintf( stderr, "%s\n", modelTag );
	fprintf( stderr, "%s\n", viewTag );

	fprintf( stderr, "mt hash: %d\n", mh );
	fprintf( stderr, "vt hash: %d\n", vh );
#endif

	//Loop through until you get a terminator, means you're at the end...
	for ( int i=mh + 1; ; i++ ) {
		char *lt_char = NULL;
		int lt_type =	lt_valuetypeat( routeTable, i );

		// Stop at nul or term
		if ( lt_type == LITE_NUL || lt_type == LITE_TRM ) {
			break;
		}

		//TODO: If this manages to be null or zero-length, it's probably a bug...
		if ( !(lt_char =	lt_text_at( routeTable, i )) ) {
			break;
		}

		//LL is either accessed wrong or not init'd.
		LL->type = CC_MODEL;
	  LL->content = strdup( lt_char );
	  LL++ ;
	}

	//Loop through until you get a terminator, means you're at the end...
	for ( int i=vh + 1; ; i++ ) {
		char *lt_char = NULL;
		int lt_type =	lt_valuetypeat( routeTable, i );

		// Stop at nul or term
		if ( lt_type == LITE_NUL || lt_type == LITE_TRM ) {
			break;
		}

		//TODO: If this manages to be null or zero-length, it's probably a bug...
		if ( !(lt_char =	lt_text_at( routeTable, i )) ) {
			break;
		}

		//LL is either accessed wrong or not init'd.
		LL->type = CC_VIEW;
	  LL->content = strdup( lt_char );
	  LL++ ;
	}

#if 0
	//Leave this, in case I want to see the structure before moving on.
	printf( "Execution path for route: %s\n", http->url );
	Loader *Ld = &l[0];
	while ( Ld->type ) {
		printf( "%s -> %s\n", printCCtype( Ld->type ), Ld->content );
		Ld++;
	}
#endif
	return l;
}



int load_and_run_files ( Loader *l ) { 
	
	return 1;
}

int counter=0;


//Callback
int lua_callback(void *nu, int argc, char **argv, char **cn) 
{
	lua_State *L = (lua_State *)nu;
	int i;
	lua_pushnumber(L, counter); // 4
	lua_newtable(L); // 5
	for (i=0;i<argc;i++) {
	#ifdef VERBOSE
		printf("Retrieving column '%s' => %s\n", cn[i], argv[i] ? argv[i] : "NULL");
	#endif
		lua_pushstring(L, cn[i]); 
		lua_pushstring(L, argv[i] ? argv[i] : "NULL");
		lua_settable(L, 5);
	}
	lua_settable(L, 3);
	counter++;
	return 0; 
}



//A Lua db insert function with binding...
int lua_db_insert ( lua_State *L )
{ 
	return 0;
}


//This is a Lua function
int lua_db ( lua_State *L )
{
	//...
	int rc;
	int len = 0;
	sqlite3 *db = NULL;
	sqlite3_stmt *stmt = NULL;
	char *errmsg = NULL;
	char errbuf[ 2048 ] = { 0 };
	const char *filename = luaL_checkstring( L, 1 );
	const char *query = luaL_checkstring( L, 2 );
	char *final_query = NULL;
	Database b; 
	memset( &b, 0, sizeof(Database));


	//Trim the query
	final_query = (char *)trim((uint8_t *)query, " \t\n\r", strlen(query), &len ); 

#if 1
	//More details
	fprintf( stderr, "SQL file for query: %s\n", filename );
	fprintf( stderr, "query: '%s'\n", final_query );
#endif

#if 1
	//Using tables (and extra memory)
	if ( !sq_init( &b ) || !sq_open( &b, filename ) ) 
	{
		fprintf( stderr, "Error was: %d, %s\n", __LINE__, "unknown as of yet..." );
		return 0; /*return an error to Lua*/
	}

	//Save the results to table
	if ( !sq_save( &b, final_query, "db", NULL ) )
	{
		fprintf( stderr, "Error was: %d, %s\n", __LINE__, "unknown as of yet..." );
		return 0; /*return an error to Lua*/
	}

 #if 1
	//Dump them
	lt_dump( &b.kvt );
 #endif

	//Clear the stack and add a table
	lua_settop( L, 0 );
	lua_newtable( L );
 #if 0
	//lua_stackdump( L );
 #endif

	//Put results in Lua
	if ( !table_to_lua( L, 1, &b.kvt ) )
	{
		fprintf( stderr, "Failed to put table on stack...\n" );
		return 0;
	}

	//Does this free?
	sq_close( &b );	
	lua_stackdump( L );
#else
	//Traditional way
	if (sqlite3_open(filename, &db) != SQLITE_OK) 
	{
		//return berr(0, ERR_DB_OPEN );
		//snprintf(errbuf, 1023, "Can't open database: %s.", sqlite3_errmsg(db));
		fprintf(stderr, "Can't open database: %s.", sqlite3_errmsg(db));
		sqlite3_close(db);
		//push a string to Lua
		return 0;
		//return err(0, "failed to open db: %s\n", sqlite3_errmsg( db ));
	}

	//There is a lot of code here...
	//Timing tests will tell a lot.
	//I'm also thinking that while Lua is not endless, it MAY handle 
	//really large record sets better than my Table implementation out of the box...

	//Close the connection
	if ( sqlite3_close( db ) != SQLITE_OK )
	{
		return 0;
	}
#endif


	//Put things on the stack and return
	return 1;
}




int lua_writetable( lua_State *L, int *pos, int ti )
{
	lua_pushnil( L );
	while ( lua_next( L, *pos ) != 0 ) 
	{
		//Set the current index
		int t = lua_type( L, -2 );

	#if 1
		//Copy the index and rotate the top and bottom
		lua_pushnil( L );
		lua_copy( L, -3, lua_gettop( L ) );
		lua_rotate( L, -2, -1 );
	#else
		//There must be a way to do this that will run on older Luas
	#endif

		//Now, setting the table works about the same as popping
		lua_settable( L, ti );	
		lua_stackdump( L );
	}
	return 1;
}



int lua_aggregate (lua_State *L)
{
#if 1
	//Stack Test
	//==========
	//Check that the lua_stackdump function works like it should.  Most of the
	//implementations that are easily found online don't really show as much
	//information as I would like.
	#define MIST(a) lua_stackdump(a); getchar();

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
	lua_pushstring( L, "jazz" ); //The new table will have this as a key name
	lua_newtable( L ); //8
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

	//
	fprintf( stderr, "Test is completed." );
	exit( 0 );
#endif

	//Check that the stack has something on it
	if ( lua_gettop( L ) == 0 )
		return 0;

	//Add a table
	lua_newtable( L );
	lua_stackdump( L );
	const int tp = lua_gettop( L ); //The top
	int pos = tp - 1; //The index on the stack I'm at
	int ti  = 1; //Where the table is on the stack
	fprintf( stderr, "table position is %d\n", ti );
	fprintf( stderr, "top (const table) is at %d\n", tp );
	fprintf( stderr, "currently at %d\n", pos );

	//Loop again, but show the value of each key on the stack
	for ( int pos = tp - 1 ; pos > 0 ; )
	{
		//
		int t = lua_type( L, pos );
		const char *type = lua_typename( L, t );
		fprintf( stderr, "Adding key [%3d] => ", pos );

	#if 1
		//Push a numeric index, since all of this should be in one table.
		lua_pushnumber( L, ti++ );
	#endif

		//Write the value into table...
		if ( t == LUA_TSTRING )
			lua_pushstring( L, lua_tostring( L, pos ) );	
		else if ( t == LUA_TFUNCTION )
			lua_pushcfunction( L, (void *)lua_tocfunction( L, pos ) );
		else if ( t == LUA_TNUMBER )
			lua_pushnumber( L, lua_tonumber( L, pos ) );
		else if ( t == LUA_TBOOLEAN)
			lua_pushboolean( L, lua_toboolean( L, pos ) );
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
		{
			void *p = lua_touserdata( L, pos );
			lua_pushlightuserdata( L, p );
		}
	#if 0
		else if ( t == LUA_TTHREAD )
			lua_pushthread( L, lua_tothread( L, pos ) );
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
	#endif
		else if ( t == LUA_TTABLE )
		{
			int np = tp + 2;
			lua_newtable( L );
			lua_writetable( L, &pos, tp + 2 );
		}

		fprintf( stderr, "Setting table at index %d\n", tp );
		//getchar();
		lua_settable( L, tp );	
		lua_stackdump( L );
		fprintf( stderr, "\n" );
		pos--;
	}

	//Remove all previous elements?
	for ( int pos = tp - 1 ; pos > 0 ; pos-- )
		lua_remove( L, pos );

	//Always return one table...
	fprintf( stderr, "Aggregated table looks like:\n" );
	lua_stackdump( L );
	return 1;
}

/*

[1] { }

*/
