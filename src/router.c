#include "router.h"

#define router_add_item(LIST, ELEMENT, SIZE, LEN) \
	add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

#ifndef DEBUG_H
 
#define DUMPMATCH( NUM )
 #define RDUMPACTION( NUM )
 #define FPRINTF(...)
#else
 #define RDUMPACTION( NUM ) \
	( NUM == ACT_ID    ) ? "ACT_ID" : \
	( NUM == ACT_WILDCARD ) ? "ACT_WILDCARD" : \
	( NUM == ACT_SINGLE   ) ? "ACT_SINGLE" : \
	( NUM == ACT_EITHER   ) ? "ACT_EITHER" : \
	( NUM == ACT_RAW  ) ? "ACT_RAW" : \
	( NUM == ACT_NONE ) ? "ACT_NONE" : "UNKNOWN" 

 #define DUMPMATCH( NUM ) \
	( NUM == RE_NUMBER    ) ? "RE_NUMBER" : \
	( NUM == RE_STRING ) ? "RE_STRING" : \
	( NUM == RE_ANY   ) ? "RE_ANY" : \
	( NUM == RE_NONE ) ? "RE_NONE" : "UNKNOWN" 

 #define FPRINTF(...) \
	fprintf( stderr, __VA_ARGS__ )
#endif


static const char NUMS[] = 
	"0123456789";


static const char ALPHA[] = 
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	;


static const int maps[] = {
	[':'] = ACT_ID,
	['?'] = ACT_SINGLE,
	['*'] = ACT_WILDCARD,
	['{'] = ACT_EITHER,
	[255] = 0
};


//Utility to add to a series of items
static void * add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
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



//Free a URImap structure
static void free_urimap ( struct urimap *mmap ) {
	if ( !mmap->listlen )
		return;

	for ( struct element **e = mmap->list; e && *e; e++ ) {
		for ( char **str = (*e)->string; str && *str; str++ ) {
			free( *str );
		}
		free( (*e)->string );
		free( (*e) );	
	}
	free( mmap->list );
}


char * router_strdup ( char *c ) {
	char *p = malloc( strlen( c ) + 1 ); 
	memset( p, 0, strlen( c ) + 1 );
	memcpy( p, c, strlen( c ) );
	return p; 
}


//Build a data mapped URI
static struct urimap * build_urimap ( struct urimap *map, const char *uri ) {

	//Define
	zWalker r = {0};
	struct element **list = NULL;
	int listlen = 0;

	//TODO: add a condition to return if '/' is the only character
	#if 1
	if ( *uri == '/' && strlen( uri ) == 1 ) {
		map->name = "/", map->listlen = 0, map->list = NULL;	
		return map;
	}
	#endif

