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

//These are static routes
const char *routes[] = {
	"/"
, "/route"
, "/route/"
, "/:id"
, "/{route,julius}/:id"
, "/route/:id"
, "/route/:id=string"
, "/route/*"
, "/route/*/jackbot"
, "/route/?"
, "/route/?accharat"
, "/route/??"
, "/route/???"
//Do I want to handle regular expressions?
//, "/route/[^a-z]"
, NULL
};

//These are possible request lines
const char *requests[] = {
	"/"
, "/2"
, "/route"
, "/route/3"
, "/julius/3"
, "/3"
, "/joseph/route/337"
, "/route/337a"
, "/route/bashful"
, "/route/3"
, NULL
};

//Hmm, is this really a hash table problem in disguise?

int resolve ( const char *route, const char *uri ) {
	const int RE_NUMBER	= 31;
	const int RE_STRING = 32;
	const int RE_ANY    = 33;

	const int ACT_GETID   = 34;
	const int ACT_WILDCARD= 35;
	const int ACT_SINGLE  = 36;
	const int ACT_EITHER  = 37;
	const int ACT_RAW = 38;

	const char nums[] = "0123456789";
	struct element {
		int type;
		int len;
		char **string;
	} element;

	struct element **expectedList = NULL;
	struct element **inputList = NULL;

	const int maps[] = {
		[':'] = ACT_GETID,
		['?'] = ACT_SINGLE,
		['*'] = ACT_WILDCARD,
		['{'] = ACT_EITHER,
		[255] = 0
	};

	//Simply check that everything does what it should
	if ( strlen(route) == 1 && strlen(uri) == 1 ) {
		return ( *uri == '/' && *route == '/' );
	}

	//Generate a little table for the route as is
	//This map only needs to be done once.
fprintf( stderr, "route & uri: %s, %s\n", route, uri );
	Mem r;
	memset( &r, 0, sizeof( Mem ) );
	while ( strwalk( &r, route, "/" ) ) {
		uint8_t *p = (uint8_t *)&route[ r.pos ];
		int action = maps[ *p ];
		struct element *e = malloc( sizeof( struct element ) );
		memset( e, 0, sizeof( struct element ));

fprintf( stderr, "%d, %d => '%c' => %d '\n", r.pos, r.size, *p, maps[*p] );
		if ( maps[ *p ] == ACT_GETID ) {
			//Add the expected parameter and type to ds here
			e->type = ACT_GETID;
			int elist=0;
			ADDITEM( p + 1, char *, e->string, elist );	
		}
		else if ( maps[ *p ] == ACT_GETSINGLE ) {
			//Count all the single characters and length
			e->type = ACT_GETSINGLE;	
		}
		else if ( maps[ *p ] == ACT_GETWILDCARD ) {
			//This can be anything
			e->type = ACT_GETWILDCARD;	
		}
		else if ( maps[ *p ] == ACT_GETEITHER ) {
			//This should have either one string or another, so build a list
			e->type = ACT_GETEITHER;	
		}
		else {
			//This is just some string (I guess the action is RAW)
		}
	/*
		write( 2, &route[ p.pos ], p.size );
		write( 2, "'\n", 2 );
	":", pull an id, and potentially a type
	"*", pull anything, I don't care
	"?", pull one character, I don't care which
	"{", pull either or 
	"[", pull from a list

	after this little thing is made using the source,
	create another one from the thing, or just loop again 
	*/
		
	}

	#if 0
	memset( &p, 0, sizeof( Mem ) );
	while ( strwalk( &p, uri, "/" ) {
		buildElement( ... );
		if ( maps[ *p ] == ACT_GETID ) {
			//Check for a parameter and a type if expected
		}
		else if ( maps[ *p ] == ACT_GETSINGLE ) {
			//Check the level for a single character or more
		}
		else if ( maps[ *p ] == ACT_GETWILDCARD ) {
			//Check for anything?
		}
		else if ( maps[ *p ] == ACT_GETEITHER ) {
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

	return 0;	
}


int main (int argc, char *argv[]) {
	//It can be from Lua too, I suppose, which ever is easier...
	const char **routelist = routes;
	//We simply want to know whether or not this engine will respond to the request list
	while ( *routelist ) {
		const char **requestList = requests;
		fprintf( stderr, "%s resolves:\n", *routelist  );
		while ( *requestList ) {
			char *resolved = resolve( *routelist, *requestList ) ? "YES" : "NO";
			fprintf( stderr, "\t%s %s\n", *requestList, resolved );
			requestList++;
		}
		routelist++;
	}
}
