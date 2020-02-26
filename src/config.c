//config.c
//Test parsing Lua config files.
//Each key should return something
//Compile me with: 
//gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
#include "config.h"

struct fp_iterator { int len, depth; void *userdata; };

const char *keys[] = {
	"returns"
,	"content-type"
,	"query"
,	"model"
,	"view"
,	"routes"
,	"hint"
,	"auth"
,	NULL
};

const char *keysstr = 
	"returns" \
	"content-type" \
	"query" \
	"model" \
	"view" \
	"routes" \
 	"hint" \
	"auth"
;

//int c = 0;
char *parent[100] = { NULL };
char *handler[100] = { NULL };
#if 0
static const int BD_VIEW = 41;
static const int BD_MODEL = 42;
static const int BD_QUERY = 43;
static const int BD_CONTENT_TYPE = 44;
static const int BD_RETURNS = 45;
#endif

char *get_route_key_type ( int num ) {
	return \
		( num == BD_VIEW ) ? "BD_VIEW" : \
		( num == BD_MODEL ) ? "BD_MODEL" : \
		( num == BD_QUERY ) ? "BD_QUERY" : \
		( num == BD_CONTENT_TYPE ) ? "BD_CONTENT_TYPE" : \
		( num == BD_RETURNS ) ? "BD_RETURNS" : "UNKNOWN" 
	;
}

void *get_key ( Table *t, const char *key ) {
	int index = lt_geti( t, key );
	int count = lt_counti( t, index );
	FPRINTF( "Key '%s' = %d.  Contains %d element(s).\n", key, index, count );
	//Check what is on the other side if it's not -1
	//If it's string, extract and return it
	//If it's number, ?
	//If it's table, ?
	//Extraction will probably be custom...
	return NULL;
}


#if 0
char ** build_filters ( Table *t ) {
}


char ** build_hosts ( Table *t ) {
}

char ** build_routes ( Table *t ) {
	//Find the routes index, use that as start and move?
	//struct route **routelist = NULL; //This could be static too... less to clean
	struct routeset r = { 0, NULL };
	int routesIndex = lt_geti( t, "routes" );
	int count = lt_counti( t, routesIndex );
	FPRINTF( "routes key at: %d\n", routesIndex );

	//Combine the routes and save each combination (strcmbd?)
	if ( !lt_exec_complex( t, routesIndex, t->count, (void *)&r, buildRoutes ) ) {
		FPRINTF( "At end of route data.\n" );
	}

	for ( int i=0; i < r.len; i++ ) {
		struct route *rr = r.routes[i];
		fprintf( stderr, "[%d] %s => ", i, rr->routename );
		fprintf( stderr, "route composed of %d files.\n", rr->elen );
		for ( int ii=0; ii < rr->elen; ii++ ) {
			struct routehandler *t = rr->elements[ ii ];
			fprintf( stderr, "\t{ %s=%s }\n", DUMPTYPE(t->type), t->filename );
		}
	}
return 0;
}
#endif



void *get_values( Table *t, const char *key, void *ud, int (*fp)(LiteKv *, int, void *)) {
	int index = lt_geti( t, key );
	int count = lt_counti( t, index );

	struct fp_iterator fp_data;
	memset( &fp_data, 0, sizeof( struct fp_iterator ) );
	fp_data.userdata = ud;
	fp_data.depth = 0;
	fp_data.len = 0;

	//FPRINTF( "Key '%s'=%d. (%p). Contains %d element(s).\n", key, index, ud, count );

	if ( count == -1 ) { 
		return NULL;
	}

	if ( !fp ) { 
		//return lt_vta( t, ;
	}
	else {
		if ( !lt_exec_complex( t, index, t->count, &fp_data, fp ) ) {
			return ud; 
		}  
	}

	return NULL;
}


int hosts_table_iterator ( LiteKv *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct host ***hosts = f->userdata;
	int *rlen = &f->len;
	int *rdepth = &f->depth;
	char *name = NULL;
	
	if ( kv->key.type == LITE_TXT ) {
		name = kv->key.v.vchar;	
	}
	else if ( kv->key.type == LITE_TRM ) {

	}

	if ( kv->value.type == LITE_TXT ) {
		fprintf( stderr, "%s", kv->value.v.vchar );
	}
	else if ( kv->value.type == LITE_TBL ) {
		fprintf( stderr, "table" );
	}

	fprintf( stderr, "\n" );
	return 1;
}


//possily a better way to handle these might be a weird const char ** hack
//char[0] could be the type (since for now there are only a few)
int b = 0;

