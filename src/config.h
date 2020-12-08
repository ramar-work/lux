#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "luabind.h"
#include "util.h"
#include "router.h"
#include "hosts.h"

#ifndef CONFIG_H
#define CONFIG_H

void *get_values ( zTable *t, const char *key, void *userdata, int (*fp)(zKeyval *, int, void *) );

struct config {
	const char *path;
	const char *root_default;
	struct routeh **routes;
	struct host **hosts;
	zTable *src;
#if 0
	//Questionable as to whether or not I'll need these...
	void *ssl;
	int fd;	
#endif
};

struct config * build_config ( char *, char *, int );
void free_config( struct config * );

#endif
