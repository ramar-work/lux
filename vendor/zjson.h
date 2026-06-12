#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ztable.h>
#include <zwalker.h>

#ifndef ZJSON_H
#define ZJSON_H

#ifndef ZJSON_MAX_DEPTH
 #define ZJSON_MAX_DEPTH 100
#endif
 
#ifndef ZJSON_MAX_STATIC_LENGTH
 #define ZJSON_MAX_STATIC_LENGTH 2048
#endif

#define mjson_add_item(LIST,ELEMENT,SIZE,LEN) \
	mjson_add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

/**
 * struct mjson
 *
 * Holds a JSON structure in memory.
 * 
 */
struct mjson {
	unsigned char *value;
	int size;
	char type;
	short index;
};

char * zjson_compress ( const char *, int, int * );

struct mjson ** zjson_decode2 ( const char *, int, char *, int );

int zjson_check_syntax( const char *, int, char *, int );

struct mjson ** zjson_decode ( const char *, int, char *, int );

//TODO: This is a utility function that will be in utilities
struct mjson ** ztable_to_zjson ( ztable_t *, char *, int );

char * zjson_stringify( struct mjson **, char *, int );

int zjson_has_real_values ( struct mjson ** );

void zjson_free ( struct mjson ** );

ztable_t * zjson_to_ztable ( struct mjson **, char *, int) ;

int zjson_check ( const char *, int, char *, int );

unsigned char *zjson_trim ( unsigned char *, char *, int , int * );

#ifdef DEBUG_H
void zjson_dump_item ( struct mjson * );
void zjson_dump ( struct mjson ** );
#endif

#endif
