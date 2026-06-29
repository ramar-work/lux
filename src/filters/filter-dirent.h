/* ------------------------------------------- * 
 * filter-dirent.h
 * ===========
 * 
 * Summary 
 * -------
 * Header file for functions comprising the dirent filter for interpreting 
 * HTTP messages.
 *
 * Usage
 * -----
 * filter-dirent.c allows hypno to act as a directory server, in which the
 * server simply presents the user with a list of files for view or download. 
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
#include <sys/types.h> 
#include <dirent.h> 
#include <zhttp.h>
#include <zrender.h>
#include "../mime.h"
#include "../util.h"
#include "../server.h"
#include "../config.h"

#ifndef FILTER_DIR_H
#define FILTER_DIR_H

const int filter_dirent ( int fd, zhttp_t *, zhttp_t *, struct cdata * );

#endif
