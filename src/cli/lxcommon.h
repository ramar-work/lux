/**
 * lxcommon.h
 * -------
 *
 * Definitions and structures we'll use with the command line tooling.
 *
 * LICENSE
 * -------
 * Copyright 2020-2024 Antonio R. Collins II (ramar@ramar.work)
 *
 * See LICENSE in the top-level directory for more information.
 *
 *
 */

#define SSH_SUPPRESS_DEPRECATED
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>
#include <dirent.h>
#include "../util.h"
#include "../config.h"

#ifndef LXCOMMON_H
#define LXCOMMON_H

#ifndef DEBUG_H
 #define DPRINTF( ... ) 0 
 #define FDPRINTF(fd, X) 0
 #define FDNPRINTF(fd, X, L) 0
 #define FPRINTF(...)

#else
	/* Removable print statements */
 #define DPRINTF( ... ) fprintf( stderr, __VA_ARGS__ )

	/* Write to a file descriptor */
 #define FDPRINTF(fd, X) write( fd, X, strlen( X ) )

	/* Write a block of length to a file descriptor */
 #define FDNPRINTF(fd, X, L) write( fd, X, L )

	/* Repetition here for the sake of keeping includes easy */
 #define FPRINTF(...) \
		fprintf( stderr, "DEBUG: %s[%d]: ", __FILE__, __LINE__ ) && \
		fprintf( stderr, __VA_ARGS__ )

#endif


/* Print to standard error with additional details */
#define ERRPRINTF(...) \
	fprintf( stderr, "%s: ", NAME ); \
	fprintf( stderr, __VA_ARGS__ ); \
	fprintf( stderr, "%s", "\n" );


/* Instant exit */
#define EXITPRINTF(CODE,...) \
	fprintf( stderr, __VA_ARGS__ ) ? CODE : CODE


/* Strcmp for literals */
#define STRLCMP(H,S) \
	( ( strlen( H ) >= sizeof( S ) - 1 ) && !memcmp( H, S, sizeof( S ) - 1 ) )


/* Save an argument to a (char *) */
#define SAVEARG(x,v) ( v = *( ++x ) )


/* Evaluate both short and long form of a command line flag */
#define EVALARG(x,SHORT,LONG) ( !strcmp( x, SHORT ) || !strcmp( x, LONG ) )


/**
 * typedef struct dir_t
 *
 * File layout structures to easily create a directory from C.
 *
 */
typedef struct dir_t {
	const char *name;
	enum {
		H_DIR = 1
	,	H_FILE
	,	H_BINFILE
	} type;
	const char *path;
	const unsigned char *content;
} dir_t;




/**
 * typedef struct kset_t
 *
 * Structure for finds and replacements.
 *
 * TODO: Retire this and try something else.  It is not
 * straightforward.
 *
 */
typedef struct kset_t {
	const char *key;
	char **ptr;
	int len;
} kset_t;



struct kv {
	int size;
	unsigned char *value;
};



typedef struct strings_t {
	int len;
	const char **strings;
} strings_t;



char *pbasename ( const char * );
struct kv * replace ( unsigned char *, kset_t * ) ;
int create_dirs( const char *, dir_t *, kset_t *, char *, int );

#endif
