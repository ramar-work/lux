#include "luabind.h"
#include "loader.h"
#include "../vendor/zhasher.h"

#ifndef LCONFIG_H
#define LCONFIG_H

//Site configs go here
struct host {
	char *name;	
	char *alias;
	char *dir;	
	char *filter;	
	char *root_default;	
	char *ca_bundle;
	char *certfile;
	char *keyfile;
};


//Global config goes here
struct srv_config {
	const char *path;
	const char *root_default;
	struct routeh **routes;
	struct host **hosts;
	zTable *src;
};


struct host * find_host ( struct host **, char * );

int host_table_iterator ( zKeyval *, int, void * );

void free_hosts ( struct host ** );

void dump_hosts ( struct host ** );

struct srv_config * build_server_config ( const char *, char *, int );

void free_server_config( struct srv_config * );
#endif
