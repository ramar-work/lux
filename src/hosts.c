#include "hosts.h"

//A hosts handler
static int hosts_iterator ( zKeyval * kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct host ***hosts = f->userdata;
	zTable *st = NULL, *nt = NULL;

	//If current index is a table
	if ( kv->key.type == LITE_TXT && kv->value.type == LITE_TBL && f->depth == 2 ) {
		struct host *w = malloc( sizeof( struct host ) );
		int count = lt_counti( ( st = ((struct fp_iterator *)p)->source ), i );
		FPRINTF( "NAME: %s, COUNT OF ELEMENTS: %d\n", kv->key.v.vchar, count ); 
		nt = loader_shallow_copy( st, i+1, i+count );
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

		w->name = strdup( kv->key.v.vchar );
		if ( !loader_run( nt, rules ) ) {
		}
		add_item( hosts, w, struct host *, &f->len );
	}
	return 1;
}


//Find a host
struct host * find_host ( struct host **hosts, char *hostname ) {
	char host[ 2048 ] = { 0 };
	int pos = memchrat( hostname, ':', strlen( hostname ) );
	memcpy( host, hostname, ( pos > -1 ) ? pos : strlen(hostname) );
	while ( hosts && *hosts ) {
		struct host *req = *hosts;
		if ( req->name && strcmp( req->name, host ) == 0 )  {
			return req;	
		}
		if ( req->alias && strcmp( req->alias, host ) == 0 ) {
			return req;	
		}
		hosts++;
	}
	return NULL;
}


#if 0 
//Build hosts list
struct host ** build_hosts ( zTable *t ) {
	struct host **hosts = NULL;
	struct fp_iterator fp_data = { 0, 0, /*NULL,*/ &hosts, host_table_iterator };
	int index;
	if ( (index = lt_geti( t, "hosts" )) == -1 ) {
		return NULL;
	}

	//fprintf( stderr, "i: %d, host: \t%p => ", index, hosts );
	if ( !lt_exec_complex( t, index, t->count, &fp_data, host_table_iterator ) ) {
		return hosts; 
	}
	//fprintf( stderr, "host: \t%p => ", hosts );
  
#if 0
	while ( hosts && (*hosts) ) {
		fprintf( stderr, "name = '%s'\n", (*hosts)->name );
		hosts++;
	}
#endif
	return hosts; 	
}
#else
struct host ** build_hosts ( zTable *t ) {
	struct host **hosts = NULL;
	const struct rule rules[] = {
		{ "hosts", "t", .v.t = (void ***)&hosts, hosts_iterator }, 
		{ NULL }
	};

	loader_run( t, rules );
	return hosts;
}
#endif


//Free hosts list
void free_hosts ( struct host ** hlist ) {
#if 0
	struct host **hl = hosts;
	while ( hl && *hl ) {
		struct host *h = *hl;
		( h->name ) ? free( h->name ) : 0;
		( h->alias ) ? free( h->alias ) : 0 ;
		( h->dir ) ? free( h->dir ) : 0;
		( h->filter ) ? free( h->filter ) : 0 ;
		( h->ca_bundle ) ? free( h->ca_bundle ) : 0 ;
		( h->certfile ) ? free( h->certfile ) : 0 ;
		( h->keyfile ) ? free( h->keyfile ) : 0 ;
		( h->root_default ) ? free( h->root_default ) : 0;
		free( *hl );
		hl++;
	}
#else
	struct host **hosts = hlist;
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
#endif
	free( hlist );
}



//Debug host list
void dump_hosts ( struct host **hosts ) {
	struct host **r = hosts;
	fprintf( stderr, "Hosts:\n" );
	while ( r && *r ) {
		fprintf( stderr, "\t%p => ", *r );
		fprintf( stderr, "%s =>\n", (*r)->name );
		fprintf( stderr, "\t\tdir = %s\n", (*r)->dir );
		fprintf( stderr, "\t\talias = %s\n", (*r)->alias );
		fprintf( stderr, "\t\tfilter = %s\n", (*r)->filter );
		r++;
	}
}
