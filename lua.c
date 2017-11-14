/*

Lua
===

x open interpreter
x cast and convert http structure
- put all of env into environment
- use the JSON or comma seperated route handler
	data.json
- add the functions that are needed by models
-	add database connector functions
- add session handler and memory functions
- any other things are made with either lua or tables...
x create a hash table depending on size of the model
- convert Lua to table (you could write the render module there too... but it sucks...) 
- free the Lua state

*/
#if 0
 //How to check for Lua version numbers
 #if LUA_VERSION_NUM == 503
	 fprintf( stderr, "%s\n", "You're running Lua 5.3" );
 #elif LUA_VERSION_NUM == 501
	 fprintf( stderr, "%s\n", "You're running Lua 5.1" );
 #endif
#endif

#include "single.h"
#include <lua.h>
#include <lauxlib.h>
#include <liblua.h>

#if 1
static char errbuf[1024] = {0};
static int counter = 1;
static char *errmsg = 0;
static sqlite3 *db = NULL;
static int lload_db (const char *name);
static int lexec_db (lua_State *L, const char *query);
static int exec_db (lua_State *L);
static int schema_db (lua_State *L);
static int remove_db (lua_State *L);
static int check_table (lua_State *L);
static int callback(void *nu, int argc, char **argv, char **cn);
static int get_schema(void *nu, int argc, char **argv, char **cn);

/* Errors and whatnot */
#define lua_return(mybool, errmsg) \
 while (1) { \
	if (!mybool) { lua_pop(L, lua_gettop(L)); lua_newtable(L); } \
	lua_pushstring(L, "status"); \
	lua_pushboolean(L, mybool); \
	lua_settable(L, 1); \
	lua_pushstring(L, "error"); \
	lua_pushstring(L, (errmsg == NULL) ? "No error" : errmsg); \
	lua_settable(L, 1); \
	if (!mybool) return 1; break; }

#endif

static inline int lload_db (const char *name) {
	int rc;
	if ((rc = sqlite3_open(name, &db))) {
		snprintf(errbuf, 1023, "Can't open database: %s.", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 0;	
	}
	return 1;
}


static inline int lexec_db (lua_State *L, const char *query) {
	int rc; 
	/* Protection against stupidity */
	/* Execute all SQL (this will do more than one query) */
	if ((rc = sqlite3_exec(db, query, callback, (void *)L, &errmsg)) != SQLITE_OK) {
		snprintf(errbuf, 1023, "Failed to execute SQL query: %s.", errmsg);
		sqlite3_free(errmsg);
		return 0;
	}
	return 1;
}

static int callback(void *nu, int argc, char **argv, char **cn) {
	lua_State *L = (lua_State *)nu;
	int i;
	lua_pushnumber(L, counter); // 4
	lua_newtable(L); // 5
	for (i=0;i<argc;i++) {
	#ifdef VERBOSE
		printf("Retrieving column '%s' => %s\n", cn[i], argv[i] ? argv[i] : "NULL");
	#endif
		lua_pushstring(L, cn[i]); 
		lua_pushstring(L, argv[i] ? argv[i] : "NULL");
		lua_settable(L, 5);
	}
	lua_settable(L, 3);
	counter++;
	return 0; 
}

static int schema_db (lua_State *L) {
	const char *dbarg, *dbfile, *table;
	int rc, ch, nt=0; 
	counter=1;

	/* Check args coming from Lua */
	if (!(dbarg = luaL_checkstring(L, 1)))
		lua_return(0, "argument 1 is not a string or database name.");

	/* Get the last string */
	if (lua_gettop(L) == 2) {
		nt = 1;
		/* Have the option to trim down to just one table */
		if (!(table = luaL_checkstring(L, 2)))
			lua_return(0, "argument 2 is not a string or table name.");
	}

	/* Check for in-memory database. */
	dbfile = (strcmp(dbarg, ":memory") == 0) ? ":memory" : dbarg;

	/* Open the database handle. */
	if (!lload_db(dbfile))
		lua_return(0, errbuf);

	/* Pop off all args */
	lua_pop(L, (nt) ? 2 : 1);

	/* Add slots for status and results */
	lua_newtable(L); // 1
	lua_pushstring(L, "results"); // 2
	lua_newtable(L); // 3

	/* Execute all SQL (this will do more than one query) */
	if (!nt) {
		if (!lexec_db(L, "select * from sqlite_master"))
			lua_return(0, errbuf);
	}
	else {
		char query[1024];
		snprintf(query, 1023, "select * from sqlite_master where name = '%s'", table); 
		if (!lexec_db(L, query))
			lua_return(0, errbuf);
	}

	lua_return(1, NULL);
	sqlite3_close(db);
	return 1;	
	return 1;
}

/* remove */
static int remove_db (lua_State *L) {
	return 0;
}


/* check */
static int check_table (lua_State *L) {
	const char *table;
	char query[1024] = {0};
	int rc;
	if (!(table = luaL_checkstring(L, 1))) {
		lua_pushboolean(L, 0);
		/* Error messages */
		return 1;
	}
	/* Write this table */
	snprintf(query, 1023, "select * from %s limit 1", table);
	/* Can't this be shorter? */
	if ((rc = sqlite3_exec(db, query, callback, 0, &errmsg)))
		lua_pushboolean(L, 1);
	else
		lua_pushboolean(L, 0);
	return 1;
}

/* write and escape bad characters */
/* '' apostrophes */

/* read */
static int exec_db (lua_State *L) {
	const char *dbarg, *dbfile, *query;
	int rc, ch; 
	counter=1;

	/* Check arguments coming from Lua */
	if (!(dbarg = luaL_checkstring(L, 1)))
		lua_return(0, "argument 1 is not a string.");

	if (!(query = luaL_checkstring(L, 2)))
		lua_return(0, "argument 2 is not a string.");

	/* Check for in-memory or not. */
	dbfile = (strcmp(dbarg, ":memory") == 0) ? ":memory" : dbarg;

	/* Pop off all arguments */
	lua_pop(L, 2);

	/* Open the database handle. */
	if (!lload_db(dbfile))
		lua_return(0, errbuf);

	/* Add slots for status and results */
	lua_newtable(L); // 1
	lua_pushstring(L, "results"); // 2
	lua_newtable(L); // 3

	/* Execute all SQL (this will do more than one query) */
	if (!lexec_db(L, query))
		lua_return(0, errbuf);

	#if 0
	/* If this was an idempotent operation, return results */
	if (!(ch = sqlite3_changes(db))) {
		lua_pop(L, 2);
	} 
	else {
		lua_settable(L, 1);
	}
	#endif
	lua_settable(L, 1);
	lua_return(1, NULL);
	sqlite3_close(db);
	return 1;	
}


#if 1
typedef struct { 
	char *name; 
	lua_CFunction func; 
	char *setname; 
	int sentinel;
} luaCF; 

luaCF regg[] =
{
	{ .setname = "set1" },
		{ "abc", abc },
		{ "val", abc },
		{ .sentinel = 1 },

	{ .setname = "set2" },
		{ "xyz", abc },
		{ "def", abc },
		{ .sentinel = 1 },

	/*Database module*/	
	{ .setname = "db" },
		{ "exec",   exec_db },
		{ "schema", schema_db },
		{ "check",  check_table },
		{ .sentinel = 1 },

	/*Render module*/	
	{ .setname = "render" },
		{ "file",   abc },
		{ .sentinel = 1 },

	/*End this*/
	{ .sentinel = -1 },
};
#endif

/*Loop through a table in memory*/
static void lua_loop ( lua_State *L )
{
	int a = lua_gettop( L );
	obprintf( stderr, "Looping through %d values.\n", a );

	for ( int i=1; i <= a; i++ )
	{
		obprintf( stderr, "%-2d: %s\n", 
			i, lua_typename( L, lua_type( L, i )) );
		//lua_getindex(L, i );
	}
}

/*Alloc*/
static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
	return NULL;
}


