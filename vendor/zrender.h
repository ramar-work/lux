/* ------------------------------------------------------- *
 * zrender.h
 * =========
 * 
 * Summary 
 * -------
 * Enables zTables (and eventually other data structures in C)
 * to be used in templating.
 *
 *
 * Usage
 * -----
 * See zrender.c for more.
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
 *
 * CHANGELOG 
 * ---------
 * 01-08-21 - Rewrote entire mapping routine, takes more memory, but far
 * simpler to reason about and less error prone.
 *
 * ------------------------------------------------------- */
#include "zwalker.h"
#include "ztable.h"

#ifndef ZRENDER_H
#define ZRENDER_H

#define zr_add_item(LIST,ELEMENT,SIZE,LEN) \
 add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

#define zr_dupstr(V) \
	(char *)zr_dupblk( (unsigned char *)V, strlen(V) + 1 )

enum {
	RW = 0,
	SX = 32,
	BL = 33,
	LS = 35,
	LE = 47,
	CX = 46,
	EK = 36,
	EX = 96,
	TM = -2,
	UN = 127,
}; 

struct xdesc;
struct xmap;

struct premap {
	unsigned char *ptr;
	int len;
};

struct xdesc {
	int children;
	int index;
	struct xmap *pxmap;
	struct premap **cxmap;
};

struct xmap {
	unsigned char *ptr;
	struct xdesc *parent;
	int len;
	char type, free;
};

typedef struct zRender {
	const char *zStart; 
	const char *zEnd;
	int error;
	char errmsg[1024];

	void *userdata;
	struct premap **premap;	
	struct xmap **xmap;
	unsigned char xmapset[128];
} zRender;


zRender * zrender_init();

void zrender_set_fetchdata( zRender *, void * ); 

void zrender_set_boundaries ( zRender *, const char *, const char * );

void zrender_set( zRender *, const char, short );

const char * zrender_strerror( zRender * );

void zrender_set_default_dialect( zRender * );

int zrender_set_marks( zRender *, const unsigned char *, unsigned int );

int zrender_convert_marks( zRender *);

unsigned char * zrender_interpret( zRender *, unsigned char **, int * );

unsigned char * zrender_render( zRender *, const unsigned char *, int, int * );

void zrender_free( zRender *);

#ifdef DEBUG_H
 #define XMAP_DUMP_LEN 20 
 void print_premap ( struct premap ** );
 void print_xmap ( struct xmap ** );
 const char * print_xmap_type ( short );
#else
 #define XMAP_DUMP_LEN
 #define print_xmap(...)
 #define print_premap(...)
 #define print_xmap_type(...)
#endif

#endif
