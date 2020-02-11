//Compile me with: 
//gcc -ldl -lpthread -o router vendor/single.o vendor/sqlite3.o router.c && ./router
#include "../vendor/single.h"
#include "util.h"
#include "router.h"

//These are static routes
const char *routes[] = {
  "/"
, "/route"
, "/route/"
, "/{route,julius}"
, "/{route,julius}/:id"
, "/route"
, "/route/:id"
, "/route/:id=string"
, "/route/:id=number"
, "/:id"
, "/route/*"
, "/route/*/jackpot"
#if 0
//Do I want to handle '?'
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
, "/route/bashful"
, "/route/bashful/jackpot"
, "/route/333/jackpot"
#if 0
, "/julius/3"
, "/3"
, "/joseph/route/337"
, "/route/337a"
, "/route/3"
#endif
, NULL
};


int main (int argc, char *argv[]) {
	//It can be from Lua too, I suppose, which ever is easier...
	const char **routelist = routes;
	//We simply want to know whether or not this engine will respond to the request list
	while ( *routelist ) {
		const char **requestList = requests;
		fprintf( stderr,  "Checking against this URI: %s\n", *routelist );
		while ( *requestList ) {
			char *r = resolve( *routelist, *requestList ) ? "YES" : "NO";
			fprintf( stderr, "%20s ?= %-30s: %s\n", *routelist, *requestList, r );
			requestList++;
		}
		routelist++;
	}
}
