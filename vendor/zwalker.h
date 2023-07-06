/* ------------------------------------------- * 
 * zwalker.h
 * ---------
 * A less error prone way of iterating through strings and unsigned character
 * data.
 *
 * Usage
 * -----
 * Coming soon!
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
 * 12-01-20 - Added memblk functions. 
 *          - Added #define aliases for memblk functions. 
 *          - Added memjump function.
 *
 * ------------------------------------------- */
#ifndef _WIN32
 #define _POSIX_C_SOURCE 200809L
#endif 

#include <stdio.h>
#include <string.h>

#ifndef ZWALKER_H
#define ZWALKER_H

#define strwalk(a,b,c) \
	memwalk(a, (unsigned char *)b, (unsigned char *)c, strlen(b), strlen((char *)c))

#define meminit(mems, p, m) \
	Mem mems; \
	memset(&mems, 0, sizeof(Mem)); \
	mems.pos = p; \
	mems.it = m;

#define memstrocc(blk,str,blklen) \
	memblkocc( blk, str, blklen, strlen( str ) )

#define memstrat(blk,str,blklen) \
	memblkat( blk, str, blklen, strlen( str ) )

#define memstr(blk,str,blklen) \
	memblk( blk, str, blklen, strlen( str ) )


typedef struct zw_t {
	int pos; //Current position within user's block 
	int next; //Position of character found
	int size; //Size of block between current position and character position
	int rsize; 	//...
	unsigned char chr; //Character found
	//Internal pointers
	unsigned char *ptr, *rptr;
	//Current position in block
	unsigned char *src;
} zw_t ;


//Keep me for backwards compat
typedef zw_t zWalker;

int memchrocc (const void *, const char, int);

int memchrat (const void *, const char, int);

int memblkocc (const void *, const void *, int, int);

int memblkat (const void *, const void *, int, int);

void * memblk (const void *, const void *, int, int);

int memtok (const void *, const unsigned char *, int, int );

int memmatch (const void *, const char *, int, char ); 

int memwalk (zw_t *, const unsigned char *, const unsigned char *, const int, const int ) ;

void zwalker_discard_tokens( zw_t * );

void zwalker_init( zw_t * );

int memjump (zw_t *, const unsigned char *, const unsigned char **, const int, const int * );

#endif
