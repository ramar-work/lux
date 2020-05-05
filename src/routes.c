#include "routes.h"

#define DUMPACTION( NUM ) \
	( NUM == ACT_ID    ) ? "ACT_ID" : \
	( NUM == ACT_WILDCARD ) ? "ACT_WILDCARD" : \
	( NUM == ACT_SINGLE   ) ? "ACT_SINGLE" : \
	( NUM == ACT_EITHER   ) ? "ACT_EITHER" : \
	( NUM == ACT_RAW  ) ? "ACT_RAW" : "UNKNOWN" 

//This seems like a bad idea...
char *parent[100] = { NULL };
char *handler[100] = { NULL };
int b = 0;
static const int RE_NUMBER = 31;
static const int RE_STRING = 32;
static const int RE_ANY    = 33;
static const int ACT_ID   = 34;
static const int ACT_WILDCARD= 35;
static const int ACT_SINGLE  = 36;
static const int ACT_EITHER  = 37;
static const int ACT_RAW = 38;
static const char *NUMS = "0123456789";
static const char *ALPHA = "abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int maps[] = {
	[':'] = ACT_ID,
	['?'] = ACT_SINGLE,
	['*'] = ACT_WILDCARD,
	['{'] = ACT_EITHER,
	[255] = 0
};

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
	struct route **r = routes;
	while ( r && *r ) {
#if 0
		struct host *h = *hl;
		( h->name ) ? free( h->name ) : 0;
		( h->alias ) ? free( h->alias ) : 0 ;
		( h->dir ) ? free( h->dir ) : 0;
		( h->filter ) ? free( h->filter ) : 0 ;
		( h->root_default ) ? free( h->root_default ) : 0;
		free( *hl );
#endif
		//free_elements( (*r)->elements );
		( (*r)->routename ) ? free( (*r)->routename ) : 0;
		free( *r );
		r++;
	}
	free( r );
}


