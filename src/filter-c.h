/* ------------------------------------------- * 
 * filter-c.h
 * -----------
 * Headers for C filter for interpreting HTTP messages.
 *
 * Usage
 * -----
 * Modules written with filter-c in mind are apps written in C that can send
 * an HTTP/HTTPS response back to a server. This is mostly for testing, but it
 * is theoretically possible to deploy a full blown web application in C as a 
 * dynamic library.
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
#include "../vendor/zhttp.h"
#include "mime.h"
#include "util.h"
#include "config.h"
#include "render.h"
#include "router.h"

#ifndef FILTER_C_H
#define FILTER_C_H

int filter_c ( struct HTTPBody *, struct HTTPBody *, void * );

typedef zTable * ( *Model )( struct HTTPBody *req, struct HTTPBody *res, int len );

typedef const char * View;

typedef struct c_route {
	const char *route;
	Model models;
	View * views;
} Route;


#endif
