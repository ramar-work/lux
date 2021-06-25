/* ------------------------------------------- * 
 * mime.h
 * ======
 * 
 * Summary 
 * -------
 * Header file for functions and data allowing Hypno to deal with different 
 * mimetypes.
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * - 
 * ------------------------------------------- */
#include <inttypes.h>
#include <string.h>

#ifndef MIME_H
#define MIME_H
struct mime { const char *extension, *mimetype; };

const char *mmtref (const char *);
const char *mfiletype_from_mime (const char *);
const char *mmimetype_from_file (const char *);
#endif
