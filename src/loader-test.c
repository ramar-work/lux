/* ------------------------------------------- * 
 * loader-test.c 
 * --------------
 * Test out loader.c against a few different
 * styles of file.
 *
 * All of the tests are broken up into 4 distinct
 * parts (which can be seen in main.c). 
 * 
 * #1 - Allocate space of $SIZE for the data set
 * we plan to use.
 *
 * #2 - Run the rule parser.  This takes each key found in the rule, looks
 * for a corresponding value in the loaded table, and sets the value in the
 * data allocated above.
 *
 * #3 - Dump the rules
 *
 * #4 - Free the rules.
 *
 * After all of this occurs, all should be well. (meaning no open Lua env,
 * no dangling references, etc)
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * No entries yet.
 * ------------------------------------------- */
#include "loader.h"
#include "util.h"
#include "hosts.h"
#include "routes.h"
#include "luautil.h"

#define TESTDIR "tests/loader/"

#define TESTCASE(D) \
	{ TESTDIR #D ".lua", #D, sizeof(struct D), set_ ## D ## _v, dump_ ## D ## _v, free_ ## D ## _v }


//Test #1 - Take this 'meg' data structure and populates it
//with data from a Lua file at TESTDIR/meg.lua
struct meg {
	char *wwwroot;
	int cycle;
	struct home { char *home, *type; } **homes;
};


//A callback used to stream zKeyvals into our structure.
int meg_homes_handler ( zKeyval * kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct home ***homes = f->userdata;
	struct home *home = NULL;

	FPRINTF( "DEPTH AT %d, RIGHT LEFT KEYS FOR KEY 'HOMES'\n", f->depth );
	if ( kv->key.type == LITE_TXT ) {
		home = malloc( sizeof( struct home ) );
		home->home = kv->key.v.vchar;
	}

	if ( kv->value.type == LITE_TXT ) {
		home->type = kv->value.v.vchar;
		add_item( homes, home, struct home *, &f->len );
	}

	return 1;
}


//We pass in whatever we want to work with
int set_meg_v( zTable *t, void *p ) {
	struct meg *m = ( struct meg * )p;

	const struct rule rules[] = {
		{ "wwwroot", "s", .v.s = &m->wwwroot  },
		{ "cycle", "i", .v.i = &m->cycle },
		{ "homes", "t", .v.t = ( void *** )&m->homes, meg_homes_handler },
		{ NULL }
	};

	//Try the loadset
	if ( !loader_run( t, rules ) ) {
		return 0;
	} 

	return 1;
}
 

//Dump the rules that we created.
void dump_meg_v( zTable *t, void *p ) {
	struct meg *m = ( struct meg * )p;
	fprintf( stderr, "wwwroot: %s\n", m->wwwroot );
	fprintf( stderr, "cycle: %d\n", m->cycle );
	fprintf( stderr, "homes:\n" );
	//We have to always use a local reference when running through this
	struct home **homes = m->homes;
	while ( homes && ( *homes ) ) {
		static int i;
		struct home *h = (*homes);	
		fprintf( stderr, "  [%d] '%s' => '%s'\n", i++, h->home, h->type );
		homes++;
	}
}


//Free the rules that were created
void free_meg_v( zTable *t, void *p ) {
	struct meg *m = ( struct meg * )p;
	struct home **homes = m->homes;
	while ( homes && (*homes) ) {
		free( *homes );
		homes++;
	}
	free( m->homes );
	free( m->wwwroot );
}


//This is a simple structure.
struct hostset { 
	struct host **hosts; 
};


int test_hosts_handler ( zKeyval * kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct host ***hosts = f->userdata;
	zTable *st = NULL, *nt = NULL;
	FPRINTF( "Invoking hosts_handler\n" );

	//If current index is a table
	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL && f->depth == 2 ) {
		struct host *w = NULL;
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
		FPRINTF( "NAME: %s, COUNT OF ELEMENTS: %d\n", kv->key.v.vchar, count ); 
		nt = loader_shallow_copy( st, i+1, i+count );
		w = malloc( sizeof( struct host ) );
		memset( w, 0, sizeof(struct host) );
		const struct rule rules[] = {
			{ "alias", "s", .v.s = &w->alias },
			{ "dir", "s", .v.s = &w->dir },
			{ "filter", "s", .v.s = &w->filter },
			{ "root_default", "s", .v.s = &w->root_default },
			{ "ca_bundle", "s", .v.s = &w->ca_bundle },
			{ "certfile", "s", .v.s = &w->certfile },
			{ "keyfile", "s", .v.s = &w->keyfile },
			{ NULL }
		};

		//Why would this fail?
		w->name = kv->key.v.vchar;
		loader_run( nt, rules ); 

		//This is extra stupid, b/c all you really need to do is loop through the table entries	
		add_item( hosts, w, struct host *, &f->len );

		//Let's just try destroying this table
		lt_free( nt );
		free( nt );
	}
	FPRINTF( "Done with hosts_handler\n" );
	return 1;
}




