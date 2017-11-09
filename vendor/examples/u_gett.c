/**
 * @ name
 * ------
 * u_gett
 *
 * @ synopsis
 * ----------
 * #include "libkirk.h"
 * 
 * void u_gett (lua_State *L, fm_t *fm)
 *
 * @ description
 * -------------
 * Gets keys from a table and place their indices within a struct.
 *
 * @ usage
 * -------
 * A call to u_gett will almost always be preceded with the 
 * initialization of an fm_t data structure.  
 *
 * The structure 'fm_t' defines all metadata for the function
 * you're writing an interface for.
 *
 * @ examples
 * ----------
 * Here is an example showing a fully filled out fm_t structure
 * before being passed into u_gett():
 * 
 * // We define all of the possible options for our function here:
 * om_t opt[] = {	
 * 		{ .name = "name", .strict = "s" }
 * 		{ .name = "callsign", .strict = "s" }
 * 		{ .name = "vehicle", .strict = "s" }
 * 		{ .name = "driver", .strict = "s" }
 * };
 * 
 * // Then we specify some parameters to describe the function.	
 * fm_t fd = {
 * 	.fname      = "make_gamecar",	// specify function name 
 * 	.discard    = "n",            // discard any number values
 * 	.accept_mt  = FALSE,          // never process metatables 
 * 	.accept_all = FALSE,          // only save keys specified in opt[] 
 * 	.expects    = T_ALPHA,        // discards numeric keys	
 * 	.index      = 1,              // table should be at index 1
 * 	.err_type   = E_CRIT,         // any error will result in exception
 * 	.max_key_l  = 1024,           // keys can be no longer than 1024 characters 
 * 	.nopts      = 4,	            // number of options given. 
 * 	.options    = opt             // reference to options. 
 * };
 *
 * // Finally, we just call u_gett with the address of fm_t
 * u_gett(error, &fd);
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



/* Need to return the number of items on the stack, I think... or  */
int
u_gett (state_t *state, fm_t *fm)
{
	/* Defines */	
	int tmp, optct = 0, k = 2, v = 3, fail = 0, stat = 0;  /* k & v are stack indexes */ 
	char discard[LUA_TYPES] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	char strict[LUA_TYPES] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 }; 
	om_t  *ot, oc;
	int *set = NULL;

	/* Options */
	struct {
		char *name, *key, *value, *keyname;
		int  index, type, keytype, valuetype, keyindex, set: 1;
	} opt;
	
	/* Define a module map for this function. */
	state->flimsy = USE_EXCEPTION;
	state->size = (sizeof(EM(util))/sizeof(err_t));
	state->map = EM(util);

	/* Set all defaults for the definition table */
	if (fm != NULL) {
		/* Move through and set all discard types */		
		if (fm->discard != NULL)
			u_settypes(fm->discard, discard);

		/* We will always pop the table for now... */
		fm->pop_table = 1;

		/* Set all of the must have values for popping to work */	
		if (!fm->index) {
			if (lua_type(state->state, 1) == LUA_TTABLE) 
				fm->index = 1;
			else
				return u_err(ERR_EXPECTED_INDEX);
		}

		/* Count all the options */	
		if (fm->options) 
		{
			/* Define a spot for it */
			ot = &(*fm->options);
			while (ot->name != NULL) {
				if (state->debug)
					fprintf(stderr, "Option %d: %s\n", optct, ot->name);
				optct += 1;
				ot++;
			}

			/* Option count */
			if (state->debug)
				fprintf(stderr, "Got %d options at function %s\n", optct, state->fname);

			/* Reset the options */
			ot = &(*fm->options);
		}
		else {
			optct = 0;
		}

		/* Allocate space for a stack */
		set = calloc(optct, sizeof(int));
		if (set == NULL) {
			fprintf(stderr, "Setting space for options failed.\n");
			state->msg = strerror(errno);
			return 0;	
		}

		/* Max hashable string length */
		if (!fm->max_key_l)
			fm->max_key_l = MAX_HASHABLE_STRING_LENGTH;

		/* What type of table should come through? */
		if (!fm->expects)
			fm->expects = T_MIXED;

		/* How deep should we attempt to go? */
		if (!fm->max_depth) 
			fm->max_depth = T_DEPTH;
	}

	/* Prepare list and evaluate counts */
	lua_pushnil(state->state);

	/* Loop through all values in a table */
	while (lua_next(state->state, fm->index) != 0)
	{
		/* Print stack */
		if (state->debug)
			u_qprint_stack(state->state, "State at top of key evaluation.");
		/* Pop and check the type of key */
		switch (opt.keytype = lua_type(state->state, -2))
		{
			/* Debug stack */
			if (state->debug) {
				fprintf(stderr, "type: %d %s\n",
					opt.keytype, lua_typename(state->state, opt.keytype));
			}		

			case LUA_TSTRING:
				opt.keyname = (char *)lua_tostring(state->state, -2);
				if (fm->expects == T_NUMBER)
				{
					/* Depends on error handler */
					if (state->flimsy)
						return u_err(ERR_EXPECTED_NUMERIC, "n", "s"); 
					else {
						fprintf(stderr, "%s '%s'\n", 
							"Numeric value expected at key", opt.keyname); 
						lua_pop(state->state, 1);
						continue;
					}
				}

				/* Show me more information */
				if (state->debug)
					fprintf(stderr, "%s ->\n", opt.keyname);
				break;

			case LUA_TNUMBER:
				opt.keyindex = lua_tonumber(state->state, -2);
				if (fm->expects == T_ALPHA) 
				{
					/* Depends on error handler */
					if (state->flimsy)
						return u_err(ERR_EXPECTED_ALPHA, opt.keyindex); 
					else {
						fprintf(stderr, "%s [%d]\n", "String value expected. Got numeric key", opt.keyindex); 
						lua_pop(state->state, 1);
						continue;
					}
				}

				/* Show me more information */
				if (state->debug)
					fprintf(stderr, "%d ->\n", opt.keyindex);

				break;
			default:
				break;
		}

		/* Calculate types */
		opt.valuetype = lua_type(state->state, -1);

		/* State after value type sniff */
		if (state->debug)
			u_qprint_stack(state->state, "State before discard type check.");

		/* Check for global discardable keys here */ 
		if (discard[opt.valuetype])
		{
			/* Give me the type of this */
			if (state->debug)
				fprintf(stderr, "Value is type [%s]\n", u_typename(opt.valuetype));

			/* Again depends on error type */
			if (state->flimsy)
				return u_err(ERR_UNEXPECTED_TYPE, v, opt.keyname);
			else {
				/* An error occurred, you ought to log it */
				u_log(ERR_UNEXPECTED_TYPE, v, opt.keyname);

				/* Always pop */
				lua_pop(state->state, 1);

				/* Continue */
				continue;
			}
		}

		/* Set up before loop */
		opt.set = 0;

		/* Find the option key */
		if (fm->options && opt.keyname != NULL)
		{ 
			/* Check through each option */
			for (tmp = 0; tmp < optct; tmp++) {
				/* Set a reference for ease */
				oc = fm->options[tmp];

				/* Show me the options as we cycle through */
				if (state->debug)
					fprintf(stderr, "Option %d: %s\n", tmp, oc.name);

				/* Compare the strings */
				if (strcmp(opt.keyname, oc.name) != 0)
					continue;
				else
					opt.set = 1;
					
				/* Strict type check */
				if (oc.strict != NULL) {
					/* Set strict array */
					u_settypes(oc.strict, strict);
					
					/* Check if they match or not */	
					if (!strict[opt.valuetype]) {
						if (state->flimsy)
							return u_err(ERR_STRICT_TYPECHECK, opt.keyname, oc.strict);
						else {
							u_qprint_stack(state->state, "What is it?");
							continue;
						}
					}
				}


				/* Validation check */
				if (0 == 1) {
					/* Run the validator function */
					/* ... */

					/* If it fails, pop value */
					if (state->flimsy)
						return u_err(ERR_VALIDATION);
					else {
						fs("Validator failed.  Skipping.");
						lua_pop(state->state, 2);
						continue;
					}
				}

				/* Type handlers - all function pointers */
				if (0 == 1) {
					if (fm->on_table != NULL) ;
					if (fm->on_numeric != NULL) ;
					if (fm->on_alpha != NULL) ;
						return u_err(ERR_TYPE_HANDLER);
				}
				
				/* Check if it's required */
				if (oc.required && set != NULL)
					set[tmp] = 1;

				break;
			}
		}

		/* Die on bad exclusive checks */
		if ((fm->exclusive) && (!opt.set))
		{
			lua_pop(state->state, 2);

			if (state->flimsy)
				return u_err(ERR_NOT_MEMBER_KEY, opt.keyname);
			else {
				fs("The key is not a member key.");
				continue;
			}
		}

		/* Allocate Lua VM stack space, this is always a fatal error. */ 
		if (!lua_checkstack(state->state, 1))
			return u_err(ERR_STACK_ALLOC);

		/* Pop key from the stack and copy values */
		lua_pushvalue(state->state, k); 
		lua_pushvalue(state->state, v);

		/* Increment the copy indices */
		/* k = v += 2; */
		k += 2;
		v += 2;

		/* Always pop the last key */
		lua_pop(state->state, 1);

		/* Show what's on the stack again. */
		if (state->debug)
			u_qprint_stack(state->state, "Copied last key to not confuse next."); 

		/* Reset the options */
		opt.name = NULL;
		opt.key = NULL;
		opt.value = NULL;
		opt.keyname = NULL;
		opt.index = 0;
		opt.type = 0;
		opt.keytype = 0;
		opt.valuetype = 0;
		opt.keyindex = 0;
		opt.set = 0;
	}

	/* Check required keys */
	for (tmp = 0; tmp < optct; tmp++) {
		/* Set a reference for ease */
		oc = fm->options[tmp];

		/* Debug printing  */
		if (state->debug) {
			printf("Name:     %s\n", oc.name);
			printf("Strict:   %s\n", oc.strict);
			printf("Table:    %d\n", oc.table);
			printf("Required: %d\n", oc.required);
			printf("Set?:     %d\n", set[tmp]);
			printf("Found:    %d\n", oc.found);
			printf("Type:     %s\n", u_typename(oc.type));
		}

		/* Was the option required */
		if (oc.required && !set[tmp]) {
			free(set);
			return u_err(ERR_REQD_KEY_UNSPEC, oc.name);
		}
	}

	/* Remove the source table. */	
	if (fm->pop_table)
		lua_remove(state->state, fm->index);	

	/* Free it */
	free(set);

	/* Success code */
	return 1;
}
