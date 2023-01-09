/* response.c */
/* -------------------------------------------- * 
response
========


 * -------------------------------------------- */
#include "response.h"


int noop( lua_State *L ) {
	return 0;
}


struct luaL_Reg response_set[] = {
 { "send", noop }
,{ "failure", noop }
,{ "success", noop }
,{ NULL }
};
