/* ------------------------------------------------------ *
 * router.h
 * ========
 * 
 * Resolves received routes according to a list of arrays.
 * 
 * You can pass in a custom receiver function if the datatype
 * is not something simple like a list of strings.
 *
 * ------------------------------------------------------ */
#include <stdlib.h>
#include <zwalker.h> 

#ifndef ROUTER_H
#define ROUTER_H

typedef enum {
	RE_NONE = 0,
	RE_NUMBER = 31,
	RE_STRING,
	RE_ANY,
} RouterAction;

typedef enum {
	ACT_NONE = 0,
	ACT_ID = 34,
	ACT_WILDCARD,
	ACT_SINGLE,
	ACT_EITHER,
	ACT_RAW,
} RouterStatus;

struct element {
	int len;
	RouterStatus type;
	RouterAction mustbe;
	char **string;
};

struct urimap {
	const char *name;
	int listlen;
	struct element **list;
};

#if 0
struct kvmap {
	const char key[ 64 ];
	int klen;
	const char value[ 64 ];
	int vlen;
};

typedef struct zRouter {
	const char *uri, *cmpuri;
	struct urimap uurimap;
	struct urimap curimap;
} zRouter;
#endif

const char * route_resolve ( const char *, const char * );
void * route_complex_resolve ( const char *, void **, const char *(*)(void *) );
const char * route_rword( void * );

#define route_resolve_list( a, b ) \
	(const char *)route_complex_resolve( a, (void **)b, route_rword )

#define route_resolve_stringlist( a, b ) \
	(const char *)route_complex_resolve( a, (void **)b, route_rword )
#if 0
//c is the member we want
#define route_resolve( a, b, c ) \
	route_complex_resolve( a, (void **)b, c )
#endif

#ifdef DEBUG_H
void dump_urimap( struct urimap * );
#endif
#endif
