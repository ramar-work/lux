#include "router.h"

#if 0
static const int RE_NUMBER = 31;
static const int RE_STRING = 32;
static const int RE_ANY    = 33;
static const int ACT_ID   = 34;
static const int ACT_WILDCARD= 35;
static const int ACT_SINGLE  = 36;
static const int ACT_EITHER  = 37;
static const int ACT_RAW = 38;
#endif
static const char NUMS[] = "0123456789";
static const char ALPHA[] = 
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const int maps[] = {
	[':'] = ACT_ID,
	['?'] = ACT_SINGLE,
	['*'] = ACT_WILDCARD,
	['{'] = ACT_EITHER,
	[255] = 0
};

#if 0
void dump_routeh ( struct routeh **v ) {;}
void free_routeh ( struct routeh **v ) {;}
void build_routeh ( struct routeh **v ) {;}
#endif

void dump_routehs ( struct routeh **set ) {
	struct routeh **r = set;
	FPRINTF( "Elements in routes at %p:\n", set );
	while ( r && *r ) {
		FPRINTF( "%s => \n", (*r)->name );
		r++;
	}	
}


//...
static struct element * create_element () {
	struct element *e = NULL;

	if ( !( e = malloc( sizeof( struct element ) ) ) ) {
		return NULL;
	}

	memset( e, 0, sizeof( struct element ));
	return e;
}


//Build a data mapped URI
static struct urimap * build_urimap ( struct urimap *map, const char *uri ) {

	//Set up the memory structure.
	Mem r;
	memset( &r, 0, sizeof( Mem ) );
	//struct urimap *urimap = NULL; 
	struct element **list = NULL;
	int listlen = 0;