int abc( lua_State *L ) 
{
	fprintf( stderr, " stdbob " );
	return 0;
}


int lua_http_pre (const Recvr *r, void *p, char *e)
{ 
#if 0
	lua_State *L = luaL_newstate(); 
	luaL_Reg *rg = reg;
	HTTP *h      = (HTTP *)r->userdata;
	Table *t     = malloc( sizeof(Table) );
#endif
	return 1; 
}


//Convert Table to Lua table
int table_to_lua (lua_State *L, int index, Table *tt)
{
	int a = 1;
	LiteKv *kv = NULL;
	lt_reset( tt );

	while ( (kv = lt_next( tt )) )
	{
		struct { int t; LiteRecord *r; } items[2] = {
			{ kv->key.type  , &kv->key.v    },
			{ kv->value.type, &kv->value.v  } 
		};

		int t=0;
		for ( int i=0; i<2; i++ ) 
		{
			LiteRecord *r = items[i].r; 
			t = items[i].t;
			obprintf( stderr, "%s\n", ( i ) ? lt_typename( t ) : lt_typename( t ));

			if ( i ) 
			{
				if (t == LITE_NUL)
					;
				else if (t == LITE_USR)
					lua_pushstring( L, "usrdata received" ); //?
				else if (t == LITE_TBL) 
				{
					lua_newtable( L );
					a += 2;
				}
			}
			if (t == LITE_NON)
				break;	
			else if (t == LITE_FLT || t == LITE_INT)
				lua_pushnumber( L, r->vint );				
			else if (t == LITE_FLT)
				lua_pushnumber( L, r->vfloat );				
			else if (t == LITE_TXT)
				lua_pushstring( L, r->vchar );
			else if (t == LITE_TRM)
			{
				a -= 2;	
				break;
			}
			else if (t == LITE_BLB) 
			{
				LiteBlob *bb = &r->vblob;
				lua_pushlstring( L, (char *)bb->blob, bb->size );
			}
		}

		( t == LITE_NON || t == LITE_TBL || t == LITE_NUL )	? 0 : lua_settable( L, a );
		lua_loop( L );
	}
	return 1;
}


