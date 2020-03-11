/*extensions for Lua (db for now)*/
#include "luaext.h"

int lua_db ( lua_State *L ) {
	//If argument is a string, send it to the database
	//If argument is a table, send it somewhere else
	//If argument count is > 1, send an error or exception
	
	//Open a table ( too bad it can't be static ( or can it ) )
	return 0;
}
