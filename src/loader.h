/* ------------------------------------------- * 
 * loader.h 
 * =========
 * 
 * Summary 
 * -------
 * Data structures for loader.
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
#include "../vendor/ztable.h"
#include "util.h"

#ifndef LOADER_H
#define LOADER_H

struct rule {
	const char *key; 
	const char *type;
	union {
		char **s;
		int *i;
		void ***t;
	} v;
	int (*handler)( zKeyval *, int, void * );
};

struct fp_iterator { 
	int len, depth; 
	void *userdata; 
	int (*exec)( zKeyval *, int, void * );
	zTable *source;
};

int loader_get_int_value ( zTable *t, const char *key, int notFound );

char * loader_get_char_value ( zTable *t, const char *key );

int loader_run ( zTable *, const struct rule * ) ;

zTable *loader_shallow_copy( zTable *, int start, int end );

void loader_free( const struct rule * );
#endif
