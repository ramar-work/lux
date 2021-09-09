/* ------------------------------------------- * 
 * filter-echo.h
 * ===========
 * 
 * Summary 
 * -------
 * Header file for functions comprising the echo filter for stress testing 
 * Hypno's capabilities. 
 *
 * Usage
 * -----
 * filter-echo.c forces hypno to simply echo back what was sent.  This is mostly
 * for testing and has little use for anything but diagnostics.  It can safely
 * be disabled in production.
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
#include "../server.h"

#ifndef FILTER_ECHO_H
#define FILTER_ECHO_H

const int filter_echo ( int, struct HTTPBody *, struct HTTPBody *, struct cdata * );

#endif
