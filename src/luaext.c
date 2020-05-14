/*extensions for Lua (db for now)*/
#include "luaext.h"


#if 0
//err
void l_buildtable (state_t *state, int index, luab_t *t) {
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


void l_gettable (state_t *state, int index, luab_t *t) {

}
#endif


int lua_db ( lua_State *L ) {
	//If argument is a string, send it to the database

	//If argument is a table, send it somewhere else

	//If argument count is > 1, send an error or exception
	
	//Open a table

	//Execute whatever

	//Close it

	//Return the status in a table.  (and free the original)
	return 0;
}
