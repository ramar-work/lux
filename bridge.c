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



//
int lua_load_file( lua_State *L, const char *file, char *err  )
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



#if 0
//Print a set of values at a particular index
static void lua_tprintindex (LiteKv *tt, int ind)
{
	int         w = 0;
  char b[lt_buflen]; 
	memset(b, 0, lt_buflen);
	struct { int t; LiteRecord *r; } items[2] = {
		{ tt->key.type  , &tt->key.v    },
		{ tt->value.type, &tt->value.v  } 
	};

	for ( int i=0; i<2; i++ ) 
	{
		LiteRecord *r = items[i].r; 
		int t = items[i].t;
		if ( i ) 
		{
			memcpy( &b[w], " -> ", 4 );
			w += 4;
			/*LITE_NODE is handled in printall*/
			if (t == LITE_NON)
				w += snprintf( &b[w], lt_buflen - w, "%s", "is uninitialized" );
#ifdef LITE_NUL
			else if (t == LITE_NUL)
				w += snprintf( &b[w], lt_buflen - w, "is terminator" );
#endif
			else if (t == LITE_USR)
				w += snprintf( &b[w], lt_buflen - w, "userdata [address: %p]", r->vusrdata );
			else if (t == LITE_TBL) 
			{
				LiteTable *rt = &r->vtable;
				w += snprintf( &b[w], lt_buflen - w, 
					"table [address: %p, ptr: %ld, elements: %d]", (void *)rt, rt->ptr, rt->count );
			}
		}
		if (t == LITE_FLT || t == LITE_INT)
			w += snprintf( &b[w], lt_buflen - w, "%d", r->vint );
		else if (t == LITE_FLT)
			w += snprintf( &b[w], lt_buflen - w, "%f", r->vfloat );
		else if (t == LITE_TXT)
			w += snprintf( &b[w], lt_buflen - w, "%s", r->vchar );
		else if (t == LITE_TRM)
			w += snprintf( &b[w], lt_buflen - w, "%ld", r->vptr );
		else if (t == LITE_BLB) 
		{
			LiteBlob *bb = &r->vblob;
			if ( bb->size < 0 )
				return;	
			if ( bb->size > lt_maxbuf )
				w += snprintf( &b[w], lt_buflen - w, "is blob (%d bytes)", bb->size);
			else {
				memcpy( &b[w], bb->blob, bb->size ); 
				w += bb->size;
			}
		}
	}

	write(2, b, w);
	write(2, "\n", 1);
}
#endif	


//Move through a list of these 
int route_controller ( const char **url )
{

	return 1;
}