//Tests rules for pulling multiple keys that are the same.
int set_hostset_v( zTable *t, void *p ) {
	struct hostset *hostset = (struct hostset *)p;
	const struct rule rules[] = {
		{ "hosts", "t", .v.t = (void ***)&hostset->hosts, test_hosts_handler }, 
		{ NULL }
	};

	//Try the loadset
	if ( !loader_run( t, rules ) ) {
		return 0;
	}
	return 1;
}


void dump_hostset_v ( zTable *t, void *p ) {
	struct hostset *hostset = (struct hostset *)p;
	struct host **hosts = hostset->hosts;
	while ( hosts && (*hosts) ) {
		fprintf( stderr, "Host: %s\n", (*hosts)->name );
		fprintf( stderr, "  w->alias: %s\n", (*hosts)->alias );
		fprintf( stderr, "  w->dir: %s\n", (*hosts)->dir );
		fprintf( stderr, "  w->filter: %s\n", (*hosts)->filter );
		fprintf( stderr, "  w->root_default: %s\n", (*hosts)->root_default );
		fprintf( stderr, "  w->ca_bundle: %s\n", (*hosts)->ca_bundle );
		fprintf( stderr, "  w->certfile: %s\n", (*hosts)->certfile );
		fprintf( stderr, "  w->keyfile: %s\n", (*hosts)->keyfile );
		hosts++;
	}
}


void free_hostset_v ( zTable *t, void *p ) {
	struct hostset *hostset = (struct hostset *)p;
	struct host **hosts = hostset->hosts;
	while ( hosts && (*hosts) ) {
		( (*hosts)->alias ) ? free( (*hosts)->alias ) : 0;
		( (*hosts)->dir ) ? free( (*hosts)->dir ) : 0;
		( (*hosts)->filter ) ? free( (*hosts)->filter ) : 0;
		( (*hosts)->root_default ) ? free( (*hosts)->root_default ) : 0;
		( (*hosts)->ca_bundle ) ? free( (*hosts)->ca_bundle ) : 0;
		( (*hosts)->certfile ) ? free( (*hosts)->certfile ) : 0;
		( (*hosts)->keyfile ) ? free( (*hosts)->keyfile ) : 0;
		free( (*hosts) ); 
		hosts++;
	}
	free( hostset->hosts );
}



struct routelist {
	struct routeh { 
		char *name; 
		//struct mvc *mvc;
		struct mvc {
			char **models;
			char **views;
			char **queries;
			char *returns;
			char *auth;
			char *content;
			char *a;
		} mvc;
	} **routes;
};



int mvc_array_handler( zKeyval *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	void ***pp = f->userdata;
	int kt = kv->key.type;

	//If table count is one, then it better be a string.
	if ( ( kt == LITE_INT || kt == LITE_TXT ) && kv->value.type == LITE_TXT ) {
		char *p = strdup( kv->value.v.vchar );
		add_item( pp, p, char *, &f->len ); 
	}
	return 1;
}


