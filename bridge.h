#include "vendor/single.h"
#include "vendor/http.h"
#include "hypno/lua.h"
#include "hypno/lauxlib.h"
#include "hypno/lualib.h"

#ifdef DEBUG_H
 #define obprintf( f, ... ) fprintf( f, __VA_ARGS__ )
#else
 #define obprintf( f, ... )
#endif

#define PRETTY_TABS( ct ) \
	fprintf( stderr, "%s", &"\t\t\t\t\t\t\t\t\t\t"[ 10 - ct ] );

#if ( LUA_VERSION_NUM >= 503 )
	//#error "Lua version is 5.3"
	#define lrotate( l, i1, i2 ) lua_rotate( l, i1, i2 )
#else
	#define lrotate( l, i1, i2 )
#endif


#define GLOBAL_FN( fn, fn_name ) \
	lua_pushcfunction(L, fn ); \
	lua_setglobal( L, fn_name );


typedef enum 
{
	CC_NONE, 
	CC_MODEL, 
	CC_VIEW, 
	CC_FUNCT,
	CC_STR,
	CC_MAX,
} CCtype;


typedef struct 
{
	CCtype type;      //model or view or something else...	
	char *content;   //Most of the time it's a file, but it really should execute...
#if 0
	void *funct;     //This can be used for userdata (which are just Lua functions here)
#else
	int index;       //This is an alternate method that 
       						 //does not use C to call Lua functions
			
#endif
} Loader;

int table_to_lua (lua_State *, int, Table *);
int lua_to_table (lua_State *, int, Table *);
void lua_loop ( lua_State *L );
int lua_load_file( lua_State *, const char *filename, char **err );
int lua_load_file2( lua_State *, Table *, const char *, char * );
void lua_tdump (lua_State *L);
void lua_stackclear ( lua_State *L );
void lua_stackdump ( lua_State *L );
Loader *parse_route( Loader *, int, HTTP *http, Table *routeTable );
char *printCCtype ( CCtype cc );
int lua_db ( lua_State *L );
int lua_aggregate (lua_State *L);
int abc ( lua_State *L ); 
