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
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * No entries yet.
 * ------------------------------------------- */
#include "../vendor/zhasher.h"
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
