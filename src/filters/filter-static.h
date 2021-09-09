/* ------------------------------------------- * 
 * filter-static.h
 * ===========
 * 
 * Summary 
 * -------
 * Header file for Functions comprising the static filter for interpreting 
 * HTTP messages.
 *
 * Usage
 * -----
 * filter-static.c enables Hypno to act as a static file server.
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include <zhttp.h> 
#include "../mime.h"
#include "../util.h"
#include "../configs.h"
#include "../server.h"

#ifndef FILTER_STATIC_H
#define FILTER_STATIC_H

const int filter_static( int, struct HTTPBody *, struct HTTPBody *, struct cdata * );
int check_static_prefix( const char *, const char * );

#endif