//...
static void free_urimaps () {

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


static void print_handler ( char *n, char **a ) {
	FPRINTF( "%s print\n", n );
	int i = 0;
#if 1
	for ( int i=0; i<12; i++ ) {
		FPRINTF( "%s[%d]: %s\n", n, i, a[i] );
	}
#else
	while ( a ) {
		FPRINTF( "handler[%d]: %s\n", ++i, *a );
		a++;
	}
#endif
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

#if 1
FPRINTF( "Elements:\n" );
	while ( routes && (*routes) ) {
		FPRINTF( "'%s' => ", (*routes)->routename );
		struct routehandler **h = (*routes)->elements;
		while ( h && *h ) {
			fprintf( stderr, "%s, ", (*h)->filename );
			h++;
		}
		FPRINTF( "\n" );
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
	FPRINTF( "Depth is %d\n", *rdepth );
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
	FPRINTF( "Got key type (%s) & value type (%s)\n", 
		lt_typename( kv->key.type ), lt_typename( kv->value.type ) );

	//Save the key or move table depth
	if ( kv->key.type == LITE_TXT ) {
		name = kv->key.v.vchar;
		//Can I access the handler[ .. ] 
		FPRINTF( "Got name '%s' at routes\n", name ); 
	}
	else if ( kv->key.type == LITE_INT || LITE_FLT ) {
		//name = kv->key.v.vchar;
		FPRINTF( "Got numeric id '%d' at routes\n", kv->key.v.vint ); 
	}
	else if ( kv->key.type == LITE_TRM ) {
		//Safest to add a null member to the end of elements
		if ( (--(*rdepth)) == 0 )
			return 0;	
		else { 
			if ( (*rdepth) == 1 ) {
				for ( int i=0; i < sizeof( parent ) / sizeof( char * ); i++ ) {
					parent[ i ] = NULL; 
					handler[ i ] = NULL;
				}	
			}
		}
	}

#if 1
	if ( kv->value.type == LITE_TXT ) {
		FPRINTF( "Got filename '%s' at %s\n", kv->value.v.vchar, __func__ );
		if ( ( type = handler[ (*rdepth) - 1 ] ) ) {
		#if 1
			struct route *rr = (*routes)[ (*rlen) - 1 ];
		#else
			struct route *rr = routes[ (*rlen) - 1 ];
		#endif
			FPRINTF( "Got type key '%s'\n ", type );
			struct routehandler *h = malloc( sizeof( struct routehandler ) );
			if ( !h || !( h->filename = strdup( kv->value.v.vchar ) ) ) {
				return 0;
			}
			memset( h, 0, sizeof(struct routehandler) );
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
			add_item( &rr->elements, h, struct routehandler *, &rr->elen );
		}
		FPRINTF( "Type: %s\n", type );
	}

	else if ( kv->value.type == LITE_TBL ) {
		//FPRINTF( "Right: %s\n", lt_typename( kv->value.type ) );
		//Only add certain keys, other wise, they're routes...
		if ( name ) {
			if( !memstr( keysstr, name, strlen(keysstr) ) ) {
				int blen = 0;
				struct route *rr = NULL; 
				char *buf = NULL; 
				char *par = parent[ (*rdepth) - 1 ];

				if ( !( rr = malloc( sizeof(struct route) ) ) ) {
					FPRINTF( "Allocation for new route failed.\n" );
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

				append_to_char( &buf, &blen, " " );
				buf[ blen - 1 ] = '\0';
				rr->elen = 0;
				rr->elements = NULL;
				rr->routename = buf;
				FPRINTF( "Got route name (%s), adding to parent index at %d\n", rr->routename, *rdepth ); 
				parent[ (*rdepth) ] = rr->routename;
				add_item( routes, rr, struct route *, rlen );
			}
			else {
				FPRINTF( "Got prepared key: %s\n", name );
				handler[ (*rdepth) ] = strdup( name );
			}
			(*rdepth)++;
			print_handler( "parent", parent );
			print_handler( "handler", handler );
		}
		FPRINTF( "Moving to next row and %d\n", *rdepth );
	}
	getchar();
#endif
	return 1;
}


//Return true on whatever resolved?
int resolve_routes ( const char *route, const char *uri ) {
	FPRINTF( "Checking routes with route = '%s' & uri = '%s'\n", route, uri );
	struct element **expectedList = NULL;
	struct element **inputList = NULL;
	int elistLen = 0;
	int ilistLen = 0;

	//Simply check that everything does what it should
	if ( strlen(route) == 1 && strlen(uri) == 1 ) {
		return ( *uri == '/' && *route == '/' );
	}

	if ( strlen(route) == 2 && strlen(uri) == 1 && route[0] == '/' && route[1] == '/' ) {
		return ( *uri == '/' && *route == '/' );
	}

	struct urimap urimaps[] = {
		{ "Route", route, NULL, 0, { 0 } },
		{ "URI", uri, NULL, 0, { 0 } },
	};

	//Loop through all the things...
	for ( int ri = 0; ri < ( sizeof( urimaps ) / sizeof( struct urimap ) ); ri++ ) {
		struct urimap *map = &urimaps[ ri ];
		memset( &map->r, 0, sizeof( Mem ) );
		while ( strwalk( &map->r, map->routeset, "/" ) ) {
			uint8_t *p = (uint8_t *)&map->routeset[ map->r.pos ];

			//Skip results that are just one '/'
			if ( *p == '/' || !map->r.size ) {
				continue;
			}

			struct element *e = malloc( sizeof( struct element ) );
			memset( e, 0, sizeof( struct element ));

			//Count all the single characters and length
			if ( maps[ *p ] == ACT_SINGLE ) {
				e->type = ACT_SINGLE;	
				//Count all the ?'s and figure out how long it needs to be 
			}
			//This can be anything
			else if ( maps[ *p ] == ACT_WILDCARD ) {
				e->type = ACT_WILDCARD;	
			}
			//This should have either one string or another, so build a list
			else if ( maps[ *p ] == ACT_EITHER || maps[*p] == ACT_ID ) {
				e->type = ( maps[ *p ] == ACT_EITHER ) ? ACT_EITHER : ACT_ID;
				char *mb = ( maps[ *p ] == ACT_EITHER ) ? ",}" : "=";
				uint8_t *block = p + 1;
				Mem pp;
				memset( &pp, 0, sizeof(Mem) );
				while ( memwalk( &pp, block, (uint8_t *)mb, map->r.size - 1, strlen(mb) ) ) {
					char buf[ 1024 ] = {0};
					memcpy( buf, &block[ pp.pos ], pp.size );
					add_item( &e->string, strdup( buf ), char *, &e->len );
					if ( pp.chr == '}' ) {
						break;
					}
				}
				if ( e->len ) {
					; //What is this?
				}
				if ( e->len > 2 && e->type == ACT_ID ) {
					//fprintf( stderr, "%s\n", e->string[1] ); getchar(); exit(0);
					if ( memcmp( e->string[1], "number", 6 ) == 0 ) 
						e->mustbe = RE_NUMBER;
					else if ( memcmp( e->string[1], "string", 6 ) == 0 ) {
						e->mustbe = RE_STRING;
					}
				}
			}
			else {
				//This is just some string (I guess the action is RAW)
				e->type = ACT_RAW;
				char buf[ 1024 ] = {0};
				memcpy( buf, p, map->r.size );
				add_item( &e->string, strdup( buf ), char *, &e->len );
			}

			if ( e->type ) {
				add_item( &map->list, e, struct element *, &map->listlen );
			}
		}
	}

#if 0
	//This exists just for debugging purposes...
	for ( int ri = 0; ri < ( sizeof(urimaps) / sizeof(struct urimap) ); ri++ ) {
		struct urimap *map = &urimaps[ ri ];
		struct element **a = map->list;
		while ( *a ) {
			FPRINTF( "( string=" ); 
			char **b = (*a)->string;
			if ( (*a)->len ) {
				for ( int i=0; i<(*a)->len; i++ ) { fprintf( stderr, "'%s', ", *b ); b++; }
			}
			fprintf( stderr, " action=%s, len=%d )\n", DUMPACTION( (*a)->type ), (*a)->len );
			a++;
		}
	}
#endif

	//Now, do sanity checks
	FPRINTF( "%d ?= %d\n", urimaps[0].listlen, urimaps[1].listlen );
	if ( urimaps[0].listlen != urimaps[1].listlen ) {
		FPRINTF( "URI map sizes are different (route = %d, URI = %d).\n", urimaps[0].listlen, urimaps[1].listlen );
		return 0;
	}

	//Then check that elements match as they should	
	struct element **elist = urimaps[0].list, **ilist = urimaps[1].list;
	while ( *elist && *ilist ) {
		int action = (*elist)->type;
		if ( action == ACT_SINGLE ) {
		}
		else if ( action == ACT_EITHER ) {
			FPRINTF( "len is: %d\n", (*elist)->len );
			int match = 0;
			char *ii = (*ilist)->string[0];
			for ( int i=0; i < ((*elist)->len - 1); i++ ) {
				char *ee = (*elist)->string[i];
				FPRINTF( "Checking '%s' & '%s'\n", ii, ee );
				if ( !ii || !ee ) {
					FPRINTF( "Optional string expected to match '%s', but was empty.\n", ee );
					continue;
				}	
				else if ( strlen(ii) != strlen(ee) ) {
					FPRINTF( "Optional string '%s' expected to match '%s', but is a different length.\n", ii, ee );
					continue;
				}
				else if ( memcmp( ii, ee, strlen(ii) ) != 0 ) {
					FPRINTF( "Optional string '%s' does not match expected string '%s'.\n", ii, ee );
					continue;
				}
				match = 1;
			}
			if ( !match ) {
				return 0;
			}	
		}
		else if ( action == ACT_ID ) {
			FPRINTF( "len is: %d\n", (*elist)->len );
			char *s = (*ilist)->string[0];				
			const char *n = (*elist)->mustbe == RE_STRING ? ALPHA : NUMS;
			int nl = strlen( n );
			if ( !s ) {
				FPRINTF( "String passed to ID was empty.\n" );
				return 0;
			}
			if ( (*elist)->mustbe ) {
				while ( *s ) {
					if ( !memchr( n, *s, nl ) ) {
						FPRINTF( "parameter '%s' did not pass type check.\n", (*ilist)->string[0] );
						return 0;  
					}
					s++;
				}
			}
		}
		else if ( action == ACT_RAW ) {
			//These should just match one to one
			char *ii = *(*ilist)->string;
			char *ee = *(*elist)->string;
			FPRINTF( "Comparing route stubs '%s' & '%s'\n", ii, ee );
			if ( !ii ) { 
				FPRINTF( "Input string expected to match '%s', but was empty.\n", ee );
				return 0;
			}
			else if ( strlen( ii ) != strlen( ee ) ) {
				FPRINTF( "Input string '%s' expected to match '%s', but is a different length.\n", ii, ee );
				return 0;
			}
			else if ( memcmp( ii, ee, strlen( ee ) ) != 0 ) {
				FPRINTF( "Input string '%s' does not match expected string '%s'.\n", ii, ee );
				return 0;
			}
		}
		else if ( action == ACT_WILDCARD ) {
			//This should theoreticlaly never not return...	
		}
		elist++, ilist++;
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
