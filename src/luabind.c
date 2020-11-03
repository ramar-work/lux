//compile me with:
//gcc -Wall -c luabind.o luabind.c
#include "luabind.h"
#include "util.h"

#define obprintf(...)

#if 0
//An example Lua function
static int foo (lua_State *L) {
       int n = lua_gettop(L);    /* number of arguments */
       lua_Number sum = 0.0;
       int i;
       for (i = 1; i <= n; i++) {
         if (!lua_isnumber(L, i)) {
           lua_pushliteral(L, "incorrect argument");
           lua_error(L);
         }
         sum += lua_tonumber(L, i);
       }
       lua_pushnumber(L, sum/n);        /* first result */
       lua_pushnumber(L, sum);         /* second result */
       return 2;                   /* number of results */
     }


//Being able to do these piecemeal might prove to be simpler
void *lua_set_methods ( lua_State *L, const char *namespace, struct lua_function **f ) {

	//Set each function	
	while ( *f ) {
		//copy( (*f)->name, (*f)->function );
		f++;
	}

	//Set a namespace
	lua_setglobal( L, namespace );
	return NULL;
}  
#endif


static void lua_dumptable ( lua_State *L, int *pos, int *sd ) {
	lua_pushnil( L );
	FPRINTF( "*pos = %d\n", *pos );

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
		getchar();
		FPRINTF( "Popping key\n" );
		lua_pop( L, 1 );
	}
	return;
}