//...
int test_routes_handler ( zKeyval * kv, int i, void *p ) {
	FPRINTF( "Invoking routes_handler\n" );
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct routeh ***routes = f->userdata;
	zTable *st = NULL, *nt = NULL;
	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL && f->depth == 2 ) {
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
#if 1
		struct routeh *route = malloc( sizeof( struct routeh ) );
		memset( route, 0, sizeof( struct routeh ) );
		route->name = kv->key.v.vchar;
#endif
		//FPRINTF( "Route name: %s\n", kv->key.v.vchar );
		nt = loader_shallow_copy( st, i+1, i+count );
		//lt_dump( nt );

#if 1
		const struct rule rules[] = {
			//Can be string, function or table
			{ "model", "x", .v.t = (void ***)&route->mvc.models, mvc_array_handler },
			{ "view", "x", .v.t = (void ***)&route->mvc.views, mvc_array_handler },
			{ "query", "x", .v.t = (void ***)&route->mvc.queries, mvc_array_handler },
			{ "returns", "s", .v.s = &route->mvc.returns },
			{ "content", "s", .v.s = &route->mvc.content },
	#if 0
			{ "auth", "s", .v.s = &w->kk },
			{ "content-type", "s", .v.s = &w->kk },
			{ "hint", "s", .v.s = &w->kk },
	#endif
			{ NULL }
		};
		
		if ( !loader_run( nt, rules ) ) {
		}
#endif
		//route->mvc = rh;
		add_item( routes, route, struct routeh *, &f->len );
		lt_free( nt );
		free( nt );
	}
	FPRINTF( "Done with routes_handler\n" );
	return 1;
}


//A callback used to populate everything in routes
int set_routelist_v( zTable *t, void *p ) {
	struct routelist *r = ( struct routelist * )p;
	struct routeh **routes = r->routes;
	//mvc probably has to be allocated here
	const struct rule rules[] = {
		{ "routes", "t", .v.t = (void ***)&r->routes, test_routes_handler }, 
		{ NULL }
	};

	//Try the loadset
	if ( !loader_run( t, rules ) ) {
		return 0;
	}

	return 1;
}


void dump_routelist_v (zTable *t, void *p) {
	//Dump everything
	struct routelist *r = ( struct routelist * )p;
	struct routeh **routes = r->routes;

	while ( routes && (*routes) ) {
		static int i;
		FPRINTF( "route %d: %s\n", i++, (*routes)->name );

		//Notice more of the local referencing.  
		//Using a pointer here will increment so that we lose track of the data.
		struct mvc c = (*routes)->mvc;
		FPRINTF( "\tmodel " );
		while ( c.models && *c.models ) {
			fprintf( stderr, "%s, ", *c.models ); 
			c.models++;
		}

		fprintf( stderr, "views: " );
		while ( c.views && *c.views ) { 
			fprintf( stderr, "%s, ", *c.views ); c.views++;
		}
		fprintf( stderr, "\n" );
		routes++;
	}
}


void free_routelist_v (zTable *t, void *p) {
	struct routelist *r = ( struct routelist * )p;
	struct routeh **routes = r->routes;

	while ( routes && (*routes) ) {
		struct mvc *c = &(*routes)->mvc;
		char **m = c->models, **v = c->views;
		while ( m && *m ) { free( *m ); m++; }
		while ( v && *v ) { free( *v ); v++; }
		free( c->models );
		free( c->views );
		free( *routes );
		routes++;	
	}
	free( r->routes );
}




#if 0
struct mvc {
	char **models;
	char **views;
	char **queries;
	char *returns;
	char *auth;
	char *content;
	char *a;
};


