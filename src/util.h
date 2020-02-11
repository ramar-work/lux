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

#ifdef DEBUG
#define FPRINTF(...) \
	fprintf( stderr, "DEBUG: %s[%d]: ", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ );
#else
#define FPRINTF(...)
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

#endif

uint8_t *read_file ( const char *filename, int *len, char *err, int errlen );
int safeatoi( const char *value );
char *get_lstr( char **str, char chr, int *lt );
char *msg_get_value ( const char *value, const char *chrs, uint8_t *msg, int len );
char *copystr ( uint8_t *src, int len ) ;



