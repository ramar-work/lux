#include "../vendor/single.h"
#include "luabind.h"
#include "util.h"

#ifndef CONFIG_H
#define CONFIG_H

void *get_values ( Table *t, const char *key, void *userdata, int (*fp)(LiteKv *, int, void *) );

struct config {
	const char *path;
	const char *root_default;
	struct route **routes;
#if 1
	//Questionable as to whether or not I'll need these...
	void *ssl;
	int fd;	
#endif
};

struct route ** build_routes ( Table *t );
struct host ** build_hosts ( Table *t );
int get_int_value ( Table *t, const char *key, int notFound );
char * get_char_value ( Table *t, const char *key );
#endif
