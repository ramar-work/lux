/**
 * util.h
 * =======
 *
 * Summary
 * -------
 * Shared utilties for filters.
 *
 * LICENSE
 * -------
 * Copyright 2020-2026 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 */
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <zhttp.h>
#include <zmime.h>
#include <ztable.h>
#include "../util.h"

// TODO: If count is successfully rewritten, use that.  If not, allow this value to change.
#define LT_SIZE_MAX 128

#define CACHECONTROL 1

#ifdef CACHECONTROL
#include <dirent.h>
#include "../lua.h"
#endif

#ifndef FILTERUTIL_H
#define FILTERUTIL_H

// Gnarly, but this works...
#define concat(BUF,BUFLEN,...) \
	( ( snprintf(BUF, BUFLEN, __VA_ARGS__ ) < 0 || snprintf(BUF, BUFLEN, __VA_ARGS__ ) >= BUFLEN ) == 0 )

#define extr_key(z,k,l,i,o,e,el) \
	extract_key(z,k,l,i,(void **)o,e,el)

typedef struct confkey_t {
	const char *name;
	const short type;
	const short required;
	void * (*exec)( ztable_t *, int, const void *, void **, char *, int );	
} confkey_t;

char * getpath( char *, char *, int );
const int http_error( zhttp_t *, int, char *, ... );
const int send_static ( zhttp_t *, const char *, char *, int );
ztable_t * load_luaconf( const char *, char *, int ) ;
int extract_key (	ztable_t *, const char *, const confkey_t *, const void *, void **, char *, int );
int check_conf_keys ( ztable_t *, const confkey_t *, const char *, char *, int );
void *get_cache_header ( ztable_t *, int, const void *, void **, char *, int );
void *get_disallowed_paths( ztable_t *, int, const void *, void **, char *, int );
void *get_redirect ( ztable_t *, int, const void *, void **, char *, int );

#endif
