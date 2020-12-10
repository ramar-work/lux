#include <limits.h>
#include <stdlib.h>
#include "luabind.h"
#include "loader.h"
#include "../vendor/zhasher.h"

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
	char *certfile;
	char *keyfile;
};


//Global config goes here
struct sconfig {
	char *path, *wwwroot;
	struct lconfig **hosts;
	zTable *src;
};


struct lconfig * find_host ( struct lconfig **, char * );

int host_table_iterator ( zKeyval *, int, void * );

void free_hosts ( struct lconfig ** );

void dump_hosts ( struct lconfig ** );

struct sconfig * build_server_config ( const char *, char *, int );

void free_server_config( struct sconfig * );
#endif
