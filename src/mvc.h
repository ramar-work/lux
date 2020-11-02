//mvc.h
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
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

#if 0
struct routeh { 
	char *name; 
	struct mvc *mvc;
};
#endif

struct routeh ** build_mvc ( zTable *t );
void dump_mvc ( struct mvc * );
void free_mvc ( struct mvc * );
void dump_routeh ( struct routeh ** );
void free_routeh ( struct routeh ** );

#endif
