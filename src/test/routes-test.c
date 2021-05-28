//Compile me with: 
//gcc -ldl -lpthread -o router vendor/single.o vendor/sqlite3.o router.c && ./router
#include "../../vendor/zwalker.h"
#include "../../vendor/ztable.h"
#include "../util.h"
#include "../router.h"

//These are static routes
//const char *routes[] = {
#define ROUTE(v) &(struct routeh){ v }

struct routeh *routes[] = {
	ROUTE( "/" ),
	ROUTE( "//" ),
	ROUTE( "/route" ),
	ROUTE( "/route/" ),
	ROUTE( "/{route,julius}" ),
	ROUTE( "/{route,julius}/:id" ),
	ROUTE( "/route" ),
	ROUTE( "/route/:id" ),
	ROUTE( "/route/:id=string" ),
	ROUTE( "/route/:id=number" ),
	ROUTE( "/:id" ),
	ROUTE( "/route/*" ),
	ROUTE( "/route/*/jackpot" ),
#if 0
//Do I want to handle '?'
, "/route/?"
, "/route/?accharat"
, "/route/??"
, "/route/???"
//Do I want to handle regular expressions?
//, "/route/[^a-z]"
#endif
	NULL
};

//These are possible request lines
//These are the easiest possible tests I've ever written...
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
	struct routeh **rlist = routes;
	struct routeh *resv = NULL;
	const char **urilist = requests;
	while ( *urilist ) {
		fprintf( stderr,  "Checking routes against this URI: %s\n", *urilist );
		//the best way to do this is to run one at a time...
		if ( !( resv = resolve_routeh( rlist, *urilist ) ) ) {
			fprintf( stderr, "Path %s did not resolve.\n", *urilist );		
			urilist++;
			continue;
		}

		fprintf( stderr, "Path %s resolved to name: %s\n", *urilist, resv->name );
		urilist++;
	}

	return 0;
}
