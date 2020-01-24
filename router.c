//Compile me with: 
//gcc -ldl -lpthread -o router vendor/single.o vendor/sqlite3.o router.c && ./router
#include "vendor/single.h"

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
, "/route/?"
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

	const int A_GETID   = 34;
	const int A_WILDCARD= 34;
	const int A_SINGLE  = 34;
	const int A_EITHER  = 34;

	const char nums[] = "0123456789";
	struct Element {
		int type;
		int len;
		char *string;
	} element;

	//Simply check that everything does what it should
	if ( strlen(route) == 1 && strlen(uri) == 1 ) {
		return ( *uri == '/' && *route == '/' );
	}

	//Generate a little table for the route as is
fprintf( stderr, "route & uri: %s, %s\n", route, uri );
	Mem p;
	memset( &p, 0, sizeof( Mem ) );
	while ( strwalk( &p, route, "/" ) ) {
fprintf( stderr, "%d, %d => '", p.pos, p.size );
		write( 2, &route[ p.pos ], p.size );
		write( 2, "'\n", 2 );
	/*
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
