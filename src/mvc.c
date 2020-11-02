//mvc.c
//Models and Views are INCREDIBLY difficult
//This runs on everything in a set...
#include "mvc.h"
#include "router.h"

static int mvc_array_handler( zKeyval *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	void ***pp = f->userdata;
	int kt = kv->key.type;

	//If table count is one, then it better be a string.
	if ( ( kt == LITE_INT || kt == LITE_TXT ) && kv->value.type == LITE_TXT ) {
		char *val = strdup( kv->value.v.vchar );
		add_item( pp, val, char *, &f->len ); 
	}
	return 1;
}


static struct routeh * create_route ( const char *name ) {
	struct routeh *route = malloc( sizeof( struct routeh ) );
	if ( !route ) {
		return NULL;
	}
	memset( route, 0, sizeof( struct routeh ) );
	route->name = strdup( name );
	route->mvc = NULL;
	return route;
}


static struct mvc * create_mvc () {
	struct mvc *rh = malloc( sizeof( struct mvc ) );
	memset( rh, 0, sizeof( struct mvc ) );
	rh->returns = NULL;
	rh->content = NULL;
	rh->queries = NULL;
	return rh;
}


//...
static int mvc_handler ( zKeyval * kv, int i, void *p ) {
	FPRINTF( "Invoking routes_handler\n" );
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct routeh ***routes = f->userdata;
	zTable *st = NULL, *nt = NULL;

	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL && f->depth == 2 ) {
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
#if 1
		struct routeh *route = create_route( kv->key.v.vchar );
		struct mvc *mvc = create_mvc();
		FPRINTF( "Route name: %s\n", route->name );
#else
		struct routeh *route = malloc( sizeof( struct routeh ) );
		memset( route, 0, sizeof( struct routeh ) );
		route->name = strdup( kv->key.v.vchar );
#endif
		nt = loader_shallow_copy( st, i+1, i+count );

		const struct rule rules[] = {
			//Can be string, function or table
			{ "model", "x", .v.t = (void ***)&mvc->models, mvc_array_handler },
			{ "view", "x", .v.t = (void ***)&mvc->views, mvc_array_handler },
			{ "query", "x", .v.t = (void ***)&mvc->queries, mvc_array_handler },
			{ "returns", "s", .v.s = &mvc->returns },
			{ "content", "s", .v.s = &mvc->content },
	#if 0
			{ "auth", "s", .v.s = &w->kk },
			{ "content-type", "s", .v.s = &w->kk },
			{ "hint", "s", .v.s = &w->kk },
	#endif
			{ NULL }
		};
	
		//TODO: Loader run should throw something back if a key wasn't found...	
		loader_run( nt, rules ); 
		lt_free( nt );
		free( nt );
		route->mvc = mvc; 	
		add_item( routes, route, struct mvc *, &f->len );
	}
	FPRINTF( "Done with routes_handler\n" );
	return 1;
}


struct routeh ** build_mvc ( zTable *t ) {
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

void free_set( char **slist ) {
	char **list = slist;	
	#if 0
	while ( *list ) { 
		if ( *list ) {
		FPRINTF( "FREEING %s\n", *list );
		//ree( *list );
		}
		list++;
	}
	#endif
	free( slist );
}

void free_mvc ( struct mvc *mvc ) {
	if ( mvc->returns )
		free( mvc->returns );
	if ( mvc->content )
		free( mvc->content );
#if 0
	if ( mvc->auth )
		free( mvc->auth );
#endif
	free_set( mvc->models );
	free_set( mvc->views );
	free_set( mvc->queries );
	free( mvc );
}


void free_routeh ( struct routeh **list ) {
	FPRINTF( "Freeing routeh list\n" );
	struct routeh **routes = list;
	while ( routes && *routes ) {
		FPRINTF( "Freeing route at %s\n", (*routes)->name );
		free_mvc( (*routes)->mvc );
		free( (*routes)->name );
		routes++;
	}
	free( list );
	FPRINTF( "Freeing routeh list\n" );
}