//Loop through a table in memory
void lua_loop ( lua_State *L ) {
	int a = lua_gettop( L );
	FPRINTF( "Looping through %d values.\n", a );

	for ( int i=1; i <= a; i++ ) {
		FPRINTF( "%-2d: %s\n", i, lua_typename( L, lua_type( L, i )) );
		//lua_getindex(L, i );
	}
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


//Convert Lua tables to regular tables
int lua_to_table (lua_State *L, int index, Table *t ) {
	static int sd;
	lua_pushnil( L );
	FPRINTF( "Current stack depth: %d\n", sd++ );

	while ( lua_next( L, index ) != 0 ) {
		int kt, vt;
		FPRINTF( "key, value: " );

		//This should pop both keys...
		FPRINTF( "%s, %s\n", lua_typename( L, lua_type(L, -2 )), lua_typename( L, lua_type(L, -1 )));

		//Keys
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			FPRINTF( "key: %lld\n", (long long)lua_tointeger( L, -2 ));
		else if ( kt  == LUA_TSTRING )
			FPRINTF( "key: %s\n", lua_tostring( L, -2 ));

		//Values
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			FPRINTF( "val: %lld\n", (long long)lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			FPRINTF( "val: %s\n", lua_tostring( L, -1 ));

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
			FPRINTF( "Descending because value at %d is table...\n", -1 );
			lua_loop( L );
			lua_to_table( L, index + 2, t ); 
			lt_ascend( t );
			sd--;
		}

		FPRINTF( "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING ) {
			lt_finalize( t );
		}
		lua_pop(L, 1);
	}

	lt_lock( t );
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
			FPRINTF( "%s\n", ( i ) ? lt_typename( t ) : lt_typename( t ));

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

	FPRINTF( "Done!\n" );
	return 1;
}


//Combine table indexes...?
int lua_combine ( lua_State *L, char *err, int errlen ) {

	//Check that there are actually tables to combine
	if ( lua_gettop( L ) == 0 ) {
		snprintf( err, errlen, "No values are on the Lua stack." );
		return 0;
	}

	//Pull all the values from all of the tables and go...
	lua_newtable(L);
	for ( int t, cur_index = lua_gettop( L ) - 1 ; cur_index > 0 ; ) {

		//Write the value into table...
		if ( ( t = lua_type( L, cur_index ) ) == LUA_TSTRING )
			lua_pushstring( L, lua_tostring( L, cur_index ) );	
		else if ( t == LUA_TNUMBER )
			lua_pushnumber( L, lua_tonumber( L, cur_index ) );
		else if ( t == LUA_TBOOLEAN)
			lua_pushboolean( L, lua_toboolean( L, cur_index ) );
		else if ( t == LUA_TTABLE ) {
			//I need to iterate through all the top level values only
			lua_pushnil( L );
			while ( lua_next( L, cur_index ) != 0 ) {
				char val[ 1024 ] = { 0 };
				int p = 0;
				if ( lua_type( L, -2 ) == LUA_TSTRING ) {
					snprintf( val, sizeof( val ), "%s", lua_tostring( L, -2 ) );
					lua_settable( L, cur_index + 1 );	
					lua_pushstring( L, val );
				}
				else if ( lua_type( L, -2 ) == LUA_TNUMBER ) {
					p = lua_tonumber( L, -2 );
					lua_settable( L, cur_index + 1 );	
					lua_pushnumber( L, p );
				}
				else {
					//Any deallocs that need to be done should be done...
					snprintf( err, errlen, "%s", "Lua received key that was neither a string or number." );
					return 0;
				}
			}

			//pop the table off once done (since it should have been copied)
			lua_remove( L, cur_index );	
		}
	#if 1
		else {
			snprintf( err, errlen, "%s", "Lua attempted to aggregate a value that was not a string, number or table." );
			return 0;
		}
	#else
		else if ( t == LUA_TFUNCTION )
			lua_pushcfunction( L, (void *)lua_tocfunction( L, cur_index ) );
		else if ( t == LUA_TTHREAD )
			lua_pushthread( L, lua_tothread( L, cur_index ) );
		else if ( t == LUA_TNIL ||  t == LUA_TNONE )
			fprintf( stderr, "(%8s) %p", type, lua_topointer( L, cur_index ) );
		else if ( t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA ) {
			void *p = lua_touserdata( L, cur_index );
			lua_pushlightuserdata( L, p );
		}
		else {
			snprintf( err, errlen, "%s", "Lua received key that was neither a string or number." );
			return 0;
		}
	#endif
		cur_index--;
	}

	return 1;
}


//A better load file
int lua_exec_string( lua_State *L, const char *str, char *err, int errlen ) {
	int lerr = 0;
	int len = 0;

	if ( !str || strlen( str ) < 3 ) {
		snprintf( err, errlen, "%s", "No Lua string supplied to load or execute." );
		return 0;
	}

	//Load the string, execute
	if (( lerr = luaL_loadstring( L, str )) != LUA_OK ) {
		if ( lerr == LUA_ERRSYNTAX )
			len = snprintf( err, errlen, "Syntax error: " );
		else if ( lerr == LUA_ERRMEM )
			len = snprintf( err, errlen, "Memory allocation error: " );
#if 0
		else if ( lerr == LUA_ERRGCMM )
			len = snprintf( err, errlen, "GC meta-method error: " );
#endif
		else {
			len = snprintf( err, errlen, "Unknown error occurred: " );
		}
	
		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
		//fprintf(stderr, "LUA LOAD ERROR: %s, %s", err, (char *)lua_tostring( L, -1 ) );
		lua_pop( L, lua_gettop( L ) );
		return 0;	
	}

	//Then execute
	if (( lerr = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
		if ( lerr == LUA_ERRRUN ) 
			len = snprintf( err, errlen, "Runtime error: " );
		else if ( lerr == LUA_ERRMEM ) 
			len = snprintf( err, errlen, "Memory allocation error: " );
		else if ( lerr == LUA_ERRERR ) 
			len = snprintf( err, errlen, "Error while running message handler: " );
#if 0
		else if ( lerr == LUA_ERRGCMM ) {
			len = snprintf( err, errlen, "Error while running __gc metamethod at: " );
		}
#endif

		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
		//fprintf(stderr, "LUA EXEC ERROR: %s, %s", err, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		return 0;	
	}
	return 1;	
}


//A better load file
int lua_exec_file( lua_State *L, const char *f, char *err, int errlen ) {
	int lerr = 0;
	int len = 0;
	struct stat check;

	if ( !f || !strlen( f ) ) {
		snprintf( err, errlen, "%s", "No filename supplied to load or execute." );
		return 0;
	}

	//Since this is supposed to accept a file, why not just check for existence?
	if ( stat( f, &check ) == -1 ) {
		snprintf( err, errlen, "File %s inaccessible: %s.", f, strerror(errno) );
		return 0;
	}

	if ( check.st_size == 0 ) {
		snprintf( err, errlen, "File %s is zero-length.  Nothing to execute.", f );
		return 0;
	}

	//Load the string, execute
	if (( lerr = luaL_loadfile( L, f )) != LUA_OK ) {
		if ( lerr == LUA_ERRSYNTAX )
			len = snprintf( err, errlen, "Syntax error at %s: ", f );
		else if ( lerr == LUA_ERRMEM )
			len = snprintf( err, errlen, "Memory allocation error at %s: ", f );
#if 0
		else if ( lerr == LUA_ERRGCMM )
			len = snprintf( err, errlen, "GC meta-method error at %s: ", f );
#endif
		else if ( lerr == LUA_ERRFILE )
			len = snprintf( err, errlen, "File access error at %s: ", f );
		else {
			len = snprintf( err, errlen, "Unknown error occurred at %s: ", f );
		}
	
		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
		//fprintf(stderr, "LUA LOAD ERROR: %s, %s", err, (char *)lua_tostring( L, -1 ) );
		lua_pop( L, lua_gettop( L ) );
		return 0;	
	}

	//Then execute
	if (( lerr = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
		if ( lerr == LUA_ERRRUN ) 
			len = snprintf( err, errlen, "Runtime error when executing %s: ", f );
		else if ( lerr == LUA_ERRMEM ) 
			len = snprintf( err, errlen, "Memory allocation error at %s: ", f );
		else if ( lerr == LUA_ERRERR ) 
			len = snprintf( err, errlen, "Error while running message handler for %s: ", f );
#if 0
		else if ( lerr == LUA_ERRGCMM ) {
			len = snprintf( err, errlen, "Error while running __gc metamethod at %s: ", f );
		}
#endif

		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
		//fprintf(stderr, "LUA EXEC ERROR: %s, %s", err, (char *)lua_tostring( L, -1 ) );	
		lua_pop( L, lua_gettop( L ) );
		return 0;	
	}
	return 1;	
}


//Wraps a nice standard return set for Lua values 
int lua_end ( lua_State *L, int status, const char *errmsg ) {
	lua_newtable( L );
	lua_pushstring( L, "status" );
	lua_pushboolean( L, status );
	lua_settable( L, 1 );

#if 1
	//Is it worth it to record the time all the time?
	const char timefmt[] = "Executed %a, %d %b %Y %H:%M:%S %Z"; 
	char timestr[256] = {0};
	time_t t = time( NULL );
	struct tm *tmp = localtime( &t );
	if ( !tmp || !strftime( timestr, sizeof(timestr), timefmt, tmp ) ) {
		;
	}
	else {
		lua_pushstring( L, "time" );
		lua_pushstring( L, timestr );
		lua_settable( L, 1 );
	}
#endif

	//the value on the other side needs to be something or other
	if ( !status ) {
		lua_pushstring( L, "results" );
		lua_pushboolean( L, 0 );
		lua_settable( L, 1 );
		lua_pushstring( L, "error" );
		lua_pushstring( L, errmsg );
		lua_settable( L, 1 );
	}
	return 1;
}
