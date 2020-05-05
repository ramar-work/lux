#include "../vendor/single.h"
#ifndef UTIL_H

#define UTIL_H

#define ADDITEM(TPTR,SIZE,LIST,LEN,FAIL) \
	if (( LIST = realloc( LIST, sizeof( SIZE ) * ( LEN + 1 ) )) == NULL ) { \
		fprintf (stderr, "Could not reallocate new rendering struct...\n" ); \
		return FAIL; \
	} \
	LIST[ LEN ] = TPTR; \
	LEN++;

#ifdef DEBUG_H
 #define FPRINTF(...) \
	fprintf( stderr, "DEBUG: %s[%d]: ", __FILE__, __LINE__ ) && \
	fprintf( stderr, __VA_ARGS__ )

 #define add_item(LIST,ELEMENT,SIZE,LEN) \
	fprintf( stderr, "%s[%d]: ", __FILE__, __LINE__ ) && \
	fprintf( stderr, "Adding a new item %p to list: %p\n", ELEMENT, LIST ) && \
		add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )
#else
 #define FPRINTF(...)
 #define add_item(LIST,ELEMENT,SIZE,LEN) \
	add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )
#endif

#define ENCLOSE(SRC, POS, LEN) \
	write( 2, "'", 1 ); \
	write( 2, &SRC[ POS ], LEN ); \
	write( 2, "'\n", 2 );

#define CREATEITEM(TPTR,SIZE,SPTR,HASH,LEN) \
	struct rb *TPTR = malloc( sizeof( SIZE ) ); \
	memset( TPTR, 0, sizeof( SIZE ) ); \
	TPTR->len = LEN;	\
	TPTR->ptr = SPTR; \
	TPTR->hash = HASH; \
	TPTR->rbptr = NULL;

#define srand_nums(BUF,BUFLEN) \
	(char *)srand_uint8t( (uint8_t *)"0123456789", sizeof("0123456789"), (uint8_t *)BUF, BUFLEN )

#define srand_letters(BUF,BUFLEN) \
	(char *)srand_uint8t( (uint8_t *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), (uint8_t *)BUF, BUFLEN )

#define srand_chars(BUF,BUFLEN) \
	(char *)srand_uint8t( (uint8_t *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"), (uint8_t *)BUF, BUFLEN )

#define mrand_nums(BUFLEN) \
	(char *)srand_uint8t( (uint8_t *)"0123456789", sizeof("0123456789"), malloc(BUFLEN), BUFLEN )

#define mrand_chars(BUFLEN) \
	(char *)srand_uint8t( (uint8_t *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"), malloc(BUFLEN), BUFLEN )

#define mrand_letters(BUFLEN) \
	(char *)srand_uint8t( (uint8_t *)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", \
		sizeof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), malloc(BUFLEN), BUFLEN )

#define append_to_char(DEST,DESTLEN,SRC) \
	append_to_uint8t( (uint8_t **)DEST,DESTLEN,(uint8_t *)SRC,strlen(SRC) )

struct fp_iterator { 
	int len, depth; 
	void *userdata; 
};

uint8_t *read_file ( const char *filename, int *len, char *err, int errlen );
int safeatoi( const char *value );
char *get_lstr( char **str, char chr, int *lt );
char *msg_get_value ( const char *value, const char *chrs, uint8_t *msg, int len );
char *copystr ( uint8_t *src, int len ) ;
uint8_t *append_to_uint8t ( uint8_t **, int *, uint8_t *, int ); 
unsigned char * srand_uint8t( uint8_t *, int, uint8_t *, int );
void *add_item_to_list( void ***, void *, int , int * );
char *append_strings_to_char (char **dest, int *len, char *delim, ... );
#endif


