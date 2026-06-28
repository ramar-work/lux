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


static void lua_rdumptable( lua_State *L, int i, int d );

struct data {
	unsigned short count;
	unsigned short index;
	unsigned short tinsert;
	unsigned short tpull;
};




/**
 * void lua_rdumpstack( lua_State *L )
 *
 * Dump the Lua stack.
 *
 */
void lua_rdumpstack( lua_State *L ) {
	//	
	for ( int i = 1, top = lua_gettop( L ); i <= top; i++ ) {

		fprintf( stderr, "[%d] => ", i );

		// Get the key first
		if ( lua_type( L, i ) == LUA_TNUMBER )
			fprintf( stderr, "(%s) %lld", lua_typename( L, lua_type( L, i ) ), lua_tointeger( L, i ) );
		else if ( lua_type( L, i ) == LUA_TSTRING )
			fprintf( stderr, "(%s) %s", lua_typename( L, lua_type( L, i ) ), lua_tostring( L, i ) );
		else if ( lua_type( L, i ) == LUA_TFUNCTION )
			fprintf( stderr, "(%s) %p::function", lua_typename( L, lua_type( L, i ) ), lua_topointer( L, i ) );
		else if ( lua_type( L, i ) == LUA_TBOOLEAN )
			fprintf( stderr, "(%s) %s", lua_typename( L, lua_type( L, i ) ), lua_toboolean( L, i ) ? "T" : "F" );
		else if ( lua_type( L, i ) == LUA_TTHREAD )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, i ) ), lua_topointer( L, i ) );
		else if ( lua_type( L, i ) == LUA_TLIGHTUSERDATA || lua_type( L, i ) == LUA_TUSERDATA )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, i ) ), (void *)lua_touserdata( L, i ) );
		else if ( lua_type( L, i ) == LUA_TNIL || lua_type( L, i ) == LUA_TNONE )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, i ) ), lua_topointer( L, i ) );
		else if ( lua_type( L, i ) == LUA_TTABLE ) {
			fprintf( stderr, "(%s) %p {\n", lua_typename( L, lua_type( L, i ) ), lua_topointer( L, i ) );
			lua_rdumptable( L, i, 1 );
			fprintf( stderr, "}" );
		}
		else {
			const char *typename = lua_typename( L, lua_type( L, i ) );
			FPRINTF( "Got invalid key type: %s in table at index %d\n", typename, i );
			return;
		}

		fprintf( stderr, "\n" );

	}
}



/**
 * static void lua_rdumptable( lua_State *L, int i, int ind )
 *
 * Recursive dump of Lua tables that may be on the stack.
 *
 */
