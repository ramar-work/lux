#include "vendor/single.h"
#include "vendor/http.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef DEBUG_H
 #define obprintf( f, ... ) fprintf( f, __VA_ARGS__ )
#else
 #define obprintf( f, ... )
#endif 

typedef struct 
{
	int   type;      //model or view or something else...	
	char *content;   //Most of the time it's a file, but it really should execute...
#if 0
	void *funct;     //This can be used for userdata (which are just Lua functions here)
#else
	int index;       //This is an alternate method that 
       						 //does not use C to call Lua functions
			
#endif
} Loader;



typedef enum 
{
	CC_NONE, 
	CC_MODEL, 
	CC_VIEW, 
	CC_FUNCT,
	CC_STR,
	CC_MAX,
} CCtype;



int table_to_lua (lua_State *, int, Table *);
int lua_to_table (lua_State *, int, Table *);
void lua_loop ( lua_State *L );
int lua_load_file( lua_State *, const char *filename, char *err );
int lua_load_file2( lua_State *, Table *, const char *, char * );
void lua_tdump (lua_State *L);
void lua_stackdump ( lua_State *L, int *p, int *sd );

Loader *parse_route( Loader *, int, Table *src, Table *route );
char *printCCtype ( CCtype cc );
