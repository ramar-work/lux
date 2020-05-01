#include "../vendor/single.h"
#include "util.h"

#ifndef HOSTS_H
#define HOSTS_H

struct host {
	char *name;	
	char *alias;
	char *dir;	
	char *filter;	
	char *root_default;	
};

struct host ** build_hosts ( Table * );
struct host * find_host ( struct host **, char * );
int host_table_iterator ( LiteKv *, int, void * );
void free_hosts ( struct host ** );
void dump_hosts ( struct host ** );

#endif