static void lua_rdumptable( lua_State *L, int i, int ind ) {

	//Add this entry to start iteration through table.	
	lua_pushnil( L );

	// Push the next two values from the table at $src index.
	for ( ; lua_next( L, i ) != 0; ) {

		//Print any spaces
		for ( int ii = 0; ii < ind; ii++ ) fprintf( stderr, "%s", "  " );

		// Get the key first
		if ( lua_type( L, -2 ) == LUA_TNUMBER ) /* Notice that the keys are always integers */
			fprintf( stderr, "(%s) %lld", lua_typename( L, lua_type( L, -2 ) ), lua_tointeger( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TSTRING )
			fprintf( stderr, "(%s) %s", lua_typename( L, lua_type( L, -2 ) ), lua_tostring( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TFUNCTION )
			fprintf( stderr, "(%s) %p::function", lua_typename( L, lua_type( L, -2 ) ), lua_topointer( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TBOOLEAN )
			fprintf( stderr, "(%s) %s", lua_typename( L, lua_type( L, -2 ) ), lua_toboolean( L, -2 ) ? "T" : "F" );
		else if ( lua_type( L, -2 ) == LUA_TTHREAD )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, -2 ) ), lua_topointer( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TLIGHTUSERDATA || lua_type( L, -2 ) == LUA_TUSERDATA )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, -2 ) ), (void *)lua_touserdata( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TNIL || lua_type( L, -2 ) == LUA_TNONE )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, -2 ) ), lua_topointer( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TTABLE )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, -2 ) ), lua_topointer( L, -2 ) );
		else {
			const char *typename = lua_typename( L, lua_type( L, -2 ) );
			FPRINTF( "Got invalid key type: %s in table at index %d\n", typename, i );
			return;
		}

		// Print a seperator
		fprintf( stderr, " => " );

		// Then get whatever values
		int WW = -1;
		if ( lua_type( L, WW ) == LUA_TNUMBER )
			fprintf( stderr, "(%s) %f", lua_typename( L, lua_type( L, WW ) ), lua_tonumber( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TSTRING )
			fprintf( stderr, "(%s) %s", lua_typename( L, lua_type( L, WW ) ), lua_tostring( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TFUNCTION )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, WW ) ), lua_topointer( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TBOOLEAN )
			fprintf( stderr, "(%s) %s", lua_typename( L, lua_type( L, WW ) ), lua_tostring( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TTHREAD )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, WW ) ), lua_topointer( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TLIGHTUSERDATA || lua_type( L, WW ) == LUA_TUSERDATA )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, WW ) ), (void *)lua_touserdata( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TNIL || lua_type( L, WW ) == LUA_TNONE )
			fprintf( stderr, "(%s) %p", lua_typename( L, lua_type( L, WW ) ), lua_topointer( L, WW ) );
		else if ( lua_type( L, WW ) == LUA_TTABLE ) {
			fprintf( stderr, "(%s) %p {\n", lua_typename( L, lua_type( L, WW ) ), lua_topointer( L, i ) );
			++ind;
			lua_rdumptable( L, lua_gettop( L ), ind );
			--ind;
			//Print any spaces
			for ( int ii = 0; ii < ind; ii++ ) fprintf( stderr, "%s", "  " );
			fprintf( stderr, "}" );
		}
		else {
			const char *typename = lua_typename( L, lua_type( L, -1 ) );
			FPRINTF( "Got invalid value type: %s in table at index %d\n", typename, i );
			return;
		}

		// New line for purtiness
		fprintf( stderr, "\n" );

		// Pop and move to the next key
		lua_pop( L, 1 );
	}	

	return;
}




/**
 * int lua_rmerge (lua_State *L, int dest, int src, char *err, int errlen )
 *
 * Recursive version of Lua merge.
 *
 */
