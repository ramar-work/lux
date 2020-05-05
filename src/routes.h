#include "../vendor/single.h"
#include "util.h"

#ifndef ROUTES_H
#define ROUTES_H

static const int BD_VIEW = 41;
static const int BD_MODEL = 42;
static const int BD_QUERY = 43;
static const int BD_CONTENT_TYPE = 44;
static const int BD_RETURNS = 45;

struct routehandler { 
	char *filename; 
	int type; 
}; 

struct route { 
	char *routename; 
	char *parent; 
	int elen; 
	struct routehandler **elements;
};

struct element {
	int type;
	int len;
	int mustbe;
	char **string;
};

//This is kind of a useless structure, but there is a lot to keep track of
struct urimap {
	const char *name, *routeset;
	struct element **list;
	int listlen;
	Mem r;
}; 


struct route ** build_routes ( Table * );
int route_table_iterator ( LiteKv *, int, void * );
void free_routes ( struct route ** );
void dump_routes ( struct route ** );
int resolve_routes ( const char *route, const char *uri );

#endif
