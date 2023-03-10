/* ------------------------------------------- * 
 * ctx-http.h
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTP contexts.
 *
 * Usage
 * -----
 * Compile me with: 
 * gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
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
#include <time.h>
#include <zhttp.h>
#include "../socket.h"
#include "../server.h"

#include "../config.h"

#ifndef CTXHTTP_H
#define CTXHTTP_H
void create_notls ( void **p ); 

const int pre_notls ( int, zhttp_t *, zhttp_t *, struct cdata *);

const int read_notls ( int, zhttp_t *, zhttp_t *, struct cdata *);

const int write_notls ( int, zhttp_t *, zhttp_t *, struct cdata *);
#endif
