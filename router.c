//Compile me with: 
//gcc -ldl -lpthread -o router vendor/single.o vendor/sqlite3.o router.c && ./router
#include "vendor/single.h"

#define ADDITEM(TPTR,SIZE,LIST,LEN) \
	if (( LIST = realloc( LIST, sizeof( SIZE ) * ( LEN + 1 ) )) == NULL ) { \
		fprintf (stderr, "Could not reallocate new rendering struct...\n" ); \
		return NULL; \
	} \
	LIST[ LEN ] = TPTR; \
	LEN++;

#define DUMPACTION( NUM ) \
	( NUM == ACT_ID    ) ? "ACT_ID" : \
	( NUM == ACT_WILDCARD ) ? "ACT_WILDCARD" : \
	( NUM == ACT_SINGLE   ) ? "ACT_SINGLE" : \
	( NUM == ACT_EITHER   ) ? "ACT_EITHER" : \
	( NUM == ACT_RAW  ) ? "ACT_RAW" : "UNKNOWN" 

//#define DDD
#ifdef DDD
#define FPRINTF(...) \
	fprintf( stderr, "DEBUG: %s[%d]: ", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ );
#else
#define FPRINTF(...)
#endif

//These are static routes
const char *routes[] = {
#if 1
  "/"
, "/route"
, "/route/"
, "/{route,julius}"
, "/{route,julius}/:id"
, "/route"
, "/route/:id"
, "/route/:id=string"
, "/:id"
, "/route/*"
, "/route/*/jackbot"
#endif
#if 0
, "/route/?"
, "/route/?accharat"
, "/route/??"
, "/route/???"
//Do I want to handle regular expressions?
//, "/route/[^a-z]"
#endif
, NULL
};

//These are possible request lines
const char *requests[] = {
  "/"
, "/2"
, "/route"
, "/route/3"
#if 0
, "/julius/3"
, "/3"
, "/joseph/route/337"
, "/route/337a"
, "/route/bashful"
, "/route/3"
#endif
, NULL
};


//Return true on whatever resolved?
int resolve ( const char *route, const char *uri ) {
	const int RE_NUMBER = 31;
	const int RE_STRING = 32;
	const int RE_ANY    = 33;
	const int ACT_ID   = 34;
	const int ACT_WILDCARD= 35;
	const int ACT_SINGLE  = 36;
	const int ACT_EITHER  = 37;
	const int ACT_RAW = 38;
	const char *NUMS = "0123456789";
	const char *ALPHA = "abcdefghijklmnopqrstuvwxyz"
			    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const int maps[] = {
		[':'] = ACT_ID,
		['?'] = ACT_SINGLE,
		['*'] = ACT_WILDCARD,
		['{'] = ACT_EITHER,
		[255] = 0
	};

	struct element {
		int type;
		int len;
		int mustbe;
		char **string;
	};

	struct element **expectedList = NULL;
	struct element **inputList = NULL;
	int elistLen = 0;
	int ilistLen = 0;

	//Simply check that everything does what it should
	if ( strlen(route) == 1 && strlen(uri) == 1 ) {
		return ( *uri == '/' && *route == '/' );
	}

	//This is kind of a useless structure, but there is a lot to keep track of
	struct urimap {
		const char *name, *routeset;
		struct element **list;
		int listlen;
		Mem r;
	} urimaps[] = {
		{ "Route", route, NULL, 0, 0 },
		{ "URI", uri, NULL, 0, 0 },
	};


	//Loop through all the things...
	for ( int ri = 0; ri < ( sizeof( urimaps ) / sizeof( struct urimap ) ); ri++ ) {
		struct urimap *map = &urimaps[ ri ];
		memset( &map->r, 0, sizeof( Mem ) );
#if 1
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
				while ( memwalk( &pp, block, mb, map->r.size - 1, strlen(mb) ) ) {
					char buf[ 1024 ] = {0};
					memcpy( buf, &block[ pp.pos ], pp.size );
					ADDITEM( strdup( buf ), char *, e->string, e->len );
					if ( pp.chr == '}' ) {
						break;
					}
				}
				if ( e->len ) {
					ADDITEM( NULL, char *, e->string, e->len );
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
				ADDITEM( strdup( buf ), char *, e->string, e->len );
			}

			if ( e->type ) {
				ADDITEM( e, struct element *, map->list, map->listlen ); 
			}
		}

		ADDITEM( NULL, struct element *, map->list, map->listlen ); 
#endif
	}

#ifdef DDD
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
			char *n = (*elist)->mustbe == RE_STRING ? ALPHA : NUMS;
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


int main (int argc, char *argv[]) {
	//It can be from Lua too, I suppose, which ever is easier...
	const char **routelist = routes;
	//We simply want to know whether or not this engine will respond to the request list
	while ( *routelist ) {
		const char **requestList = requests;
		fprintf( stderr,  "Checking against this URI: %s\n", *routelist );
		while ( *requestList ) {
			char *r = resolve( *routelist, *requestList ) ? "YES" : "NO";
			fprintf( stderr, "%30s ?= %-10s: %s\n", *routelist, *requestList, r );
			requestList++;
		}
		routelist++;
	}
}
