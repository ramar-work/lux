/* --------------------------------------------- *
 * echo.c
 * ------
 *
 * All functions take an argument that matches
 * the function name. (e.g. echo.string takes 
 * a string, and so forth).  If the argument
 * does not match, die.
 * --------------------------------------------- */
#include "echo.h"
#include "lua.h"

int echo_numeric_arg ( lua_State *L ) {
	const int arg = 1;
#if 1
	int a = luaL_checknumber( L, arg );
#else
	if ( !lua_isnumber( L, arg ) ) {
		return luaL_error( L, "echo.number: argument %d not a number.", arg ); 
	}

	int a = lua_tonumber( L, 1 );
	lua_pop( L, 1 );
#endif
	lua_pushnumber( L, a );
	return 1;
}


int echo_string_arg ( lua_State *L ) {
	const int arg = 1;
	const char f[] = "echo.string";
#if 1
	const char *a = luaL_checkstring( L, 1 );
#else
	if ( !lua_isstring( L, arg ) ) {
		return luaL_error( L, "%s: argument %d not a number.", f, arg ); 
	}
	const char * a = lua_tostring( L, arg );
#endif
	return 1;
}


int echo_table_arg ( lua_State *L ) {
	const int arg = 1;
#if 1
	luaL_checktype( L, arg, LUA_TTABLE );
	//have to convert the table here (if you want...)
#else
#endif
	return 0;
}

struct luaL_Reg echo_set[] = {
	{ "string", echo_string_arg }
,	{ "number", echo_numeric_arg }
,	{ "table", echo_table_arg }
,	{ NULL }
};