//Convert Lua tables to regular tables
int lua_to_table (lua_State *L, int index, Table *t )
{
	static int sd;
	lua_pushnil( L );
	obprintf( stderr, "Current stack depth: %d\n", sd++ );

	//
	while ( lua_next( L, index ) != 0 ) 
	{
		int kt, vt;
		obprintf( stderr, "key, value: " );

		//This should pop both keys...
		obprintf( stderr, "%s, %s\n", lua_typename( L, lua_type(L, -2 )), lua_typename( L, lua_type(L, -1 )));

		//Keys
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			obprintf( stderr, "key: %lld\n", (long long)lua_tointeger( L, -2 ));
		else if ( kt  == LUA_TSTRING )
			obprintf( stderr, "key: %s\n", lua_tostring( L, -2 ));

		//Values
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			obprintf( stderr, "val: %lld\n", (long long)lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			obprintf( stderr, "val: %s\n", lua_tostring( L, -1 ));

		//Get key (remember Lua indices always start at 1.  Hence the minus.
		if (( kt = lua_type( L, -2 )) == LUA_TNUMBER )
			lt_addintkey( t, lua_tointeger( L, -2 ) - 1);
		else if ( kt  == LUA_TSTRING )
			lt_addtextkey( t, (char *)lua_tostring( L, -2 ));

		//Get value
		if (( vt = lua_type( L, -1 )) == LUA_TNUMBER )
			lt_addintvalue( t, lua_tointeger( L, -1 ));
		else if ( vt  == LUA_TSTRING )
			lt_addtextvalue( t, (char *)lua_tostring( L, -1 ));
		else if ( vt == LUA_TTABLE )
		{
			lt_descend( t );
			obprintf( stderr, "Descending because value at %d is table...\n", -1 );
			lua_loop( L );
			lua_to_table( L, index + 2, t ); 
			lt_ascend( t );
			sd--;
		}


		obprintf( stderr, "popping last two values...\n" );
		if ( vt == LUA_TNUMBER || vt == LUA_TSTRING )
		lt_finalize( t );
		lua_pop(L, 1);
	}

	lt_lock( t );
	return 0;
}






#include "lua-data.c"


int lua_http_handler (HTTP *h, Table *p)
{
	//Initialize Lua's environment and set up everything else
	char renderblock[ 60000 ] = { 0 };
	lua_State *L = luaL_newstate(); 
	luaCF *rg = regg;
	LiteBlob *b  = NULL;
	Table t;     
	Render R; 
	Buffer *rr = NULL;
	char *modelfile = NULL, 
       *viewfile = NULL;

	//Set up a table
	lt_init( &t, NULL, 127 );
	
	//Check that Lua initialized here
	if ( !L )
		return 0;

	//Now create two tables: 1 for env, and another for 
	//user defined functions 
	luaL_openlibs( L );
	lua_newtable( L );
	int at=2;
	lua_loop( L );

	//Loop through and add each UDF
	while ( rg->sentinel != -1 )
	{
		//Set the top table
		if ( rg->sentinel == 1 ) 
		{
			lua_settable( L, 1 );
			lua_loop( L );
		}

		else if ( !rg->name && rg->setname )
		{
			obprintf( stderr, "Registering new table: %s\n", rg->setname );
			lua_pushstring( L, rg->setname );
			lua_newtable( L );
			lua_loop( L );
		}

		else if ( rg->name )
		{	
			obprintf( stderr, "Registering funct: %s\n", rg->name );
			lua_pushstring( L, rg->name );
			lua_pushcfunction( L, rg->func );
			lua_settable( L, 3 );
		}

		rg++;
	}

	//Loop through all of the http structure
	table_to_lua( L, 1, &h->request.table );

	//Each one of these needs to be in a table
	fprintf( stderr," Finished converting... " );
	lua_setglobal( L, "env" ); /*This needs to be readonly*/

	//Get data.lua if it's available and load routes
	if ( luaL_dofile( L, "lua-test/index.lua" ) != 0 )
	{
		fprintf( stderr, "Error occurred!\n" );
		if ( lua_gettop( L ) > 0 ) {
			fprintf( stderr, "%s\n", lua_tostring( L, 1 ) );
		}
	}

	//Converts what came from the stack
	lua_loop( L );
	lua_to_table( L, 1, &t );
	lt_dump( &t );

	//Initialize the rendering module	
	//TODO: Error handling is non-existent here...
	int fd = open( "lua-test/views/default.html", O_RDONLY );
	read( fd, renderblock, sizeof( renderblock )); 
	close(fd);
	render_init( &R, &t );
	render_map( &R, (uint8_t *)renderblock, strlen( renderblock ));
	render_render( &R ); 
	rr = render_rendered( &R );
	//write( 2, bf_data( rr ), bf_written( rr ) );

#if 1
	http_set_status( h, 200 );
	http_set_content( h, "text/html", bf_data( rr ), bf_written( rr ));
	//http_set_content_length( h, );
	//http_set_content_type( h, "text/html" );
#endif

	render_free( &R );
	lt_free( &t );

	return 1;
}

