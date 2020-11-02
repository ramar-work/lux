#include "loader.h"
#include "util.h"
#include "hosts.h"
#include "routes.h"
#include "luautil.h"

#define TESTDIR "tests/loader/"

struct home {
	char *home;
	char *type;
};

struct meg {
	char *wwwroot;
	int cycle;
	struct home **homes;
};


int homes_handler ( zKeyval * kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct home ***homes = f->userdata;
	struct home *home = NULL;

	FPRINTF( "DEPTH AT %d, RIGHT LEFT KEYS FOR KEY 'HOMES'\n", f->depth );
	if ( kv->key.type == LITE_TXT ) {
		home = malloc( sizeof( struct home ) );
		home->home = strdup( kv->key.v.vchar );
	}

	if ( kv->value.type == LITE_TXT ) {
		home->type = strdup( kv->value.v.vchar );
		add_item( homes, home, struct home *, &f->len );
	}

	return 1;
}


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
		w->name = strdup( kv->key.v.vchar );
		loader_run( nt, rules ); 
		add_item( hosts, w, struct host *, &f->len );
	}
	FPRINTF( "Done with hosts_handler\n" );
	return 1;
}


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

//Tests rules for pulling keys at a top-level.
//I expect to see one unique key for one unique value (of any type)
int set_server_rules( zTable *t ) {
	struct meg m;
	memset( &m, 0, sizeof( struct meg ) );

	const struct rule rules[] = {
		{ "wwwroot", "s", .v.s = &m.wwwroot  },
		{ "cycle", "i", .v.i = &m.cycle },
		{ "homes", "t", .v.t = ( void *** )&m.homes, homes_handler },
		{ NULL }
	};

	//Try the loadset
	if ( !loader_run( t, rules ) ) {
		return 0;
	} 

	fprintf( stderr, "wwwroot: %s\n", m.wwwroot );
	fprintf( stderr, "cycle: %d\n", m.cycle );
	fprintf( stderr, "homes:\n" );
	while ( m.homes && (*m.homes) ) {
		static int i;
		fprintf( stderr, "home %d: %s\n", i++, (*m.homes)->home );
		free( (*m.homes)->home );
		free( (*m.homes)->type );
		free( (*m.homes) );
		m.homes++;
	}
	free( m.wwwroot );
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


#if 1
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
#endif


#if 0
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
	const char *name;
	int (*exec)( zTable * );	
	void *data;
} tests[] = {
	{ TESTDIR "server.lua", set_server_rules },
	{ TESTDIR "server.lua", set_host_rules },
	{ TESTDIR "host.lua", set_route_rules },
	{ NULL }
};


int main (int argc, char *argv[]) {
	struct Test *t = tests;
	while ( t->name ) {
		//Build filename
		lua_State *L = luaL_newstate();
		zTable *tt = malloc( sizeof(zTable) ); 
		char err[ 2048 ] = { 0 };
		FPRINTF( "Attempting to load file: %s", t->name );

		//Load via Lua
		if ( !lua_exec_file( L, t->name, err, sizeof(err) ) ) {
			FPRINTF( "Error trying to load Lua file: %s\n", err );
			goto next;	
		}

		//Create and dump the table
		if ( !lt_init( tt, NULL, 1024 ) || !lua_to_table( L, 1, tt ) ) {
			FPRINTF( "Error trying convert Lua to table.\n" );
			goto next;
		}

		if ( !t->exec( tt ) ) {
			FPRINTF( "Loader run failed.\n" );
			goto next;
		}
next:
		free( tt );
		lua_close( L );
		t++;
	}
	return 0;
}