int aa = 0;
int bb = 0;
int route_table_iterator ( LiteKv *kv, int i, void *p ) {

	struct fp_iterator *f = (struct fp_iterator *)p;
	struct route ***routes = f->userdata;
	int *rlen = &f->len;
	int *rdepth = &f->depth;
	char *name = NULL;
	char *type = NULL;
  char nbuf[ 64 ] = { 0 };

	//Save the key or move table depth
	if ( kv->key.type == LITE_TXT )
		name = kv->key.v.vchar;
	/*else if ( kv->key.type == LITE_INT || LITE_FLT )
		name = kv->key.v.vchar;*/
	else if ( kv->key.type == LITE_TRM ) {
		//Safest to add a null member to the end of elements
		//b--;
		if ( (--(*rdepth)) == 0 )
			return 0;	
		else { 
			if ( (*rdepth) == 1 ) {
				for ( int i=0; i < sizeof( parent ) / sizeof( char * ); i++ ) {
					parent[ i ] = NULL, handler[ i ] = NULL;
				}	
			}
		}
	}
	#if 0
	else {
		FPRINTF( "got some other type of key: %s\n", lt_typename( kv->key.type ) ); 
		//snprintf( nbuf, sizeof(nbuf) - 1, "%d", 
	}
	#endif

	if ( kv->value.type == LITE_TXT ) {
		FPRINTF( "filename: %s\n", kv->value.v.vchar );
		if ( ( type = handler[ (*rdepth) - 1 ] ) ) {
		#if 1
			struct route *rr = (*routes)[ (*rlen) - 1 ];
		#else
			struct route *rr = routes[ (*rlen) - 1 ];
		#endif
			struct routehandler *h = malloc( sizeof( struct routehandler ) );
			if ( !h || !( h->filename = strdup( kv->value.v.vchar ) ) ) 
				return 0;
			if ( memcmp( "model", type, 3 ) == 0 )
				h->type = BD_MODEL;
			else if ( memcmp( "view", type, 3 ) == 0 )
				h->type = BD_VIEW;
			else if ( memcmp( "query", type, 3 ) == 0 )
				h->type = BD_QUERY;
			else if ( memcmp( "content", type, 3 ) == 0 )
				h->type = BD_CONTENT_TYPE;
			else if ( memcmp( "returns", type, 3 ) == 0 )
				h->type = BD_RETURNS;
			else {
				//This isn't valid, so drop it...
				//return 0;
			}
			add_item( &rr->elements, h, struct routetype *, &rr->elen );
		}
	}

	else if ( kv->value.type == LITE_TBL ) {
		//FPRINTF( "Right: %s\n", lt_typename( kv->value.type ) );
		//Only add certain keys, other wise, they're routes...
		if ( name ) {
			if( !memstr( keysstr, name, strlen(keysstr) ) ) {
				int blen = 0;
				struct route *rr = NULL; 
				char *buf = NULL, *par = parent[ b - 1 ];

				if ( !( rr = malloc( sizeof(struct route) ) ) )
					return 0;

			#if 0
				for ( char **p = (char *[]){ par ? par : "1","/",name,NULL }; p; p++ ) {
					( **p != '1' ) ? append_to_char( &buf, &blen, *p ) : 0;	
				}
			#else
				if ( !par ) {
					append_to_char( &buf, &blen, "/" );
					append_to_char( &buf, &blen, name );
				}
				else {
					append_to_char( &buf, &blen, par );
					append_to_char( &buf, &blen, "/" );
					append_to_char( &buf, &blen, name );
				}
			#endif

				buf[ blen ] = '\0';
				rr->elen = 0;
				rr->elements = NULL;
				rr->routename = buf;
				parent[ (*rdepth) ] = rr->routename;

				//FPRINTF( "Got route name (%s), saving to: %p->%p\n", name, rr, routes );
			#if 0
				add_item( routes, rr, struct route *, rlen );
			#else
				add_item( routes, rr, struct route *, rlen );
			#endif
			}
			else {
				FPRINTF( "Got prepared key: %s\n", name );
				handler[ (*rdepth) ] = strdup( name );
			}
			(*rdepth)++;
		}
	}
	return 1;
}


int get_int_value ( Table *t, const char *key, int notFound ) {
	int i = lt_geti( t, key );
	LiteRecord *p = NULL;
	if ( i == -1 ) {
		return notFound;
	}

	if (( p = lt_ret( t, LITE_INT, i ))->vint == 0 ) {
		return notFound;
	}

	return p->vint;
}


char * get_char_value ( Table *t, const char *key ) {
	int i = lt_geti( t, key );
	LiteRecord *p = NULL;
	if ( i == -1 ) {
		return NULL;
	}

	if (( p = lt_ret( t, LITE_TXT, i ))->vchar == NULL ) {
		return NULL;
	}

	return p->vchar;
}



struct host ** build_hosts ( Table *t ) {
	struct host **hosts = NULL;
	if ( !get_values( t, "hosts", &hosts, hosts_table_iterator ) ) {
		return NULL;
	}
#if 1
	while ( hosts && (*hosts) ) {
		fprintf( stderr, "'%s' => ", (*hosts)->name );
		hosts++;
	}
#endif
	return hosts; 	
}


struct route ** build_routes ( Table *t ) {
	struct route **routes = NULL;
	struct fp_iterator fp_data = { 0, 0, &routes };
	int index;

	if ( (index = lt_geti( t, "routes" )) == -1 ) {
		return NULL;
	}

	//TODO: This can fail, so I need to catch it.
	if ( !lt_exec_complex( t, index, t->count, &fp_data, route_table_iterator ) ) {
		return routes; 
	}  

#if 0
	while ( routes && (*routes) ) {
		fprintf( stderr, "%s, %d: '%s' => ", __FILE__, __LINE__, (*routes)->routename );
		struct routehandler **h = (*routes)->elements;
		while ( h && *h ) {
			fprintf( stderr, "%s, ", (*h)->filename );
			h++;
		}
		fprintf( stderr, "\n" );
		routes++;
	}
#endif
	return routes; 	
}


void free_hosts () {
}

void free_routes () {
}
