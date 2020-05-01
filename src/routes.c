#include "routes.h"

//This seems like a bad idea...
char *parent[100] = { NULL };
char *handler[100] = { NULL };
int b = 0;
static const char *keys[] = {
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


//Free route list
void free_routes ( struct route ** routes ) {
}


//Debugging function to show the route key type.
static char *get_route_key_type ( int num ) {
	return \
		( num == BD_VIEW ) ? "BD_VIEW" : \
		( num == BD_MODEL ) ? "BD_MODEL" : \
		( num == BD_QUERY ) ? "BD_QUERY" : \
		( num == BD_CONTENT_TYPE ) ? "BD_CONTENT_TYPE" : \
		( num == BD_RETURNS ) ? "BD_RETURNS" : "UNKNOWN" 
	;
}

//Build routes list
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


//Generate a list of routes
int route_table_iterator ( LiteKv *kv, int i, void *p ) {
	struct fp_iterator *f = (struct fp_iterator *)p;
	struct route ***routes = f->userdata;
	int *rlen = &f->len;
	int *rdepth = &f->depth;
	char *name = NULL;
	char *type = NULL;
  char nbuf[ 64 ] = { 0 };
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

				if ( !( rr = malloc( sizeof(struct route) ) ) ) {
					return 0;
				}

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


//Debug print route list
void dump_routes ( struct route **set ) {
	struct route **r = set;
	fprintf( stderr, "Routes:\n" );
	while ( r && *r ) {
		fprintf( stderr, "\t%p => ", *r );
		fprintf( stderr, "%s => \n", (*r)->routename );
		for ( int ii=0; ii < (*r)->elen; ii++ ) {
			struct routehandler *t = (*r)->elements[ ii ];
			fprintf( stderr, "\t\t{ %s=%s }\n", get_route_key_type(t->type), t->filename );
		}
		r++;
	}	
}
