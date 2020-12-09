/* ------------------------------------------- * 
 * config.h
 * ========
 * 
 * Summary 
 * -------
 * File to deal with parsing global configuration
 * for server.
 *
 * Usage
 * -----
 * Compile me with: 
 * gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
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
 * 
 * ------------------------------------------- */
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
