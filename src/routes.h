#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "loader.h"
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

#if 0
struct rh {
	char **models;
	char **views;
	char **queries;
	char *returns;
	char *auth;
	char *content;
	char *a;
}
#endif

struct route { 
	char *name; 
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
int resolve_routes ( const char *route, const char *uri );
#ifndef DEBUG_H
 #define dump_routes(set)
#else
void dump_routes ( struct route ** );
#endif
#endif
