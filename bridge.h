#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "vendor/single.h"

#ifdef DEBUG_H
 #define obprintf( f, ... ) fprintf( f, __VA_ARGS__ )
#else
 #define obprintf( f, ... )
#endif 

int table_to_lua (lua_State *, int, Table *);
int lua_to_table (lua_State *, int, Table *);
void lua_loop ( lua_State *L );
