#include "bridge.h"

#ifdef LUA_53
 #define lua_rotate( ... ) lua_rotate( __VA_ARGS__ )
#else
 #define lua_rotate( ... )
#endif


#define lua_loopstack( L ) \
	fprintf ( stderr, "====> CURRENT STACK (AT %s:%s:%d) LOOKS LIKE: <=====\n", __FILE__, __func__, __LINE__ ); \
	lua_loopstackvals( L ); getchar()


char *CCtypes[] = {
	[CC_NONE]  = "None",
	[CC_MODEL] = "Model",     //Models are usually executed, and added to env 
	[CC_VIEW]  = "View",      //Views will be loaded, parsed, and sent to buffer
	[CC_FUNCT] = "Function",  //Functions are executed, payload sent to buffer
	[CC_STR]   = "String",    //Strings are just referenced and loaded to buffer
	[CC_MAX]   = NULL,        //A nul terminator
};


//Loop through a table in memory
void lua_loop ( lua_State *L ) {
	int a = lua_gettop( L );
	obprintf( stderr, "Looping through %d values.\n", a );

	for ( int i=1; i <= a; i++ ) {
		obprintf( stderr, "%-2d: %s\n", i, lua_typename( L, lua_type( L, i )) );
		//lua_getindex(L, i );
	}
}


