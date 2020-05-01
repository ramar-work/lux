#include "../vendor/single.h"
#include "util.h"

#ifndef ROUTES_H
#define ROUTES_H

static const int BD_VIEW = 41;
static const int BD_MODEL = 42;
static const int BD_QUERY = 43;
static const int BD_CONTENT_TYPE = 44;
static const int BD_RETURNS = 45;

struct route { 
	char *routename; 
	char *parent; 
	int elen; 
	struct routehandler { char *filename; int type; } **elements;
};

struct route ** build_routes ( Table * );
void free_routes ( struct route ** );
int route_table_iterator ( LiteKv *, int, void * );

#endif
