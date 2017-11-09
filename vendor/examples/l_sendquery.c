/**
 * @ name
 * ------
 * l_sendquery
 *
 * @ synopsis
 * ----------
 * #include "libkirk.h"
 * 
 * int l_sendquery (lua_State *L);
 *
 * @ description
 * -------------
 * 
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

int 
l_sendquery (lua_State *L)
{
	// Initialize the connection scaffolding from here.
	sqlite3 *db = NULL;

	// Get and check stuff from here.
	const char *db_path, *qqry;
	db_path = luaL_checkstring(L, 1); 
	qqry = luaL_checkstring(L, 2);
	lua_pop(L, 2);
	// printf("%s, %s\n", db_path, qqry);

	// Open the connection.
	int oc = sqlite3_open(db_path, &db); 
	if ( oc != SQLITE_OK ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return LSQLITE_ERR_ON_OPEN;
	}

	// Do any parsing before anything else. 
	// parse(qqry);

	// Compile the query, also checking for correctness 
	int pc;
	sqlite3_stmt *stmt;
	const char *tail;
	pc = sqlite3_prepare_v2(db, qqry, -1, &stmt, &tail);
	if ( pc != SQLITE_OK ) {
		fprintf(stderr, "Could not execute query: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

	// Initialize and set up columns.
	int i, sx, cc, qc;
	cc = sqlite3_column_count(stmt);
	const char *cnames[cc];
	sx = 1;
	for (i=0; i<cc; i++) // {
		cnames[i] = sqlite3_column_name(stmt, i);
		// printf("%s\n", cnames[i]);
	//}

	// Grow the stack for result set.
	int stack_alloc, stack_max;
	stack_alloc = (cc*2);
	stack_max   = lua_gettop(L);
	if ( stack_alloc > stack_max ) //{
		lua_checkstack(L, ((stack_alloc - stack_max) + 2));
		// +2 - I need one for results, and another for my "verbose" table 
	//}

	// Make a source table.
	// l_mktables(L, 2);
	lua_newtable(L);
	lua_newtable(L);

	// 
	struct timeval qstart, qfin;

	// Retrieve data
	gettimeofday(&qstart, NULL);
	while ( qc != SQLITE_DONE ) {
		qc = sqlite3_step(stmt);
		if ( qc == SQLITE_ROW ) {
			// Allocations and saves. 
			lua_newtable(L);
			for (i=0; i<cc; i++) {
				printf("%s: %s\n", cnames[i], sqlite3_column_text(stmt, i));
				// Always allocate space.	
				lua_pushstring(L, cnames[i]);
				lua_pushstring(L, (const char *)sqlite3_column_text(stmt, i)); 
				lua_settable(L, 3);
			}
		
			// Ugly...
			char nn[sx];		// <-- Address this at some point...
			snprintf(nn, sx, "%d", sx);
			lua_setfield(L, 2, nn); 
			// printf("%d\n", sx);
			sx = sx + 1;
		} 
		// Handle other codes...
		else {
		}
	}

	/* ALL THIS NEEDS TO BE WRAPPED.  Exceptions need this logic as well... */

	// Clean up and get some statistics.
	int rows_aff; 
	gettimeofday(&qfin, NULL);
	rows_aff = sqlite3_changes(db);	
	sqlite3_close(db);

	// Place all results of this one call here.
	lua_setfield(L, 1, "__results");

	// Rows affected are needed.
	lua_pushstring(L, "__affected");
	lua_pushnumber(L, rows_aff);
	lua_settable(L, 1);

	// True or false? (can depend on many things)
	lua_pushstring(L, "__status");
	lua_pushboolean(L, 1); // 0 = false, 1 = true, 
								  // Damn you Lua and your non-Posix wiles!
	lua_settable(L, 1);

	// Final query time.	
	int et;
	int pwr = 1000000;
	et = (((qfin.tv_sec * pwr)+ qfin.tv_usec) - ((qstart.tv_sec * pwr ) + qstart.tv_usec));	

	/*
	printf("%d microseconds\n", (et / (float)(1000000)));
	printf("%.7f milliseconds\n", et);
	printf("%.7f centiseconds\n", et);
	fprintf(stdout, "Query completed in %.7f seconds.\n", (float)(et)/(float)(1000000));
	*/

	lua_pushstring(L, "__exectime");
	lua_pushnumber(L, et);
	lua_settable(L, 1);

	// Debugs stack issues...
	// nakify(L);

	// Return the number of results to Lua.
	return 1;
};
