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
	int (*handler)( LiteKv *, int, void * );
#endif
};

struct fp_iterator { 
	int len, depth; 
	void *userdata; 
	int (*exec)( LiteKv *, int, void * );
	Table *source;
};


int loader_run ( Table *, const struct rule * ) ;
Table *loader_shallow_copy( Table *, int start, int end );
void loader_free( const struct rule * );
#endif
