#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "util.h"

#ifndef LOADER_H
#define LOADER_H

struct rule {
	const char *key; 
	const char *type;
#if 1
	union {
		char **s;
		int *i;
		void ***t;
	} v;
	int (*handler)( zKeyval *, int, void * );
#endif
};

struct fp_iterator { 
	int len, depth; 
	void *userdata; 
	int (*exec)( zKeyval *, int, void * );
	zTable *source;
};


int loader_run ( zTable *, const struct rule * ) ;
zTable *loader_shallow_copy( zTable *, int start, int end );
void loader_free( const struct rule * );
#endif
