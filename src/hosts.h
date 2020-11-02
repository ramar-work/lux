#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "loader.h"
#include "util.h"

#ifndef HOSTS_H
#define HOSTS_H

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

struct host ** build_hosts ( zTable * );
struct host * find_host ( struct host **, char * );
int host_table_iterator ( zKeyval *, int, void * );
void free_hosts ( struct host ** );
void dump_hosts ( struct host ** );

#endif
