/**
 * @ name
 * ------
 * u_coreset
 *
 * @ synopsis
 * ----------
 * #include "libkirk.h"
 * 
 * <def>
 *
 * @ description
 * -------------
 * Returns a table containing libkirk's standard keys (time, results, status & errv)
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
 **/

//u_coreset (lua_State *state->state, int index, bool_t stat, bool_t errv, char *errv_msg, char *fn)
int
u_coreset (state_t *state, int success, int index, char *errv)
{
	/* Define messages here and clean up the headers */
	char *ERR_V = "errv";
	char *STATUS_V = "status";
	char *TIMESTAMP_V = "timestamp";
	char *UNSPEC_V = "Error unspecified";

	if (!success) {
		/* Wipe any memory that has been allocated. */	
		if (state->allocated && state->memref != NULL) {
			fprintf(stderr, "%s\n", "memref free'ing is happening here.");
			free(state->memref);
		}
			
		/* Always wipe the stack */
		lua_pop(state->state, lua_gettop(state->state));
	}

	/* Add a table */
	lua_newtable(state->state);

	/* Set status = [true, false]	 */
	if (!success) 
		u_setbf(state->state, STATUS_V, index);
	else 
		u_setbt(state->state, STATUS_V, index); 
	
	/* Set error value */
	if (success)
		u_setbf(state->state, ERR_V, index);
	else {
		/* errv = (state->msg || UNSPEC_V) */
		lua_pushstring(state->state, ERR_V);

		if ( errv != NULL )
			lua_pushstring(state->state, errv);
		else {
			if (state->msg != NULL)
				lua_pushstring(state->state, state->msg);
			else
				lua_pushstring(state->state, UNSPEC_V);
		}

		lua_settable(state->state, index);
	}

	/* Add a Unix timestamp */
	u_setts(state->state, TIMESTAMP_V, index);

	/* Return the number of results added */
	return 1;
}