//Print CC type
char *printCCtype ( CCtype cc ) {
	switch (cc) {
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
int lua_load_file( lua_State *L, const char *file, char **err  ) {
	if ( luaL_dofile( L, file ) != 0 ) {
		fprintf( stderr, "Error occurred!\n" );
		//The entire stack needs to be cleared...
		if ( lua_gettop( L ) > 0 ) {
			fprintf( stderr, "%s\n", lua_tostring(L, 1) );
			return ( snprintf( *err, 1023, "%s\n", lua_tostring( L, 1 ) ) ? 0 : 0 );
		}
	}
	return 1;	
}


//
int lua_load_file2( lua_State *L, Table *t, const char *file, char *err ) {
	if ( luaL_dofile( L, file ) != 0 ) {
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
int table_to_lua (lua_State *L, int index, Table *tt) {
	int a = index;
	LiteKv *kv = NULL;
	lt_reset( tt );

	while ( (kv = lt_next( tt )) ) {
		struct { int t; LiteRecord *r; } items[2] = {
			{ kv->key.type  , &kv->key.v    },
			{ kv->value.type, &kv->value.v  } 
		};

		int t=0;
		for ( int i=0; i<2; i++ ) {
			LiteRecord *r = items[i].r; 
			t = items[i].t;
			obprintf( stderr, "%s\n", ( i ) ? lt_typename( t ) : lt_typename( t ));

			if ( i ) {
				if (t == LITE_NUL)
					;
				else if (t == LITE_USR)
					lua_pushstring( L, "usrdata received" ); //?
				else if (t == LITE_TBL) {
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
			else if (t == LITE_TRM) {
				a -= 2;	
				break;
			}
			else if (t == LITE_BLB) {
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
int lua_to_table (lua_State *L, int index, Table *t ) {
	static int sd;
	lua_pushnil( L );
	obprintf( stderr, "Current stack depth: %d\n", sd++ );

	while ( lua_next( L, index ) != 0 ) {
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
		else if ( vt == LUA_TTABLE ) {
			lt_descend( t );
			obprintf( stderr, "Descending because value at %d is table...\n", -1 );
			lua_loop( L );
			lua_to_table( L, index + 2, t ); 
			lt_ascend( t );
			sd--;
		}

		obprintf( stderr, "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING ) {
			lt_finalize( t );
		}
		lua_pop(L, 1);
	}

	lt_lock( t );
	return 1;
}


void lua_stackclear ( lua_State *L ) {
	int top = lua_gettop( L );
	lua_pop( L, top );
}


void lua_loopstackvals( lua_State *L ) {
	for ( int pos = 1; pos <= lua_gettop( L ); pos++ ) {
		int t = lua_type( L, pos );
		const char *type = lua_typename( L, t );
		fprintf( stderr, "[%3d] => ", pos );
		fprintf( stderr, " %s\n", type );
	}
	fprintf( stderr, "\n" );
}



void lua_dumptable ( lua_State *L, int *pos, int *sd )
{
	lua_pushnil( L );
	//fprintf( stderr, "*pos = %d\n", *pos );

	while ( lua_next( L, *pos ) != 0 ) {
		//Fancy printing
		//fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t\t"[ 10 - *sd ] );
		PRETTY_TABS( *sd );
		fprintf( stderr, "[%3d:%2d] => ", *pos, *sd );

		//Print both left and right side
		for ( int i = -2; i < 0; i++ ) {
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
			else if ( t == LUA_TTABLE ) {
			#if 1
				fprintf( stderr, "(%8s) %p\n", type, lua_topointer( L, i ) );
				(*sd) ++, (*pos) += 2;
				lua_dumptable( L, pos, sd );
				(*sd) --, (*pos) -= 2;
			#else
				fprintf( stderr, "(%8s) %p {\n", type, lua_topointer( L, i ) );
				int diff = lua_gettop( L ) - *pos;

				(*sd) ++, (*pos) += diff;
				lua_dumptable( L, pos, sd );
				(*sd) --, (*pos) -= diff;
			#endif
				PRETTY_TABS( *sd );
				fprintf( stderr, "}" );
			}

			fprintf( stderr, "%s", ( i == -2 ) ? " -> " : "\n" );
			//PRETTY_TABS( *sd );
		}

		lua_pop( L, 1 );
	}
	return;
}




void lua_stackdump ( lua_State *L ) {
	//No top
	if ( lua_gettop( L ) == 0 )
		return;

	//Loop again, but show the value of each key on the stack
	for ( int pos = 1; pos <= lua_gettop( L ); pos++ ) {
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
		else if ( t == LUA_TTABLE ) {
		#if 0
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
		#else
			fprintf( stderr, "(%8s) %p {\n", type, lua_topointer( L, pos ) );
			int sd = 1;
			lua_dumptable( L, &pos, &sd );
			fprintf( stderr, "}" );
		#endif
		}	
		fprintf( stderr, "\n" );
	}
	return;
}


//Parse the current route according to what was in the table.
Loader *parse_route ( Loader *l, int lsize, HTTP *http, Table *routeTable ) {
	//Define and declare
	Loader *LL = l;
	Table *http_r = &http->request.table;
	char us[ 1024 ] = { 0 }, *fullRoute = NULL;
	int szof_us = sizeof( us );
	int b     = 0;
	int count = 0; 
	int url_ind   = lt_get_long_i( http_r, (uint8_t *)"URL", 3 );

	//This should be checked out of this function
	while ( lt_rettype( http_r, 0, url_ind++ ) != LITE_TRM )
		count++;

	//Fail if this happens, b/c the URL wasn't done right
	if ( count < 2 ) {
		return NULL; //err( NULL, "URL doesn't exist." );
	}

	//Reset count and set index to the top of the URL chain
	url_ind -= count;	

	// TODO:'/' should always resolve to default
	// TODO: add regexp support...
	if ( (int)strlen( http->request.path ) == 1 && strcmp( http->request.path, "/" ) == 0 )
		fullRoute = "routes.default";
	else {
		fullRoute = strcmbd( "/", "routes", &http->request.path[1] );
		for ( int i=0; i<strlen(fullRoute); i++ ) {
			( fullRoute[i] == '/' ) ? fullRoute[i] = '.' : 0;
		}
	}

	//Now check the hash against the thing and loop through shit...
	char *modelTag = strcmbd( ".", fullRoute, "model" );
	char	*viewTag = strcmbd( ".", fullRoute, "view" );

	int mh = lt_get_long_i( routeTable, (uint8_t *)modelTag, strlen(modelTag) );
	int	vh = lt_get_long_i( routeTable, (uint8_t *)viewTag, strlen(viewTag) );
#if 0
fprintf( stderr, "%s, %s\n", modelTag, viewTag );
fprintf( stderr, "%d, %d\n", mh, vh );
exit(0);
#endif

	//Dump the routeTable, cuz I think this where the bug is...
	lt_dump( routeTable );
//exit(0);

	//Loop through 'til you get a terminator, means you're at the end...
	int h[2]={mh, vh};
#if 0
nsprintf( modelTag );
nsprintf( viewTag );
niprintf( h[0] );
niprintf( h[1] );
exit(0);
#endif

	for ( int ih=0; ih<2; ih++ ) {
		if ( lt_valuetypeat( routeTable, h[ ih ] ) != LITE_TBL ) {
			LL->type = (!ih) ? CC_MODEL : CC_VIEW;
			//TODO: This will crash on zero-length route strings... (why oen would do that, idk. but still have to account for it.)
			LL->content = strdup( lt_text_at( routeTable, h[ ih ] )) ;
			LL++ ;
		}
		else {
			for ( int i=h[ ih ] + 1; ; i++ ) {
				char *lt_char = NULL;
				int lt_type =	lt_valuetypeat( routeTable, i );

				// Stop at nul or term
				if ( lt_type == LITE_NUL || lt_type == LITE_TRM || !(lt_char =	lt_text_at( routeTable, i )) )
					break;

				//LL is either accessed wrong or not init'd.
				LL->type = (!ih) ? CC_MODEL : CC_VIEW;
				LL->content = strdup( lt_char );
				LL++ ;
			}
		}
	}
	free( modelTag );
	free( viewTag );
	return l;
}



int load_and_run_files ( Loader *l ) { 
	return 1;
}

int counter=0;


//Callback
int lua_callback(void *nu, int argc, char **argv, char **cn) {
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
int lua_db_insert ( lua_State *L ) { 
	return 0;
}


int lua_writetable( lua_State *L, int *pos, int ti ) {
	lua_pushnil( L );
	while ( lua_next( L, *pos ) != 0 ) {
		//Set the current index
		int t = lua_type( L, -2 );

	#if 1
		//Copy the index and rotate the top and bottom
		lua_pushnil( L );
		lua_copy( L, -3, lua_gettop( L ) );
	//lua_stackdump( L );
		lrotate( L, -2, -1 );
	#else
		//There must be a way to do this that will run on older Luas
	#endif

		//Now, setting the table works about the same as popping
		lua_settable( L, ti );	
		//lua_stackdump( L );
	}
	return 1;
}


//This is a Lua function
int lua_db ( lua_State *L ) {
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
	if ( !sq_init( &b ) || !sq_open( &b, filename ) ) {
		fprintf( stderr, "Error was: %d, %s\n", __LINE__, "unknown as of yet..." );
		return 0; /*return an error to Lua*/
	}

	//Save the results to table
	if ( !sq_save( &b, final_query, "db", NULL ) ) {
		fprintf( stderr, "Error was: %d, %s\n", __LINE__, "unknown as of yet..." );
		return 0; /*return an error to Lua*/
	}

 #if 1
	//Dump them
	Table *x = &b.kvt;
	lt_dump( x );
 #endif

	//Clear the stack and add a table
	lua_settop( L, 0 );
	lua_newtable( L );
 #if 0
	//lua_stackdump( L );
 #endif

	//Put results in Lua
	if ( !table_to_lua( L, 1, &b.kvt ) ) {
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


int lua_aggregate (lua_State *L) {
	//Check that the stack has something on it
	if ( lua_gettop( L ) == 0 )
		return 0;

	//Add a table
#if 1
	lua_newtable(L);
	const int tp = lua_gettop( L ); //The top
	int pos = tp - 1; //The index on the stack I'm at
	int ti  = 1; //Where the table is on the stack
	#if 0
	fprintf(stderr,"Stackdump:\n");
	fprintf( stderr, "table position is %d\n", ti );
	fprintf( stderr, "top (const table) is at %d\n", tp );
	fprintf( stderr, "currently at %d\n", pos );
	#endif

	//Loop again, but show the value of each key on the stack
	for ( int pos = tp - 1 ; pos > 0 ; ) {
		int t = lua_type( L, pos );
		const char *type = lua_typename( L, t );
		//fprintf( stderr, "Adding key [%3d] => ", pos );

		//Push a numeric index, since all of this should be in one table.
		lua_pushnumber( L, ti++ );

		//Write the value into table...
#else
	lua_newtable( L );

	//Get the index of the new table
	int tp = lua_gettop( L );
	int ti  = 1;

	//Loop again, but show the value of each key on the stack
	for ( int pos = 1; pos < tp; tp-- /*pos++*/ ) 
	{
		int t = lua_type( L, pos );
		const char *type = lua_typename( L, t );
		lua_pushnumber( L, ti++ );
#endif

		if ( t == LUA_TSTRING )
			lua_pushstring( L, lua_tostring( L, pos ) );	
		else if ( t == LUA_TFUNCTION )
			lua_pushcfunction( L, (void *)lua_tocfunction( L, pos ) );
		else if ( t == LUA_TNUMBER )
			lua_pushnumber( L, lua_tonumber( L, pos ) );
		else if ( t == LUA_TBOOLEAN)
			lua_pushboolean( L, lua_toboolean( L, pos ) );
#if 1
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA ) {
			void *p = lua_touserdata( L, pos );
			lua_pushlightuserdata( L, p );
		}
	#if 0
		else if ( t == LUA_TTHREAD )
			lua_pushthread( L, lua_tothread( L, pos ) );
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, pos ) );
	#endif
		else if ( t == LUA_TTABLE ) {
			int np = tp + 2;
			lua_newtable( L );
			lua_writetable( L, &pos, tp + 2 );
		}

		//fprintf( stderr, "Setting table at index %d\n", tp );
		//getchar();
		lua_settable( L, tp );	
		//lua_stackdump( L );
		//fprintf( stderr, "\n" );
		pos--;
	}

	//Remove all previous elements?
	for ( int pos = tp - 1 ; pos > 0 ; pos-- ) {
		//lua_remove( L, pos );
	}

	//Always return one table...
	//fprintf( stderr, "Aggregated table looks like:\n" )p

	//lua_stackdump( L );
#else
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			lua_pushnil( L );
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
			lua_pushlightuserdata( L, lua_touserdata( L, pos ));
		else if ( t == LUA_TTABLE ) 
		{
			lua_pushnil( L );
			lua_copy( L, pos, lua_gettop( L ) ); 
		}

		//set the new table
		lua_settable( L, tp );	
		lua_remove( L, pos );
	}
#endif
	return 1;
}


//Table of Lua functions
int abc ( lua_State *L ) { fprintf( stderr, "chicken" ); return 0; } 
