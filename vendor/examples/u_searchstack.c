/**
* @name
* -----
* ?.c 
* 
* @synopsis
* ---------
* Moves through the stack to get option values.  The most
* inefficient thing I can think of. 
*
* @description
* ------------
* Loops through the stack to find the index of a wanted key.
*
*
**/
luav_t
u_searchstack (lua_State *L, int lt, const char *key)
{
	unsigned int inc, top, pos = 0; 
	luav_t t;
	top = lua_gettop(L);


	/* Loop through the stack */
	for (inc = 1; inc < top; inc += 2)
	{
		if ((lua_type(L, inc) == LUA_TSTRING) && (!strcmp(key, lua_tostring(L, inc))))
			pos = inc + 1;	
	}


	/* If the value is set and of the right type, return it. */
	switch (lt) {
		case LUA_TSTRING:
			// fprintf(stderr, "%s is string.\n", key); 
			if (pos)
				t.string = lua_tostring(L, pos);
			else
				t.string = NULL;
			break;
		case LUA_TNUMBER:
			// fprintf(stderr, "%s is number.\n", key); 
			if (pos)
				t.number = lua_tonumber(L, pos);
			else
				t.number = 0;
			break;
		case LUA_TBOOLEAN:
			// fprintf(stderr, "%s is boolean.\n", key); 
			if (pos)
				t.number = lua_tonumber(L, pos);
			else
				t.number = 0;
			break;
		case LUA_TTABLE:
			// fprintf(stderr, "%s is table.\n", key); 
			if (pos)
				t.number = lua_tonumber(L, pos);
			else
				t.number = 0;
			break;
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
		case LUA_TFUNCTION:
		case LUA_TTHREAD:
			if (pos)
				t.ud = pos;
			else
				t.ud = 0;
			break;
	}

	return t;
}


char *
u_stackstring (state_t *state, char *name)
{
	char *n;
	if (state->error) 
		state->error = 0;

	if ((n = (char *)(u_searchstack(state->state, LUA_TSTRING, name)).string) != NULL)
		return n;
	else {
		state->error = 1;
		return (char *)NULL; 
	}
}


long
u_stacknumber (state_t *state, char *name)
{
	long n; 
	if (state->error) 
		state->error = 0;

	/* Check Lua's code to make sure this isn't it... */
	if (n = (u_searchstack(state->state, LUA_TNUMBER, name)).number)
		return n;
	else {
		state->error = 1;
		return 0;
	}
}


int
u_stackbool (state_t *state, char *name)
{
	int n; 
	if (state->error) 
		state->error = 0;

	if (n = (int)(u_searchstack(state->state, LUA_TNUMBER, name)).number) {
		// fprintf(stderr, "false\n");
		return n;
	}
	else {
		state->error = 1;
		// fprintf(stderr, "true\n");
		return 0;
	}
}


int
u_stackv (state_t *state, char type, char *name)
{
	int n; 
	if (state->error)
		state->error = 0;

	if (n = (int)(u_searchstack(state->state, u_typeint(type), name)).ud) 
	{
		if (state->debug) {
			fprintf(stderr, "Found %s for key %s at position %d.\n",
				u_typename(u_typeint(type)), name, n);	
		}
		return n;
	}
	else {
		state->error = 1;
		return 0;
	}
}


