#include "lib.h"
#include "echo.h"
#include "db.h"
#include "lua.h"

struct lua_fset functions[] = {
	{ "echo", echo_set }
, { "db", db_set }
, { "lua", lua_set }
,	{ NULL }
};


