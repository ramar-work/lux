#include "bridge.h"

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


char *CCtypes[] = {
	[CC_NONE]  = "None",
	[CC_MODEL] = "Model",     //Models are usually executed, and added to env 
	[CC_VIEW]  = "View",      //Views will be loaded, parsed, and sent to buffer
	[CC_FUNCT] = "Function",  //Functions are executed, payload sent to buffer
	[CC_STR]   = "String",    //Strings are just referenced and loaded to buffer
	[CC_MAX]   = NULL,        //A nul terminator
};



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
int lua_load_file( lua_State *L, const char *file, char *err  )
{
	if ( luaL_dofile( L, file ) != 0 )
	{
		fprintf( stderr, "Error occurred!\n" );
		//The entire stack needs to be cleared...
		if ( lua_gettop( L ) > 0 ) 
			return ( snprintf( err, 1023, "%s\n", lua_tostring( L, 1 ) ) ? 0 : 0 );
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
	int a = 1;
	LiteKv *kv = NULL;
	lt_reset( tt );

	obprintf( stderr, "Converting HTTP data into Lua...\n" );

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



void lua_dumptable ( lua_State *L, int *sd )
{
	lua_pushnil( L );
	while ( lua_next( L, *sd ) != 0 ) 
	{
		//Fancy printing
		fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t "[ 10 - *sd ] );
		fprintf( stderr, "[%3d:%2d] => ", *p, *sd / 2 );

		//Print both left and right side
		for ( int i = -2; i < 0; i++ )
		{
			int t = lua_type( L, i );
			const char *type = lua_typename( L, t );
			if ( t == LUA_TSTRING )
				fprintf( stderr, "(%s) %s", type, lua_tostring( L, i ));
			else if ( t == LUA_TFUNCTION )
				fprintf( stderr, "(%s) %p", type, (void *)lua_tocfunction( L, i ) );
			else if ( t == LUA_TNUMBER )
				fprintf( stderr, "(%s) %lld", type, (long long)lua_tointeger( L, i ));
			else if ( t == LUA_TBOOLEAN)
				fprintf( stderr, "%s: %s", type, lua_toboolean( L, i ) ? "true" : "false" );
			else if ( t == LUA_TTHREAD )
				fprintf( stderr, "%s: %p", type, lua_tothread( L, i ) );
			else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
				fprintf( stderr, "%s: %p", type, lua_touserdata( L, i ) );
			else if ( t == LUA_TNIL ||  t == LUA_TNONE )
				fprintf( stderr, "%s: %p", type, lua_topointer( L, i ) );
			else if ( t == LUA_TTABLE ) 
			{
				//recursion may not work here....
				fprintf( stderr, "%s\n", type );
				(*sd) ++;
				lua_dumptable( L, sd );
				(*sd) -= 2; //rewind stack depth to state before processing table
			}
			fprintf( stderr, "%s", ( i == -2 ) ? " -> " : "\n" );
		}
		lua_pop( L, 1 );
	}
	return;
}

void lua__stackdump ( lua_State *L, int *p, int *sd )
{
	//No top
	if ( lua_gettop( L ) == 0 )
		return;

	//Loop through all of the values that are on the stack
	for ( int i=0; i<lua_gettop(L); i++ )
		printf( "[%d] %s\n", i, lua_typename( L, lua_type( L, i ) ) );

	//Loop again, but show the value of each key on the stack
	for ( int ii = 0; ii < lua_gettop( L ); ii++ ) 
	{
		int t = lua_type( L, ii );
		const char *type = lua_typename( L, t );
		if ( t == LUA_TSTRING )
			fprintf( stderr, "(%s) %s", type, lua_tostring( L, ii ));
		else if ( t == LUA_TFUNCTION )
			fprintf( stderr, "(%s) %p", type, (void *)lua_tocfunction( L, ii ) );
		else if ( t == LUA_TNUMBER )
			fprintf( stderr, "(%s) %lld", type, (long long)lua_tointeger( L, ii ));
		else if ( t == LUA_TBOOLEAN)
			fprintf( stderr, "%s: %s", type, lua_toboolean( L, ii ) ? "true" : "false" );
		else if ( t == LUA_TTHREAD )
			fprintf( stderr, "%s: %p", type, lua_tothread( L, ii ) );
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA )
			fprintf( stderr, "%s: %p", type, lua_touserdata( L, ii ) );
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			fprintf( stderr, "%s: %p", type, lua_topointer( L, ii ) );
		else if ( t == LUA_TTABLE ) 
		{
			(*sd)++;
			lua_dumptable( L, 0, sd );
			*sd--;
		}
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
Loader *parse_route ( Loader *l, int lsize, Table *httpTable, Table *routeTable )
{
	//Define and declare
	Loader *LL = l;
	char us[ 1024 ] = { 0 };
	int szof_us = sizeof( us );
	int b     = 0;
	int count = 0; 
	int ind   = lt_get_long_i( httpTable, (uint8_t *)"URL", 3 );

	//This should be checked out of this function
	while ( lt_rettype( httpTable, 0, ind++ ) != LITE_TRM )
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
	lt_dump( httpTable );
	lt_dump( routeTable );
#endif

	//Recycle the same buffer for each "merge" of the URL
	for ( int i = ++ind; i < count; i++ ) 
	{
		//Get the value from URL table and combine it into string buffer 'us'... 
		int yh = 0;
		LiteBlob *bb = &lt_blob_at( httpTable, i );
		memcpy( &us[ b ], &bb->blob[ 1 ], bb->size - 1 );
		b += bb->size - 1;

		//Check the hash in the routing table and tell me what it is...
		if ( (yh = lt_get_long_i( routeTable, (uint8_t *)us, b )) == -1 )
			return err( NULL, "val not found '%s'...\n", us );
		else
		{
			//Get the type of the element that's there
			int type = lt_rettype( routeTable, 1, yh );
		#if 1
			fprintf( stderr, "hash and type of " ); 
			write( 2, "'", 1 );
			write( 2, us, b );
			write( 2, "'", 1 );
			fprintf( stderr, " - %d, %s\n", yh, lt_typename( type ) );
		#endif

		#if 0
			//Depending on type, the router action will change
			if ( type != LITE_USR && type != LITE_BLB && type != LITE_TXT && type != LITE_TBL ) 
				err( 0, "type '%s' not accepted...\n", lt_typename( type ) );
			//A Lua function most likely...not ready yet
			else if ( type == LITE_USR )
				err( 0, "function type not accepted (yet)...\n" );
			//Binary data..., unsure how this will work now
			else if ( type == LITE_BLB ) 
				err( 0, "binary type not accepted (yet)...\n" );
		#else
			if ( type != LITE_TXT && type != LITE_TBL )
				return err( NULL, "Type '%s' not accepted...\n", lt_typename( type ) );
		#endif
			else if ( type == LITE_TXT )
				LL->type = CC_STR, LL->content = lt_text_at( routeTable, yh ), LL++ ;
			else if ( type == LITE_TBL )
			{
				//Define what models and views look like
				struct { char *fn; int cn; } ab[] =
					{ { "model", CC_MODEL }, { "view", CC_VIEW } };

				//Check for 'model(s)' and 'view(s)'
				for ( int ii=0; ii < 2; ii++ )
				{
					char *locate = strcmbd( ".", us, ab[ ii ].fn );
					int m = -1;
					if ((m = lt_get_long_i( routeTable, (uint8_t *)locate, strlen( locate ))) > -1 )
					{
						int mtype = lt_rettype( routeTable, 1, m  );
						//Then check the type and do something
						if ( mtype != LITE_TXT && mtype != LITE_TBL /* && mtype != LITE_USR */ )
							return err( NULL, "type '%s' not accepted for table...\n", lt_typename( mtype ) );
						else if ( mtype == LITE_TXT )
							LL->content = lt_text_at( routeTable, m ), LL->type = ab[ ii ].cn, LL++;
						else if ( mtype == LITE_TBL ) 
						{
							//Increment current table index until you hit a terminator
							while ( lt_rettype( routeTable, 0, ++m ) != LITE_TRM ) 
							{
								int kt = lt_rettype( routeTable, 0, m ), vt = lt_rettype( routeTable, 1, m );
								//return err( NULL, "type '%s' not accepted for table...\n", lt_typename( type ) );
								if ( vt != LITE_TXT )
									printf( "Value type '%s' not yet supported...", lt_typename( mtype ) );
								else 
								{
									LL->content = lt_text_at( routeTable, m );	
									LL->type = ab[ii].cn; //ctype[ ii ];
									LL++;
								}
							} // while ( lt_rettype( ... ) != LITE_TRM )
						} // ( mtype == LITE_TBL )
					} // ( m == -1 )
					free( locate );
				} // for ( ;; )
			}	// else ( type ==	LITE_NON )
		}
	
	#if 1
		while ( LL->type ) {
			printf( "%s -> %s\n", printCCtype( LL->type ), LL->content );
			LL++;
		}
	#endif

		//Add to the string for the next iteration
		fprintf( stderr, "length of early bar: %d\n", b );
		b += 1;
		us[ b ] = '.';	

		//uh....
		fprintf( stderr, "length of appended bar: %d\n", b );
		fprintf( stderr, "%s", "next iteration of URL: '" );
		//write( 2, us, b + 1 );
		fprintf( stderr, "%s", "'\n" );
	}

#if 0
	//This test is done.  I want to see the structure before moving on.
	//printf( "Execution path for route: %s\n", tt->url );
	Loader *Ld = &l[0];
	while ( Ld->type ) {
		printf( "%s -> %s\n", printCCtype( Ld->type ), Ld->content );
		Ld++;
	}
#endif
	return l;
}
