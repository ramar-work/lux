//mvc.h
#include "../vendor/single.h"
#include "util.h"
#include "loader.h"

#ifndef MVC_H
#define MVC_H

struct mvc {
	char **models;
	char **views;
	char **queries;
	char *returns;
	char *auth;
	char *content;
};

struct routeh { 
	char *name; 
	struct mvc *mvc;
};

struct routeh ** build_mvc ( Table *t );
void dump_routeh ( struct routeh ** );
void dump_mvc ( struct mvc * );

#endif
