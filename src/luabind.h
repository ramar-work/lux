#include "../vendor/single.h"

#define PRETTY_TABS( ct ) \
	fprintf( stderr, "%s", &"                    "[ 20 - ct ] );

#if 1
 //This is to handle system Lua or whatever other libraries may be needed.
 #include <lua.h>
 #include <lauxlib.h>
 #include <lualib.h>
#else
 #include <lua.h>
 #include <lauxlib.h>
 #include <lualib.h>
#endif

	#define lrotate( l, i1, i2 ) lua_rotate( l, i1, i2 )
#if 0
#if ( LUA_VERSION_NUM >= 503 )
	//#error "Lua version is 5.3"
	#define lrotate( l, i1, i2 ) lua_rotate( l, i1, i2 )
#else
	#define lrotate( l, i1, i2 )
#endif
#endif


//Regular Lua utilities
void lua_stackdump (lua_State *); 
//void lua_stackclear (lua_State *);
int lua_aggregate (lua_State *L);
int lua_combine (lua_State *L, char *err, int errlen);

//Universal hash table utilities
int table_to_lua (lua_State *, int, Table *) ;
int lua_to_table (lua_State *, int, Table *);
int lua_exec_string( lua_State *L, const char *, char *e, int elen );
int lua_exec_file( lua_State *L, const char *, char *e, int elen );