int lua_rmerge ( lua_State *L, int dest, int src, char *err, int errlen ) {

	//Maybe get the top one time
	int ltop = lua_gettop( L );

	//Assumes that the user is adding a table, and probably shouldn't
	if ( ltop < 2 ) {
		snprintf( err, errlen, "Nothing to merge.  Aborting...\n" );
		return 0;
	}

	//Check if destination or source indices are out of range
	if ( dest > ltop || src > ltop ) {
		const char fmt[] = "%s table index out of range.  Aborting...\n";
		snprintf( err, errlen, fmt, ( dest > ltop ) ? "Destination" : "Source" );
		return 0;
	}

	//Check that at least two more entries can be added to the stack
	if ( !lua_checkstack( L, 2 ) ) {
		snprintf( err, errlen, "Exhausted Lua stack space.  Aborting...\n" );
		return 0;
	}

	//Add this entry to start iteration through table.	
	lua_pushnil( L );

	// Push the next two values from the table at $src index.
	for ( ; lua_next( L, src ) != 0; ) {
		// Get the key first
		if ( lua_type( L, -2 ) == LUA_TNUMBER ) /* Notice that the keys are always integers */
			lua_pushnumber( L, lua_tointeger( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TSTRING )
			lua_pushstring( L, lua_tostring( L, -2 ) );
		else {
			const char *typename = lua_typename( L, lua_type( L, -2 ) );
			snprintf( err, errlen, "Got invalid key type: %s at table\n", typename );
			return 0;
		}

		// Then get whatever values
		if ( lua_type( L, -2 ) == LUA_TNIL || lua_type( L, -2 ) == LUA_TNONE )
			lua_pushnil( L ); // This should come out as 'null'
		else if ( lua_type( L, -2 ) == LUA_TNUMBER )
			lua_pushnumber( L, lua_tonumber( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TBOOLEAN )
			lua_pushboolean( L, lua_toboolean( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TSTRING )
			lua_pushstring( L, lua_tostring( L, -2 ) );
		else if ( lua_type( L, -2 ) == LUA_TTABLE ) {
			int t = lua_gettop( L ) - 1;
			if ( !lua_checkstack( L, 1 ) ) {
				snprintf( err, errlen, "Exhausted Lua stack space when descending\n" );
				return 0;
			}
			lua_newtable( L );
			if ( !lua_rmerge( L, t + 2, t, err, errlen ) ) {
				return 0;
			}
		}
		#if 0
		// TODO: We could use the byte representation of other types
		// but this doesn't actually seem all that helpful
		#endif
		else {
			const char *typename = lua_typename( L, lua_type( L, -1 ) );
			snprintf( err, errlen, "Got invalid value type: %s at table\n", typename );
			return 0;
		}

		// Set the table values
		lua_settable( L, dest );
			
		// Then pop them
		lua_pop( L, 1 );
	}	

	return 1;
}



/**
 * int lua_exec_file( lua_State *L, const char *f, char *err, int errlen )
 *
 * luaL_loadfile( ... ) with error handling.
 *
 */
int lua_exec_file( lua_State *L, const char *f, char *err, int errlen ) {
	int len = 0, lerr = 0;

	if ( !f || !strlen( f ) ) {
		snprintf( err, errlen, "%s", "No filename supplied to load or execute." );
		return 0;
	}

#if 0
	struct stat check;
	//Since this is supposed to accept a file, why not just check for existence?
	if ( stat( f, &check ) == -1 ) {
		snprintf( err, errlen, "File %s inaccessible: %s.", f, strerror(errno) );
		return 0;
	}

	// This was an error, but now it shouldn't be
	if ( check.st_size == 0 ) {
		snprintf( err, errlen, "File %s is zero-length.  Nothing to execute.", f );
		return 0;
	}
#endif

	//Load the string, execute
	if (( lerr = luaL_loadfile( L, f )) != LUA_OK ) {
		if ( lerr == LUA_ERRSYNTAX )
			len = snprintf( err, errlen, "Syntax error: " );
		else if ( lerr == LUA_ERRMEM )
			len = snprintf( err, errlen, "Memory allocation error: " );
	#ifdef LUA_53
		else if ( lerr == LUA_ERRGCMM )
			len = snprintf( err, errlen, "GC meta-method error: " );
	#endif
		else if ( lerr == LUA_ERRFILE )
			len = snprintf( err, errlen, "File access error: " );
		else {
			len = snprintf( err, errlen, "Unknown: "  );
		}
	
		errlen -= len;	
		snprintf( &err[ len ], errlen, "%s\n", (char *)lua_tostring( L, -1 ) );
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


	

/**
 * int ztable_to_lua ( lua_State *L, ztable_t *t )
 *
 * Copy all records from a ztable_t to a Lua table at any point in the stack.
 *
 */
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



/**
 * int lua_count ( lua_State *L, int i )
 *
 * Counts the elements in a table.
 *
 */
int lua_count ( lua_State *L, int i ) {
	int count = 0;

	if ( !lua_istable( L, i ) ) {
		//TODO: Needs to throw an exception instead
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



/**
 * int lua_retglobal( lua_State *L, const char *key, int type )
 *
 * Attempts to retriev e a key from a global table and clears the stack if a match is not found.
 *
 */
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


/**
 * int lua_to_ztable ( lua_State *L, int index, ztable_t *t )
 *
 * Convert Lua tables to ztable_t.
 *
 */
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



/**
 * const char * lua_getv ( lua_State *L, const char *key, int index )
 *
 * Locate a value in a table and return the index in which it was found.
 * No match == -1.
 *
 */
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
