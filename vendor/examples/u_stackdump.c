/**
 * @ name
 * ------
 * u_mprint_stack
 * u_qprint_stack
 *
 * @ synopsis
 * ----------
 * #include "libkirk.h"
 * 
 * void u_mprint_stack (lua_State *L, char *msg);
 *
 * @ description
 * -------------
 * Identical to u_vprint_stack(), with the exception of 
 * a pause at the end of traversal. 
 *
 * @ examples
 * ----------
 * 
 *
 * @ caveats
 * ---------
 * 
 *
 * @ return_values
 * ---------------
 * 
 *
 * @ errors
 * --------
 * 
 *
 * @ author
 * --------
 * <author>
 *
 * @ license
 * ---------
 * <mit>
 *
 * @ see_also
 * ----------
 * 
 *
 * @ section
 * ---------
 * 3
 *
 * @ tests
 * -------
 * 
 *
 * @ todo
 * ------
 * 
 *
 * @ end
 * -----
 * 
 *
 **/

void
u_mprint_stack (lua_State *L, const char *msg)
{
	int i, top;
	top = lua_gettop(L);

	if (msg != NULL)
		fprintf(stderr, "%s\n", msg);

	for (i = 1; i <= top; i++) {
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:
				printf("%d: %s [%s]\n", i, lua_tostring(L, i), lua_typename(L, t));
				break;		
			case LUA_TBOOLEAN:
				if (lua_toboolean(L, i))
					printf("%d: true [%s]\n", i, lua_typename(L, t));
				else
					printf("%d: false [%s]\n", i, lua_typename(L, t));
				break;		
			case LUA_TNUMBER:
				printf("%d: %g [%s]\n", i, lua_tonumber(L, i), lua_typename(L, t));
				break;
			default: 
				printf("%d: %s\n", i, lua_typename(L, t));
		}
	}

	/* Pause */
	getchar();
}


void
u_mprint (char *msg)
{
	fprintf(stderr, "%s\n", msg);
	getchar();
}


void
u_qprint_stack (lua_State *L, const char *msg)
{
	int i, b, top;
	top = lua_gettop(L);

	if (msg != NULL)
		fprintf(stderr, "%s\n", msg);

	for (i = 1; i <= top; i++) {
		int t = lua_type(L, i);
		switch (t) {
			case LUA_TSTRING:
				printf("%d: %s [%s]\n", i, lua_tostring(L, i), lua_typename(L, t));
				break;		
			case LUA_TBOOLEAN:
				if (lua_toboolean(L, i))
					printf("%d: true [%s]\n", i, lua_typename(L, t));
				else
					printf("%d: false [%s]\n", i, lua_typename(L, t));
				break;		
			case LUA_TNUMBER:
				printf("%d: %g [%s]\n", i, lua_tonumber(L, i), lua_typename(L, t));
				break;
			default: 
				printf("%d: %s\n", i, lua_typename(L, t));
		}
	}
}
