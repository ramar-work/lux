/* ------------------------------------------- * 
 * lua.c
 * ======
 * 
 * Summary 
 * -------
 * -
 *
 * Usage
 * -----
 * Lua primitives
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "lua.h"

//Dump a stack
void lua_istack ( lua_State *L ) {
	fprintf( stderr, "\n" );
	for ( int i = 1; i <= lua_gettop( L ); i++ ) {
		fprintf( stderr, "[%d] => %s", i, lua_typename( L, lua_type( L, i ) ) );
		if ( lua_type( L, i ) == LUA_TSTRING ) {
			fprintf( stderr, " > %s", lua_tostring( L, i ) );
		}
		fprintf( stderr, "\n" );
	}
}



//TODO: Reject keys that aren't a certain type
void lua_dumpstack ( lua_State *L ) {
	const char spaces[] = /*"\t\t\t\t\t\t\t\t\t\t"*/"          ";
	const int top = lua_gettop( L );
	struct data { unsigned short count, index; } data[64], *dd = data;
	memset( data, 0, sizeof( data ) );
	dd->count = 1;

	//Return early if no values
	if ( ( dd->index = top ) == 0 ) {
		fprintf( stderr, "%s\n", "No values on stack." );
		return;
	}

	//Loop through all values on the stack
	for ( int it, depth=0, ix=0, index=top; index >= 1; ix++ ) {
		fprintf( stderr, "%s[%d:%d] ", &spaces[ 10 - depth ], index, ix );

		for ( int t = 0, count = dd->count; count > 0; count-- ) {
			if ( ( it = lua_type( L, index ) ) == LUA_TSTRING )
				fprintf( stderr, "(%s) %s", lua_typename( L, it ), lua_tostring( L, index ) );
			else if ( it == LUA_TFUNCTION )
				fprintf( stderr, "(%s) %d", lua_typename( L, it ), index );
			else if ( it == LUA_TNUMBER )
				fprintf( stderr, "(%s) %lld", lua_typename( L, it ), (long long)lua_tointeger( L, index ) );
			else if ( it == LUA_TBOOLEAN)
				fprintf( stderr, "(%s) %s", lua_typename( L, it ), lua_toboolean( L, index ) ? "T" : "F" );
			else if ( it == LUA_TTHREAD )
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_tothread( L, index ) );
			else if ( it == LUA_TLIGHTUSERDATA || it == LUA_TUSERDATA )
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_touserdata( L, index ) );
			else if ( it == LUA_TNIL || it == LUA_TNONE )
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_topointer( L, index ) );
			else if ( it == LUA_TTABLE ) {
				fprintf( stderr, "(%s) %p", lua_typename( L, it ), lua_topointer( L, index ) );
			}

			//Handle keys
			if ( count > 1 )
				index++, t = 1, dd->count -= 2, fprintf( stderr, " -> " );
			//Handle new tables
			else if ( it == LUA_TTABLE ) {
				lua_pushnil( L );
				if ( lua_next( L, index ) != 0 ) {
					++dd, ++depth; 
					dd->index = index, dd->count = 2, index = lua_gettop( L );
				}
			}
			//Handle situations in which a table is on the "other side"
			else if ( t ) {
				lua_pop( L, 1 );
				//TODO: This is quite gnarly... Maybe clarify a bit? 
				while ( depth && !lua_next( L, dd->index ) ) {
					( ( index = dd->index ) > top ) ? lua_pop( L, 1 ) : 0;
					--dd, --depth, fprintf( stderr, "\n%s}", &spaces[ 10 - depth ] );
				}
				( depth ) ? dd->count = 2, index = lua_gettop( L ) : 0;
			}
		}
		fprintf( stderr, "\n" );
		index--;
	}
}



