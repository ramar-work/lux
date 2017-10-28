#include "vendor/single.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef DEBUG_H
 #define obprintf( f, ... ) fprintf( f, __VA_ARGS__ )
#else
 #define obprintf( f, ... )
#endif 

int table_to_lua (lua_State *, int, Table *);
int lua_to_table (lua_State *, int, Table *);
void lua_loop ( lua_State *L );
int lua_load_file( lua_State *, const char *filename, char *err );
int lua_load_file2( lua_State *, Table *, const char *, char * );
void lua_tdump (lua_State *L);
