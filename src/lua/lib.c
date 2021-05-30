#include "lib.h"
#include "echo.h"
#include "db.h"

struct lua_fset functions[] = {
	{ "echo", echo_set }
, { "db", db_set }
,	{ NULL }
};


