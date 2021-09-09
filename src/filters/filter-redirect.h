/* ------------------------------------------- * 
 * filter-redirect.h
 * =========
 * 
 * Summary 
 * -------
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include <zhttp.h>
#include "../server.h"

const int 
filter_redirect ( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn );
