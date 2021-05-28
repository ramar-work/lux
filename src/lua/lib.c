#include "lib.h"
//#include "echo.h"
#include "db.h"

struct lua_fset functions[] = {
#if 0
	{ "echo", echo_set }
#endif
  { "db", db_set }
,	{ NULL }
};


