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
#include "../server/server.h"
#include "../config.h"

#ifdef SENDFILE_ENABLED
 #include <sys/sendfile.h>
#endif

#ifndef CTXHTTP_H
#define CTXHTTP_H

int create_notls ( server_t * );
void free_notls ( server_t * );
const int pre_notls ( server_t *, conn_t *);
const int read_notls ( server_t *, conn_t *);
const int write_notls ( server_t *, conn_t *);
const void post_notls ( server_t *, conn_t *);

#endif