	//
	while ( strwalk( &r, uri, "/" ) ) {
		uint8_t *p = (uint8_t *)&uri[ r.pos ];
		struct element *e = NULL;

		//Skip results that are just one '/'
		if ( *p == '/' || !r.size ) {
			continue;
		}

		//Generate a new element
		if ( !( e = create_element() ) ) {
			//The whole map should be freed...
			return NULL;
		}

		//Count all the single characters and length
		if ( maps[ *p ] == ACT_SINGLE ) {
			e->type = ACT_SINGLE;	
		}
		//This can be anything
		else if ( maps[ *p ] == ACT_WILDCARD ) {
			e->type = ACT_WILDCARD;	
		}
		//This should have either one string or another, so build a list
		else if ( maps[ *p ] == ACT_EITHER || maps[ *p ] == ACT_ID ) {
			e->type = ( maps[ *p ] == ACT_EITHER ) ? ACT_EITHER : ACT_ID;
			char *mb = ( maps[ *p ] == ACT_EITHER ) ? ",}" : "=";
			uint8_t *block = p + 1;
			Mem pp;
			memset( &pp, 0, sizeof(Mem) );
			while ( memwalk( &pp, block, (uint8_t *)mb, r.size - 1, strlen(mb) ) ) {
				char *b, buf[ 1024 ] = {0};
				memcpy( buf, &block[ pp.pos ], pp.size );
				b = strdup(buf);
				add_item( &e->string, b, char *, &e->len );
				if ( pp.chr == '}' ) {
					break;
				}
			}

			if ( e->len == 1 ) 
				e->mustbe = RE_ANY;
			else if ( e->len > 1 && e->type == ACT_ID ) {
				//fprintf( stderr, "%s\n", e->string[0] ); getchar(); 
				//fprintf( stderr, "%s\n", e->string[1] ); getchar();
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
			char *b, buf[ 1024 ] = {0};
			memcpy( buf, p, r.size );
			b = strdup( buf );
			add_item( &e->string, b, char *, &e->len );
		}

		if ( e->type ) {
			add_item( &list, e, struct element *, &listlen );
		}
	}

	//Set the urimap and save for later...
	if ( !listlen ) {
		map->name = "/";
		map->listlen = 0;	
		map->list = NULL;	
	}
	else {
		map->name = uri;	
		map->list = list;	
		map->listlen = listlen;
	}
	return map;
}


#ifdef DEBUG_H
void dump_urimap( struct urimap *map ) {
	if ( !map->listlen ) {
		return;
	}

	//This exists just for debugging purposes...
	struct element **a = map->list;
	while ( *a ) {
		FPRINTF( "( string=" ); 
		char **b = (*a)->string;
		if ( (*a)->len ) {
			for ( int i=0; i<(*a)->len; i++ ) { fprintf( stderr, "'%s', ", *b ); b++; }
		}
		fprintf( stderr, 
			" action=%s, be=%s, len=%d )\n", 
			DUMPACTION( (*a)->type ), DUMPMATCH( (*a)->mustbe ), (*a)->len );
		a++;
	}
}
#endif


int compare_urimaps ( struct urimap *map1, struct urimap *map2 ) {

	//...
	struct element **elist = map1->list; 
	struct element **ilist = map2->list;
	FPRINTF( "%s %p (%d elements) & %p (%d elements)\n", __func__, 
		elist, map1->listlen, ilist, map2->listlen );

	if ( map1->listlen != map2->listlen ) {
		FPRINTF( "URI map sizes are different (route = %d, URI = %d).\n", map1->listlen, map2->listlen );
		return 0;
	}

#if 1
	while ( *elist && *ilist ) {
		int action = (*elist)->type;
		if ( action == ACT_SINGLE ) {
		}
		else if ( action == ACT_EITHER ) {
			FPRINTF( "len is: %d\n", (*elist)->len );
			int match = 0;
			char *ii = (*ilist)->string[0];
			for ( int i=0; i < (*elist)->len; i++ ) {
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
			if ( (*elist)->mustbe != RE_ANY ) {
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
#endif
	return 1;
}


//...
struct routeh * resolve_routeh ( struct routeh **rlist, const char *uri ) {
	FPRINTF( "resolve_routeh started...\n" );

	//Throw if uri is blank...
	struct urimap urimap = {0};	
	struct routeh **routes = rlist;

	if ( !uri ) {
		FPRINTF( "No URI specified.\n" );
		return NULL;	
	}

	//Build URI map for the uri
	if ( !build_urimap( &urimap, uri ) ) {
		FPRINTF( "Failed to build URI map for uri '%s'\n", uri );
		return NULL;
	}

	//Check against all the routes now and just return the first match
	while ( routes && *routes ) {
		FPRINTF( "Route '%p' = %s\n", *routes, (*routes)->name );
		struct urimap cmap = {0};	
		char *rname = (*routes)->name;

		//Check that single names or blank uris are not allowed
		FPRINTF( "Comparison of routes '%s' & '%s'\n", uri, (*routes)->name );
		FPRINTF( "strlens '%s' %ld & '%s' %ld\n", 
			uri, strlen(uri), (*routes)->name, strlen((*routes)->name) );

		//check that we're not just looking for root
		if ( strlen( rname ) == 1 && strlen(uri) == 1 && *uri == '/' && *rname == '/' )
			return *routes;
		else if ( strlen( rname ) == 2 && strlen(uri) == 1 && rname[0] == '/' && rname[1] == '/' ) {
			return ( *uri == '/' && *rname == '/' ) ? *routes : NULL;
		}

		//Build URI map for the current route 
		if ( !build_urimap( &cmap, (*routes)->name ) ) {
			FPRINTF( "Failed to build URI map for route '%s'\n", (*routes)->name );
			routes++;
			continue;
		}


		//Can these really match?
		if ( compare_urimaps( &cmap, &urimap ) ) {
			FPRINTF( "SUCCESS: Route %s resolved against '%s'\n", uri, (*routes)->name );
			free_urimap( &cmap ); 
			free_urimap( &urimap ); 
			return (*routes);
		}
	
		//Destroy the cmap
		free_urimap( &cmap ); 
		FPRINTF( "FAILURE: Route %s not resolved against '%s'\n", uri, (*routes)->name );
		routes++;
	}

	free_urimap( &urimap ); 
	FPRINTF( "resolve_routeh ended...\n" );
	return NULL;
}



void free_urimap ( struct urimap *mmap ) {
	//free string and list
	if ( !mmap->listlen ) {
		return;
	}

	struct urimap *map = mmap;
	while ( map->list && *map->list ) {
		//Destroy all allocated strings
		char **str = (*map->list)->string;
		while ( str && *str ) {
			FPRINTF( "check and free str: %s\n", *str );
			free( *str );
			str++;
		}
		free( (*map->list)->string );	

		//Destroy each element
		free( *map->list );
		map->list++;
	}
	//free( mmap->list );
}
