//router.h
#include "../vendor/single.h"
#include "util.h"
#include "loader.h"
#include "mvc.h"

#ifndef ROUTER_H
#define ROUTER_H

struct routeh { 
	char *name; 
	struct mvc *mvc;
};

struct element {
	int type;
	int len;
	int mustbe;
	char **string;
};

struct urimap {
	const char *name, *routeset;
	int listlen;
	struct element **list;
	//Mem r;
};

#if 0
void dump_routeh ( struct routeh ** );
void free_routeh ( struct routeh ** );
#endif
void build_routeh ( struct routeh ** );
struct routeh * resolve_routeh ( struct routeh **, const char * );
void dump_routehs ( struct routeh **set ) ;
void free_urimap ( struct urimap * ) ;

#endif
