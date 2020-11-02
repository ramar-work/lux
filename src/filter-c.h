#include "http.h"
#include "mime.h"
#include "util.h"
#include "config.h"
#include "render.h"
#include "router.h"

#ifndef FILTER_C_H
#define FILTER_C_H

int filter_c ( struct HTTPBody *, struct HTTPBody *, void * );

typedef zTable * ( *Model )( struct HTTPBody *req, struct HTTPBody *res, int len );

typedef const char * View;

typedef struct c_route {
	const char *route;
	Model models;
	View * views;
} Route;


#endif
