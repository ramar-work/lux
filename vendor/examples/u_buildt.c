/**
 * @ name
 * ------
 * u_buildt
 *
 * @ synopsis
 * ----------
 * #include "libkirk.h"
 * 
 * <def>
 *
 * @ description
 * -------------
 * Assemble a table.
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

// u_buildt (lua_State *L, int index, int ct, ...)
void 
u_buildt (state_t *state, int index, luab_t *t) 
{
	/* Set the remaining values. */
	int tblct = 0, ct = 1, tmp;
	luab_t *luav;
	luav = t;

	/* cycle */
	while (luav->type) {
		/* Get the key. */
		if (luav->key != NULL)
			lua_pushstring(state->state, luav->key);
		else {
			lua_pushnumber(state->state, ct);
			ct += 1;
		}

		/* Evaluate and save. */
		switch (luav->type)
		{
			case LUA_TSTRING:
				lua_pushstring(state->state, luav->string);
				break;
			case LUA_TNUMBER:
				lua_pushnumber(state->state, luav->number);
				break;
			case LUA_TBOOLEAN:
				lua_pushboolean(state->state, luav->boolean);
				break;
			default:
				break;
		}

		/* Set the key */
		lua_settable(state->state, index);

		/* Increment the pointer */
		luav++;
	}
}
