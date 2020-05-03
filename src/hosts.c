#include "hosts.h"


//Free hosts list
void free_hosts ( struct host ** hosts ) {
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
	free( hosts );
}


//Can be refactored to use function pointers, based on what the types are... 
int host_table_iterator ( LiteKv *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct host ***hosts = f->userdata;
	int *rlen = &f->len;
	int *rdepth = &f->depth;
	char *name = NULL;
	const char *keysstr = 
		"alias" \
		"dir" \
		"filter" \
		"hosts" \
		"root_default" \
		"ca_bundle" \
		"certfile" \
		"keyfile" \
	;

	//Save the key or move table depth
	if ( kv->key.type == LITE_TRM &&  --(*rdepth) == 0 )
		return 0;	
	else if ( kv->key.type == LITE_TXT ) {
		if ( strcmp( name = kv->key.v.vchar, "hosts" ) == 0 && kv->value.type != LITE_TBL )
			return 0;
		else if ( !memstr( keysstr, name = kv->key.v.vchar, strlen( keysstr ) ) ) { 
			struct host * host = malloc( sizeof( struct host ) );
			memset( host, 0, sizeof( struct host ) );
		#if 0
			host->alias = NULL;
			host->dir = NULL;
			host->filter = NULL;
			host->root_default = NULL;
		#endif
			host->name = strdup( kv->key.v.vchar ); 
			add_item( hosts, host, struct host *, rlen );
		}
	}

	//I'd have to mark the last element and a pointer to it
	if ( kv->value.type == LITE_TBL )
		(*rdepth)++;
	else if ( kv->value.type == LITE_TXT && kv->key.type == LITE_TXT ) { 
		char *key = kv->key.v.vchar;
		struct host *host = (*hosts)[ (*rlen) - 1 ];
		strcmp( key, "alias" ) == 0 ? host->alias = strdup( kv->value.v.vchar ) : 0 ;
		strcmp( key, "dir" ) == 0 ? host->dir = strdup( kv->value.v.vchar ) : 0;
		strcmp( key, "filter" ) == 0 ? host->filter = strdup( kv->value.v.vchar ) : 0;
		strcmp( key, "root_default" ) == 0 ? host->root_default = strdup( kv->value.v.vchar ) : 0 ;
		strcmp( key, "ca_bundle" ) == 0 ? host->ca_bundle = strdup( kv->value.v.vchar ) : 0;
		strcmp( key, "certfile" ) == 0 ? host->certfile = strdup( kv->value.v.vchar ) : 0;
		strcmp( key, "keyfile" ) == 0 ? host->keyfile = strdup( kv->value.v.vchar ) : 0;
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

 
//Build hosts list
struct host ** build_hosts ( Table *t ) {
	struct host **hosts = NULL;
	struct fp_iterator fp_data = { 0, 0, &hosts };
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