struct mvc {
	char **models;
	char **views;
	char **queries;
	char *returns;
	char *auth;
	char *content;
	char *a;
};


struct routeh { 
	char *name; 
	struct mvc *mvc;
};


//Models and Views are INCREDIBLY difficult
//This runs on everything in a set...
int mvc_array_handler( zKeyval *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	void ***pp = f->userdata;
	int kt = kv->key.type;

	//If table count is one, then it better be a string.
	if ( ( kt == LITE_INT || kt == LITE_TXT ) && kv->value.type == LITE_TXT ) {
		add_item( pp, kv->value.v.vchar, char *, &f->len ); 
	}
	return 1;
}


//...
int test_routes_handler ( zKeyval * kv, int i, void *p ) {
	FPRINTF( "Invoking routes_handler\n" );
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct routeh ***routes = f->userdata;
	zTable *st = NULL, *nt = NULL;
	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL && f->depth == 2 ) {
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
#if 1
		struct routeh *route = malloc( sizeof( struct routeh ) );
		memset( route, 0, sizeof( struct routeh ) );
		route->name = strdup( kv->key.v.vchar );
#endif
		FPRINTF( "Route name: %s\n", kv->key.v.vchar );
		nt = loader_shallow_copy( st, i+1, i+count );
		lt_dump( nt );

#if 1
		//Allocate a routehandler
		struct mvc *rh = malloc( sizeof( struct mvc ) );
		memset( rh, 0, sizeof( struct mvc ) );

		const struct rule rules[] = {
			//Can be string, function or table
			{ "model", "x", .v.t = (void ***)&rh->models, mvc_array_handler },
			{ "view", "x", .v.t = (void ***)&rh->views, mvc_array_handler },
			{ "query", "x", .v.t = (void ***)&rh->queries, mvc_array_handler },
			{ "returns", "s", .v.s = &rh->returns },
			{ "content", "s", .v.s = &rh->content },
	#if 0
			{ "auth", "s", .v.s = &w->kk },
			{ "content-type", "s", .v.s = &w->kk },
			{ "hint", "s", .v.s = &w->kk },
	#endif
			{ NULL }
		};
		
		if ( !loader_run( nt, rules ) ) {
		}
#endif
		route->mvc = rh; 	
		add_item( routes, route, struct mvc *, &f->len );
	}
	FPRINTF( "Done with routes_handler\n" );
	return 1;
}



//Tests rules for pulling multiple keys that are the same.
int set_host_rules( zTable *t ) {
	struct host **hosts = NULL;
	const struct rule rules[] = {
		{ "hosts", "t", .v.t = (void ***)&hosts, test_hosts_handler }, 
		{ NULL }
	};

	//Try the loadset
	if ( !loader_run( t, rules ) ) {
		return 0;
	}

	//See the hosts
	while ( hosts && (*hosts) ) {
		static int i;
		FPRINTF( "w->name: %s\n", (*hosts)->name );
		FPRINTF( "w->alias: %s\n", (*hosts)->alias );
		FPRINTF( "w->dir: %s\n", (*hosts)->dir );
		FPRINTF( "w->filter: %s\n", (*hosts)->filter );
		FPRINTF( "w->root_default: %s\n", (*hosts)->root_default );
		FPRINTF( "w->ca_bundle: %s\n", (*hosts)->ca_bundle );
		FPRINTF( "w->certfile: %s\n", (*hosts)->certfile );
		FPRINTF( "w->keyfile: %s\n", (*hosts)->keyfile );
		free( (*hosts)->name );
		free( (*hosts)->alias );
		free( (*hosts)->dir );
		free( (*hosts)->filter );
		free( (*hosts)->root_default );
		free( (*hosts)->ca_bundle );
		free( (*hosts)->certfile );
		free( (*hosts)->keyfile );
		free( (*hosts) );
		hosts++;
	}
	return 1; 
}