	while ( strwalk( &r, uri, "/" ) ) {
		unsigned char *p = (unsigned char *)&uri[ r.pos ];
		struct element *e = NULL;

		//Skip results that are just one '/'
		if ( *p == '/' || !r.size ) {
			continue;
		}

		//Generate a new element
		if ( !( e = create_element() ) ) {
			return NULL;
		}

		//Count all the single characters and length
		if ( maps[ *p ] == ACT_SINGLE )
			e->type = ACT_SINGLE;	
		else if ( maps[ *p ] == ACT_WILDCARD )
			e->type = ACT_WILDCARD;	
		else if ( maps[ *p ] == ACT_EITHER || maps[ *p ] == ACT_ID ) {
			e->type = ( maps[ *p ] == ACT_EITHER ) ? ACT_EITHER : ACT_ID;
			char *mb = ( maps[ *p ] == ACT_EITHER ) ? ",}" : "=";
			unsigned char *block = p + 1;
			zWalker pp;
			memset( &pp, 0, sizeof(zWalker) );
			while ( memwalk( &pp, block, (unsigned char *)mb, r.size - 1, strlen(mb) ) ) {
				char *b, buf[ 1024 ] = {0};
				memcpy( buf, &block[ pp.pos ], pp.size );
				b = router_strdup(buf);
				router_add_item( &e->string, b, char *, &e->len );
				if ( pp.chr == '}' ) {
					break;
				}
			}

			if ( e->len == 1 ) 
				e->mustbe = RE_ANY;
			else if ( e->len > 1 && e->type == ACT_ID ) {
				if ( memcmp( e->string[1], "number", 6 ) == 0 ) 
					e->mustbe = RE_NUMBER;
				else if ( memcmp( e->string[1], "string", 6 ) == 0 ) {
					e->mustbe = RE_STRING;
				}
			}
		}
		else {
			char *b, buf[ 512 ] = {0};
			e->type = ACT_RAW;
			memcpy( buf, p, r.size );
			b = router_strdup( buf );
			router_add_item( &e->string, b, char *, &e->len );
		}

		if ( e->type ) {
			router_add_item( &list, e, struct element *, &listlen );
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


int compare_urimaps ( struct urimap *map1, struct urimap *map2 ) {

	//...
	struct element **elist = map1->list; 
	struct element **ilist = map2->list;
	if ( map1->listlen != map2->listlen ) {
		return 0;
	}

#if 1
	while ( *elist && *ilist ) {
		int action = (*elist)->type;
		if ( action == ACT_SINGLE ) {
		}
		else if ( action == ACT_EITHER ) {
			int match = 0;
			char *ii = (*ilist)->string[0];
			for ( int i=0; i < (*elist)->len; i++ ) {
				char *ee = (*elist)->string[i];
				if ( !ii || !ee ) {
					continue;
				}	
				else if ( strlen(ii) != strlen(ee) ) {
					continue;
				}
				else if ( memcmp( ii, ee, strlen(ii) ) != 0 ) {
					continue;
				}
				match = 1;
			}
			if ( !match ) {
				return 0;
			}	
		}
		else if ( action == ACT_ID ) {
			char *s = (*ilist)->string[0];				
			const char *n = (*elist)->mustbe == RE_STRING ? ALPHA : NUMS;
			int nl = strlen( n );
			if ( !s ) {
				return 0;
			}
			if ( (*elist)->mustbe != RE_ANY ) {
				while ( *s ) {
					if ( !memchr( n, *s, nl ) ) {
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
			if ( !ii ) { 
				return 0;
			}
			else if ( strlen( ii ) != strlen( ee ) ) {
				return 0;
			}
			else if ( memcmp( ii, ee, strlen( ee ) ) != 0 ) {
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



//Stub function to pull text member out of something 
const char * route_rword ( void *f ) {
	return ( const char * )f;
}



//Do a quick single resolution
const char * route_resolve ( const char *uri, const char *rname ) {
	struct urimap urimap = {0}, cmap = {0};	
	int rlen = strlen( rname );
	int ulen = strlen( uri );

	//Build URI map for the uri
	if ( !build_urimap( &urimap, uri ) ) {
		return NULL;
	}

	//check that we're not just looking for root
	if ( rlen == 1 && ulen == 1 && *uri == '/' && *rname == '/' ) {
		free_urimap( &urimap ); 
		return rname;
	}
	else if ( rlen == 2 && ulen == 1 && rname[0] == '/' && rname[1] == '/' ) {
		free_urimap( &urimap ); 
		return ( *uri == '/' && *rname == '/' ) ? rname : NULL;
	}

	//Build URI map for the current route 
	if ( !build_urimap( &cmap, rname ) ) {
		free_urimap( &urimap ); 
		return NULL;	
	}

	//Can these really match?
	if ( compare_urimaps( &cmap, &urimap ) ) {
		free_urimap( &cmap ); 
		free_urimap( &urimap ); 
		return rname;
	}

	//Destroy the cmap
	free_urimap( &cmap ); 
	free_urimap( &urimap ); 
	return NULL;
} 



//Cycle through a list and return something
void * route_complex_resolve ( const char *uri, void **rlist, const char *(*rc)(void *) ) {
	struct urimap urimap = {0};	
	int ulen = !uri ? 0 : strlen( uri );	
	//const char **routes = rlist;

	//Stop when URI is not specified, should return an error
	if ( !uri ) {
		return NULL;	
	}

	//Build URI map for the uri
	if ( !build_urimap( &urimap, uri ) ) {
		return NULL;
	}

	//Check against all the routes now and just return the first match
	while ( rlist && *rlist ) {
		struct urimap cmap = {0};	
		const char *rname = rc( *rlist );
		int rlen = strlen( rname );

		//check that we're not just looking for root
		if ( rlen == 1 && ulen == 1 && *uri == '/' && *rname == '/' ) {
			free_urimap( &urimap ); 
			return *rlist;
		}
		else if ( rlen == 2 && ulen == 1 && rname[0] == '/' && rname[1] == '/' ) {
			free_urimap( &urimap ); 
			return ( *uri == '/' && *rname == '/' ) ? *rlist : NULL;
		}

		//Build URI map for the current route 
		if ( !build_urimap( &cmap, rname ) ) {
			rlist++;
			continue;
		}

		//Can these really match?
		if ( compare_urimaps( &cmap, &urimap ) ) {
			free_urimap( &cmap ); 
			free_urimap( &urimap ); 
			return *rlist;
		}
	
		//Destroy the cmap
		free_urimap( &cmap ); 
		rlist++;
	}

	free_urimap( &urimap ); 
	return NULL;
}



#ifdef DEBUG_H
void dump_urimap( struct urimap *map ) {
	if ( !map->listlen ) {
		return;
	}

	//This exists just for debugging purposes...
	struct element **a = map->list;
	while ( *a ) {
		char **b = (*a)->string;
		if ( (*a)->len ) {
			for ( int i=0; i<(*a)->len; i++ ) { fprintf( stderr, "'%s', ", *b ); b++; }
		}
		fprintf( stderr, 
			" action=%s, be=%s, len=%d )\n", 
			RDUMPACTION( (*a)->type ), DUMPMATCH( (*a)->mustbe ), (*a)->len );
		a++;
	}
}
#endif