//Takes all items in a table and merges them into 
//a single table (removes whatever was there)
int lua_merge( lua_State *L ) {
	const int top = lua_gettop( L );
	struct data { unsigned short index, tinsert, tpull; } data[64], *dd = data;
	memset( data, 0, sizeof( data ) );

	//Return early if no values
	if ( top < 2 ) {
		return 1;
	}

	//Push a blank table at the beginning
	lua_newtable( L ), lua_insert( L, 1 );

	//set some defaults
	dd->index = 1, dd->tinsert = 1;

	//Loop through all values on the stack
	for ( int kt, it, depth = 0, index = top + 1; index > 1; ) {
		if ( !depth ) {
			if ( ( it = lua_type( L, index ) ) == LUA_TTABLE ) {
				lua_pushnil( L );
				if ( lua_next( L, index ) )
					dd++, dd->tinsert = 1, dd->index = 1, dd->tpull = index, depth++;
				else {
					lua_pushnumber( L, dd->index );
					lua_newtable( L );
					lua_settable( L, dd->tinsert );
					dd->index ++;
				}
			}
			else {
				lua_pushnumber( L, dd->index );
				if ( ( it = lua_type( L, index ) ) == LUA_TSTRING )
					lua_pushstring( L, lua_tostring( L, index ) );
				else if ( it == LUA_TNUMBER ) {
					lua_pushinteger( L, lua_tointeger( L, index ) );
				}
				lua_settable( L, dd->tinsert );
				dd->index ++;
			}
		}
		else {
			//Add the key again to maintain lua_next behavior
			if ( ( kt = lua_type( L, -2 ) ) == LUA_TNUMBER )
				lua_pushnumber( L, lua_tonumber( L, -2 ) );
			else if ( kt == LUA_TSTRING )
				lua_pushstring( L, lua_tostring( L, -2 ) );
			else {
				//fprintf( stderr, "Invalid key type: %s\n", lua_typename( L, kt ) );
				fprintf( stderr, "ERROR: HANDLE OTHER KEY TYPES!\n" );
				return 0;
			}

			lua_insert( L, dd->tpull + 1 ), lua_settable( L, dd->tinsert );

			if ( !lua_next( L, dd->tpull ) ) {
				lua_pop( L, 1 ), dd--, depth--;
			}
		}
		( !depth ) ? index-- : 0;
	}
	return 1;
}



