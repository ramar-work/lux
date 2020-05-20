/*extensions for Lua (db for now)*/
#include "luaext.h"
//#include "lua-db.h"

static const struct luaL_Reg libhypno[] = {
	{ "sql", lua_execdb },
	{ NULL, NULL }
};

int luaopen_libhypno( lua_State *L ) {
	luaL_newlib( L, libhypno );
	return 1;
}

int lua_execdb( lua_State *L ) {
	Table *t;
	sqlite3 *ptr;
	char err[ 2048 ] = {0};
	const char *db_name = lua_tostring( L, 1 );
	const char *db_query = lua_tostring( L, 2 );
#if 0
	fprintf( stderr, "name = %s\n", db_name );
	fprintf( stderr, "query = %s\n", db_query );
#endif
	lua_pop(L, 2);	

	//Try opening something and doing a query
	if ( !( ptr = db_open( db_name, err, sizeof(err) )) ) {
		//This is an error condition and needs to go back
		return lua_end( L, 0, err );
	}

	if ( !( t = db_exec( ptr, db_query, NULL, err, sizeof(err) ) ) ) {
		return lua_end( L, 0, err );
	}

	if ( !db_close( (void **)&ptr, err, sizeof(err) ) ) {
		return lua_end( L, 0, err );
	}

	//Add a table with 'status', 'results', 'time' and something else...
	lua_end( L, 1, NULL );
	lua_pushstring( L, "results" );
	lua_newtable( L );
	if ( !table_to_lua( L, 3, t ) ) {
		//free the table
		//something else will probably happen
		return lua_end( L, 0, "Could not add result set to Lua.\n" );
	}
	lua_settable( L, 1 );
	lt_free( t );
	free( t );	
	return 1;
}

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

