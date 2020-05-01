#include "../vendor/single.h"
#include "luabind.h"
#include "util.h"
#include "routes.h"
#include "hosts.h"

#ifndef CONFIG_H
#define CONFIG_H

void *get_values ( Table *t, const char *key, void *userdata, int (*fp)(LiteKv *, int, void *) );

struct config {
	const char *path;
	const char *root_default;
	struct route **routes;
	struct host **hosts;
#if 1
	//Questionable as to whether or not I'll need these...
	void *ssl;
	int fd;	
#endif
};

int get_int_value ( Table *t, const char *key, int notFound );
char * get_char_value ( Table *t, const char *key );
char *get_route_key_type ( int num );

struct config * build_config ( char *, char *, int );
void free_config( struct config * );

#endif
