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
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
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

#ifndef CTXHTTP_H
#define CTXHTTP_H
void create_notls ( void **p ); 

const int pre_notls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);

const int read_notls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);

const int write_notls ( int, struct HTTPBody *, struct HTTPBody *, struct cdata *);
#endif
