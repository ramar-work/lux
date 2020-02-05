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

//These are static routes
const char *routes[] = {
#if 0
  "/"
, "/route"
, "/route/"
  "/route/"
, "/{route,julius}"
, "/{route,julius}/:id"
, "/route"
#endif
  "/route/:id"
, "/route/:id=string"
, "/:id"
#if 0
, "/route/*"
, "/route/*/jackbot"
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
#if 0
  "/"
#endif
 "/2"
#if 0
, "/route"
, "/route/3"
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
	const char nums[] = "0123456789";
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

	//Simply check that everything does what it should
	if ( strlen(route) == 1 && strlen(uri) == 1 ) {
		return ( *uri == '/' && *route == '/' );
	}

	//Generate a little table for the route as is
	//This map only needs to be done once.
	fprintf( stderr, "Checking URI (%s) against route (%s)\n", uri, route );
	const char *rr[] = { route, uri };
	struct element **ll[] = { expectedList, inputList };
	Mem r;

	for ( int ri = 0; ri < ( sizeof( rr ) / sizeof( char *) ); ri ++ ) {
		memset( &r, 0, sizeof( Mem ) );
		const char *ry = rr[ ri ];
		struct element **ly = ll[ ri ];
		while ( strwalk( &r, route, "/" ) ) {
			uint8_t *p = (uint8_t *)&route[ r.pos ];
			int action = maps[ *p ];
			struct element *e = malloc( sizeof( struct element ) );
			memset( e, 0, sizeof( struct element ));
			//Skip results that are just one '/'
			if ( *p == '/' ) {
				continue;
			}
			//Count all the single characters and length
			else if ( maps[ *p ] == ACT_SINGLE ) {
				e->type = ACT_SINGLE;	
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
				while ( memwalk( &pp, block, mb, r.size - 1, strlen(mb) ) ) {
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
			}
			else {
				//This is just some string (I guess the action is RAW)
				e->type = ACT_RAW;
				char buf[ 1024 ] = {0};
				memcpy( buf, p, r.size );
				ADDITEM( strdup( buf ), char *, e->string, e->len );
			}

			if ( e->type ) {
				ADDITEM( e, struct element *, expectedList, elistLen ); 
			}
		}

		ADDITEM( NULL, struct element *, expectedList, elistLen ); 
	}

#if 0
	struct element **a = expectedList;
	fprintf( stderr, "Resolved URL looks like: %s\n", (*a) ? "" : "(none)"  );
	while ( *a ) {
		fprintf( stderr, "action=%s, len=%d, string=", 
			DUMPACTION( (*a)->type ), (*a)->len );
		char **b = (*a)->string;
		if ( (*a)->len ) {
			for ( int i=0; i<(*a)->len; i++ ) {
				fprintf( stderr, "'%s', ", *b );
				b++;
			}
		}
		fprintf( stderr, "\n" );
		a++;
	}

	while ( strwalk( &r, uri, "/" ) {
		//Print the one side and compare
		//Move the list up
	}
#endif

#if 0
	//This should return true at the end provided some situations are true
	memset( &r, 0, sizeof( Mem ) );
	while ( strwalk( &r, uri, "/" ) ) {
		if ( maps[ *p ] == ACT_ID ) {
			//Check for a parameter and a type if expected
		}
		else if ( maps[ *p ] == ACT_SINGLE ) {
			//Check the level for a single character or more
		}
		else if ( maps[ *p ] == ACT_WILDCARD ) {
			//Check for anything?
		}
		else if ( maps[ *p ] == ACT_EITHER ) {
			//Loop through the list of strings
		}
		else {
			//Check that the string matches the string
		}
	}
#endif

	#if 0
	//Finally, we could loop through both and make sure that things match
	//Check the size of both and make sure they match
	routeElements, uriElements;
	while ( *uriElements ) {
		(*routeElements)->a == (*uriElements)->a;
		routeElements++, uriElements++:
	}
	#endif
getchar();
	return 0;	
}


int main (int argc, char *argv[]) {
	//It can be from Lua too, I suppose, which ever is easier...
	const char **routelist = routes;
	//We simply want to know whether or not this engine will respond to the request list
	while ( *routelist ) {
		const char **requestList = requests;
		//fprintf( stderr, "%s resolves:\n", *routelist  );
		while ( *requestList ) {
			char *r = resolve( *routelist, *requestList ) ? "YES" : "NO";
			fprintf( stderr, "%s ?= %s: %s\n", *routelist, *requestList, r );
			requestList++;
		}
		routelist++;
	}
}