//...
int set_route_rules ( zTable *t ) {
	struct routeh **routes = NULL;
	const struct rule rules[] = {
		{ "routes", "t", .v.t = (void ***)&routes, test_routes_handler }, 
		{ NULL }
	};

	//Try the loadset
	if ( !loader_run( t, rules ) ) {
		return 0;
	}

	//Dump everything
	while ( routes && *routes ) {
		static int i;
		FPRINTF( "route %d: %s\n", i++, (*routes)->name );

		struct mvc *c = (*routes)->mvc;
		FPRINTF( "\tmodel " );
		while ( c->models && *c->models ) {
			fprintf( stderr, "%s, ", *c->models ); c->models++;
		}

		fprintf( stderr, "views: " );
		while ( c->views && *c->views ) { 
			fprintf( stderr, "%s, ", *c->views ); c->views++;
		}
		fprintf( stderr, "\n" );
		routes++;
	}
	return 1;
}


//Tests more rules for pulling one unique key per value
int set_lua_rules ( zTable *t ) {
	const struct rule rules[] = {
		{ "db", "s", .v.s = &host.db }, 
		{ "title", "s", .v.s = &host.title },  
		{ "fqdn", "s", .v.s = &host.fqdn }, 
		//{ "routes", "t", build_routes }, 
		{ NULL }
	};

 #if 0
	return ( loader_run( t, rules ) );
 #else
	if ( !loader_run( t, rules ) ) {
		return 0;
	}
	return 1; 
 #endif
}
#endif



struct Test {
	//Filename to use for the test
	const char *filename;

	//Name to use for the test
	const char *name;

	//Size of void *p;
	const int sizep;

	//A callback to set the rules
	int (*set)( zTable *, void *p );	

	//A callback to dump the filled struct 
	void (*dump)( zTable *, void *p );	

	//A callback to free bytes allocated by following the ruleset
	void (*free)( zTable *, void *p );	

	//Random pointer data
	void *data;
} tests[] = {
#if 0
	TESTCASE( meg ),
 	TESTCASE( hostset ),
#else
 	TESTCASE( routelist ),
#if 0
	{ TESTDIR "server.lua", set_server_rules },
	{ TESTDIR "server.lua", set_host_rules },
	{ TESTDIR "host.lua", set_route_rules },
#endif
#endif
 	{ NULL }
};



int main (int argc, char *argv[]) {
	struct Test *t = tests;
	while ( t->filename ) {

		//Initialize Lua and allocate a single table. 
		lua_State *L = luaL_newstate();
		zTable *tt = malloc( sizeof(zTable) ); 
		char err[ 2048 ] = { 0 };
		void *p = NULL;

		//Load via Lua
		FPRINTF( "Attempting to load file: %s\n", t->filename );
		if ( !lua_exec_file( L, t->filename, err, sizeof(err) ) ) {
			FPRINTF( "Error trying to load Lua file: %s\n", err );
			goto next;	
		}

		//Create a table
		if ( !lt_init( tt, NULL, 1024 ) ) {
			FPRINTF( "Error allocating new table.\n" );
			goto next;
		}

		//Put Lua data into table
		if ( !lua_to_table( L, 1, tt ) ) {
			FPRINTF( "Error trying to move Lua data into table.\n" );
			goto next;
		}
	
		//Allocate the structure we need for testdata
		if ( t->sizep > 0 && !( p = malloc( t->sizep ) ) ) {
			FPRINTF( "Error allocating userdata for testing.\n" );
			goto onext;
		}

		memset( p, 0, t->sizep );

		//Run the table through the set of rules and let's see what we find 
		if ( !t->set( tt, p ) ) {
			FPRINTF( "Error setting userdata from Lua config file.\n" );
			goto onext;
		}

		//Dump...
		t->dump( tt, p );

		//...and free
		t->free( tt, p );

onext:
		lt_free( tt );

next:
		free( tt );
		free( p );
		lua_close( L );
		t++;
	}
	return 0;
}
