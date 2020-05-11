//mvc.c
//Models and Views are INCREDIBLY difficult
//This runs on everything in a set...
#include "mvc.h"

static int mvc_array_handler( LiteKv *kv, int i, void *p ) {
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
static int mvc_handler ( LiteKv * kv, int i, void *p ) {
	FPRINTF( "Invoking routes_handler\n" );
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct routeh ***routes = f->userdata;
	Table *st = NULL, *nt = NULL;
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


struct routeh ** build_mvc ( Table *t ) {
	struct routeh **routes = NULL;
	const struct rule rules[] = {
		{ "routes", "t", .v.t = (void ***)&routes, mvc_handler }, 
		{ NULL }
	};

	//TODO: Why would this fail?
	if ( !loader_run( t, rules ) ) {
		return NULL;
	}
	return routes;
}


void dump_routeh ( struct routeh **rlist ) {
	//Dump everything
	struct routeh **routes = rlist;
	while ( routes && *routes ) {
		static int ri;
		struct mvc *c = (*routes)->mvc;
		FPRINTF( "route %d: %s\n", ri++, (*routes)->name );
		dump_mvc( c );
		routes++;
	}
}


void dump_mvc ( struct mvc *mvc ) {
	//Dump everything
	char **models = mvc->models; 
	char **views = mvc->views;
	
	FPRINTF( "\tmodels: " );
	while ( models && *models ) {
		fprintf( stderr, "%s, ", *models ); 
		models++;
	}

	fprintf( stderr, "views: " );
	while ( views && *views ) { 
		fprintf( stderr, "%s, ", *views ); 
		views++;
	}
	fprintf( stderr, "\n" );
}

