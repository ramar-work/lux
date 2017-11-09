int
u_setresults (state_t *state, luab_t *t)
{ 
	/* Define */
	int index; 

	/* Push the results. */
	lua_pushstring(state->state, "results");
	lua_newtable(state->state);
	index = lua_gettop(state->state);
	u_buildt(state, index, t);

	/* Return the set index in case you're not done. */
	return index;	
}
