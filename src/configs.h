/* ------------------------------------------- * 
 * configs.h 
 * =========
 * 
 * Summary 
 * -------
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * No entries yet.
 *
 * ------------------------------------------- */
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <ztable.h>
#include "lua.h"
#include "loader.h"
#include "util.h"

#ifndef _WIN32
#else
 #define PATH_MAX 2048
 #define realpath(A, B) _fullpath( B, A, PATH_MAX )
// #define strerror(A) strerror( A )
#endif

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
