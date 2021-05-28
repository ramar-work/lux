#include <limits.h>
#include <stdlib.h>
#include <ztable.h>
#include "lua.h"
#include "loader.h"

#ifndef LCONFIG_H
#define LCONFIG_H

//Site configs go here
struct lconfig {
	char *name;	
	char *alias;
	char *dir;	
	char *filter;	
	char *root_default;	
	char *ca_bundle;
	char *cert_file;
	char *key_file;
};


//Global config goes here
struct sconfig {
	char *wwwroot;
	struct lconfig **hosts;
	zTable *src;
};


struct lconfig * find_host ( struct lconfig **, char * );

int host_table_iterator ( zKeyval *, int, void * );

void free_hosts ( struct lconfig ** );

void dump_hosts ( struct lconfig ** );

void dump_sconfig ( struct sconfig * );

struct sconfig * build_server_config ( const char *, char *, int );

void free_server_config( struct sconfig * );
#endif