//A better load file
int lua_exec_file( lua_State *L, const char *f, char *err, int errlen ) {
	int len = 0, lerr = 0;
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
	#ifdef LUA_53
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
	#ifdef LUA_53
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


	
//Copy all records from a ztable_t to a Lua table at any point in the stack.
int ztable_to_lua ( lua_State *L, ztable_t *t ) {
	short ti[ 128 ] = { 0 }, *xi = ti; 

	lt_kfdump( t, 1 );

	//Push a table and increase Lua's "save table" index
	lua_newtable( L );
	*ti = 1;

	//Reset table's index
	lt_reset( t );

	//Loop through all values and copy
	for ( zKeyval *kv ; ( kv = lt_next( t ) ); ) {
		zhValue k = kv->key, v = kv->value;
		if ( k.type == ZTABLE_NON )
			return 1;	
		else if ( k.type == ZTABLE_INT ) //Arrays start at 1 in Lua, so increment by 1
			lua_pushnumber( L, k.v.vint + 1 );				
		else if ( k.type == ZTABLE_FLT )
			lua_pushnumber( L, k.v.vfloat );				
		else if ( k.type == ZTABLE_TXT )
			lua_pushstring( L, k.v.vchar );
		else if ( kv->key.type == ZTABLE_BLB)
			lua_pushlstring( L, (char *)k.v.vblob.blob, k.v.vblob.size );
		else if ( k.type == ZTABLE_TRM ) {
			lua_settable( L, *( --xi ) );
		}

		if ( v.type == ZTABLE_NUL )
			;
		else if ( v.type == ZTABLE_INT )
			lua_pushnumber( L, v.v.vint );				
		else if ( v.type == ZTABLE_FLT )
			lua_pushnumber( L, v.v.vfloat );				
		else if ( v.type == ZTABLE_TXT )
			lua_pushstring( L, v.v.vchar );
		else if ( v.type == ZTABLE_BLB )
			lua_pushlstring( L, (char *)v.v.vblob.blob, v.v.vblob.size );
		else if ( v.type == ZTABLE_TBL ) {
			lua_newtable( L );
			*( ++xi ) = lua_gettop( L );
		}
		else /* ZTABLE_TRM || ZTABLE_NON || ZTABLE_USR */ {
		#if 1
			if ( v.type == ZTABLE_TRM )
				fprintf( stderr, "Got value of type: %s\n", "ZTABLE_TRM" );
			else if ( v.type == ZTABLE_NON )
				fprintf( stderr, "Got value of type: %s\n", "ZTABLE_NON" );
			else if ( v.type == ZTABLE_USR ) {
				fprintf( stderr, "Got value of type: %s\n", "ZTABLE_USR" );
			}
		#endif
			return 0;
		}

		if ( v.type != ZTABLE_NON && v.type != ZTABLE_TBL && v.type != ZTABLE_NUL ) {
			lua_settable( L, *xi );
		}
	}
	return 1;
}


#if 0
//Count the elements in a table.
int lua_xcount ( lua_State *L, int i ) {
	int count = 0;

	if ( !lua_istable( L, i ) ) {
		fprintf( stderr, "[%s, %d] Value at %i is not a table\n", __FILE__, __LINE__, i );
		return 0;
	}

	//Descend, but keep in mind that we always have a count...
	lua_pushnil( L );
	for ( int v; lua_next( L, i ) != 0; ) {
		if ( ( v = lua_type( L, -1 ) ) == LUA_TTABLE ) {
			count += lua_count( L, i + 2 ); 
		}

		if ( lua_type( L, -2 ) == LUA_TSTRING )
			FPRINTF( "KEY IS %s => ", lua_tostring( L, -2 ) );	
		else {
			FPRINTF( "KEY IS %s => ", lua_typename( L, lua_type( L, -2 ) ) );	
		}

		FPRINTF( "(points to type %s)\n", lua_typename( L, lua_type( L, -1 ) ) );	
		lua_pop( L, 1 );
		count++;
	}

	return count;
}
#endif



//Count the elements in a table.
int lua_count ( lua_State *L, int i ) {
	int count = 0;

	if ( !lua_istable( L, i ) ) {
		fprintf( stderr, "[%s, %d] Value at %i is not a table\n", __FILE__, __LINE__, i );
		return 0;
	}

	//Descend, but keep in mind that we always have a count...
	lua_pushnil( L );
	for ( int v; lua_next( L, i ) != 0; ) {
		if ( ( v = lua_type( L, -1 ) ) == LUA_TTABLE ) {
			count += lua_count( L, i + 2 ); 
		}
#if 0
		if ( lua_type( L, -2 ) != LUA_TSTRING ) {
			fprintf( stderr, "KEY IS %s => ", lua_typename( L, lua_type( L, -2 ) ) );	
		}
		else {
			fprintf( stderr, "KEY IS %s => ", lua_tostring( L, -2 ) );	
		}

		fprintf( stderr, "( points to type %s)\n", lua_typename( L, lua_type( L, -2 ) ) );	
#endif
		lua_pop( L, 1 );
		count++;
	}

	return count;
}



//Attempts to retrieve a key from global table and clears the stack if it doesn't match.
int lua_retglobal( lua_State *L, const char *key, int type ) {
	lua_getglobal( L, key );
	int pos = lua_gettop( L );	

	if ( lua_isnil( L, pos ) || lua_type( L, pos ) != type ) {
		lua_pop( L, pos );
		return 0;
	}
	return 1;
}



#if 0
#ifndef DEBUG_H
#define TELL(fmt,a)
	1
#else
static char B[ 1024 ];
#define TELL(fmt,a) \
	memset( B, 0, sizeof(B) ) && snprintf( B, sizeof(B), fmt, a ) && fprintf( stderr, "%s", B )
#endif
#endif

#define TELL(fmt,a) 1

//Convert Lua tables to regular tables
int lua_to_ztable ( lua_State *L, int index, ztable_t *t ) {

	if ( !lua_checkstack( L, 3 ) ) {
		fprintf( stderr, "STACK OUT OF SPACE!" );	
		return 0;
	}

	lua_pushnil( L );

	while ( lua_next( L, index ) != 0 ) {
		int kt = lua_type( L, -2 ); 
		int vt = lua_type( L, -1 );

		//Get key (remember Lua indices always start at 1.  Hence the minus.
		if ( kt == LUA_TNUMBER )
			TELL( "(%lld)", lua_tointeger( L, -2 ) - 1 ) && lt_addintkey( t, lua_tointeger( L, -2 ) - 1 );
		else if ( kt == LUA_TSTRING )
			TELL( "(%s)", lua_tostring( L, -2 ) ) && lt_addtextkey( t, (char *)lua_tostring( L, -2 ));
		else {
			//Invalid key type
			fprintf( stderr, "Got invalid key in table!" );
			return 0;
		}

		//Get value
		if ( vt == LUA_TNUMBER ) {
			TELL( " (%lld)\n", lua_tointeger( L, -1 ) ) && lt_addintvalue( t, lua_tointeger( L, -1 ));
			lt_finalize( t );
		}
		else if ( vt  == LUA_TSTRING ) {
		#if 1

			//unsigned char *a = lua_tostring( L, -1 );
//FPRINTF( "%d", *

			TELL( " (%s)\n", lua_tostring( L, -1 ) ) && lt_addtextvalue( t, (char *)lua_tostring( L, -1 ));
			lt_finalize( t );
		#else
			const char *a = NULL;
			if ( ( a = lua_tostring( L, -1 ) ) )
				lt_addtextvalue( t, a );
			else {
				lt_addtextvalue( t, (char *)"" );
			}	
		#endif
		}
		else if ( vt == LUA_TTABLE ) {
			TELL( " (table at %d)\n", index ); 
			lt_descend( t );
			//tables with nothing should not recurse...
			lua_to_ztable( L, index + 2, t ); 
			lt_ascend( t );
		}
		else if ( vt == LUA_TBOOLEAN ) {
			char *v = lua_toboolean( L, -1 ) ? "true" : "false"; 
			lt_addtextvalue( t, v );
			lt_finalize( t );
		}
	#if 0
		else if ( vt == LUA_TUSERDATA | vt == LUA_TLIGHTUSERDATA ) {
			lua_addudvalue( t, lua_touserdata( L, -1 ) );
			lt_finalize( t ); 
		}
		else if ( vt == LUA_TFUNCTION ) {
			//Somehow we have to inject scope...
			//Then we need to execute
			lua_pcall( L, 1, 1 );
			//We can execute immediately, or wait until the environment is on
			//(or just use a file)  
		}
		else if ( vt == LUA_TNONE | vt == LUA_TNIL ) {

		}
		else if ( vt == LUA_TTHREAD ) {
			fprintf( stderr, "Threads in zTables are unsupported as of yet!" );
			return 0;
		}
		else {
			fprintf( stderr, "Got invalid value in table!" );
			return 0;
		}
	#else
		else {
			fprintf( stderr, "Got invalid value in table!" );
			//return 0;
			char buf[ 1024 ] = {0}, *type = (char *)lua_typename( L, vt );
			snprintf( buf, sizeof( buf ), "%s%s%s", "[[[", type, "]]]" ); 
			lt_addtextvalue( t, buf );
			lt_finalize( t );
		}
	#endif

		lua_pop( L, 1 );
	}
	return 1;
}



//Retrieve a value from a table (and return the index it was found at or -1)
const char * lua_getv ( lua_State *L, const char *key, int index ) {
	lua_pushnil( L );

	for ( int kv, vv; lua_next( L, index ) != 0; ) {
		if ( ( kv = lua_type( L, -2 ) ) == LUA_TSTRING ) {
			if ( strcmp( key, lua_tostring( L, -2 ) )	== 0 && lua_type( L, -1 ) == LUA_TSTRING ) {
				return lua_tostring( L, -1 );
			}
		}
		lua_pop( L, 1 );
	}

	return NULL;
} 
