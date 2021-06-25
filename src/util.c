/* ------------------------------------------- * 
 * util.c 
 * =========
 * 
 * Summary 
 * -------
 * General utilities.
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
#include "util.h"


unsigned char * srand_uint8t( unsigned char *src, int srclen, unsigned char *buf, int buflen ) {
	if ( !buf || !memset( buf, 0, buflen ) ) { 
		return NULL;
	}
	
	unsigned char *b = buf;
	srclen -= 1;

	while ( --buflen ) {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		*b = src[ ts.tv_nsec % srclen ];
		b++;
	}
	return buf;
}


//TODO: None of these should take an error buffer.  They are just utilities...
unsigned char *read_file ( const char *filename, int *len, char *err, int errlen ) {
	//Check for and load whatever file
	int fd, fstat, bytesRead, fileSize;
	unsigned char *buf = NULL;
	struct stat sb;
	memset( &sb, 0, sizeof( struct stat ) );

	//Check for the file 
	if ( (fstat = stat( filename, &sb )) == -1 ) {
		//fprintf( stderr, "FILE STAT ERROR: %s\n", strerror( errno ) );
		snprintf( err, errlen, "Stat error on %s: %s\n", filename, strerror( errno ) );
		return NULL;	
	}

	//Check for the file 
	if ( (fd = open( filename, O_RDONLY )) == -1 ) {
		snprintf( err, errlen, "File open error on %s: %s\n", filename, strerror( errno ) );
		return NULL;	
	}

	//Allocate a buffer
	fileSize = sb.st_size + 1;
	if ( !(buf = malloc( fileSize )) || !memset(buf, 0, fileSize)) {
		snprintf( err, errlen, "allocation error: %s, %s\n", filename, strerror( errno ) );
		close( fd );
		return NULL;	
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( (bytesRead = read( fd, buf, sb.st_size )) == -1 ) {
		snprintf( err, errlen, "read error: %s, %s\n", filename, strerror( errno ) );
		free( buf );
		close( fd );
		return NULL;	
	}

	//This should have happened before...
	if ( close( fd ) == -1 ) {
		snprintf( err, errlen, "COULD NOT CLOSE FILE %s: %s\n", filename, strerror( errno ) );
		free( buf );
		return NULL;	
	}

	*len = sb.st_size;
	return buf;
}


//Safely convert numeric buffers...
int * satoi( const char *value, int *p ) {
	//Make sure that content-length numeric
	const char *v = value;
	while ( *v ) {
		if ( (int)*v < 48 || (int)*v > 57 ) return NULL;
		v++;
	}
	*p = atoi( value );
	return p; 
}


//Safely convert numeric buffers...
int * datoi( const char *value ) {
	//Make sure that content-length numeric
	const char *v = value;
	while ( *v ) {
		if ( (int)*v < 48 || (int)*v > 57 ) return NULL;
		v++;
	}

	int *p = malloc( sizeof(int) );
	*p = atoi( value );
	return p; 
}


//Safely convert numeric buffers...
int safeatoi( const char *value ) {
	//Copy to string
	char lc[ 128 ];
	memset( lc, 0, sizeof( lc ) );
	memcpy( lc, value, strlen( value ) );

	//Make sure that content-length numeric
	for ( int i=0; i < strlen(lc); i++ )  {
		if ( (int)lc[i] < 48 || (int)lc[i] > 57 ) {
			return 0;
		}
	}

	return atoi( lc );
}


//Get space between
char *get_lstr( char **str, char chr, int *lt ) {
	//find string, clone string and increment ptr
	int r;
	char *rr = NULL;
	if (( r = memchrat( *str, chr, *lt ) ) == -1 ) {
		rr = malloc( *lt );
		memset( rr, 0, *lt );
		memcpy( rr, *str, *lt );	
		rr[ *lt - 1 ] = '\0';
	}	
	else {
		rr = malloc( r + 1 );
		memset( rr, 0, r );
		memcpy( rr, *str, r );	
		rr[ r ] = '\0';
		*str += r + 1;
		*lt -= r;
	}

	return rr;
}


//Extract value (a simpler code that can be used to grab values)
char *msg_get_value ( const char *value, const char *chrs, unsigned char *msg, int len ) {
	int start=0, end=0;
	char *bContent = NULL;

	if ((start = memstrat( msg, value, len )) > -1) {
		start += strlen( value );
		msg += start;

		//If chrs is more than one character, accept only the earliest match
		int pend = -1;
		while ( *chrs ) {
			end = memchrat( msg, *chrs, len - start );
			if ( end > -1 && pend > -1 && pend < end ) {
				end = pend;	
			}
			pend = end;
			chrs++;	
		}

		//Set 'end' if not already...	
		if ( end == -1 && pend == -1 ) {
			end = len - start; 
		}

		//Prepare for edge cases...
		if ((bContent = malloc( end + 1 )) == NULL ) {
			return ""; 
		}

		//Prepare the raw buffer..
		memset( bContent, 0, end + 1 );	
		memcpy( bContent, msg, end );
	}

	return bContent;
}


//Just copy the key
char *copystr ( unsigned char *src, int len ) {
	len++;
	char *dest = malloc( len );
	memset( dest, 0, len );
	memcpy( dest, src, len - 1 );
	return dest;
}


unsigned char *append_to_uint8t ( unsigned char **dest, int *len, unsigned char *src, int srclen ) {
	if ( !( *dest = realloc( *dest, (*len) + srclen ) ) ) {	
		return NULL;
	}

	if ( !memcpy( &(*dest)[ *len ], src, srclen ) ) {
		return NULL;
	}

	(*len) += srclen;
	return *dest;
}


#if 0
char *append_strings_to_char (char **dest, int *len, char *delim, ... ) {
	char *p = NULL;	
	va_list args;
	va_start( args, delim );
	p = va_arg( args, char * );

	while ( p ) { 
		if ( delim && !append_to_uint8t( (unsigned char **)dest, len, (unsigned char *)delim, strlen( delim ) ) ) {
			return NULL;
		}

		if ( !append_to_uint8t( (unsigned char **)dest, len, (unsigned char *)p, strlen( p ) ) ) {
			return NULL;
		}

		p = va_arg( args, char * );
	} 

	va_end( args );
	return (char *)append_to_uint8t( (unsigned char **)dest, len, (unsigned char *)"\0", 1 );
}
#endif


//Add to series
void * add_item_to_list( void ***list, void *element, int size, int * len ) {
#if 0
	fprintf( stderr, "list => %p, %d, %d\n", *list, (*len), (*len) + 2 );
#endif

	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		FPRINTF( "Failed to reallocate block from %d to %d\n", size, size * ((*len) + 2) ); 
		return NULL;
	}

#if 0
	fprintf( stderr, 
		"Successfully reallocated block to size %d\n", size * ((*len) + 2) ); 
	fprintf( stderr, "list => %p, %d, %d, %d\n", *list, (*len), (*len) + 1, size * ((*len) + 2 ) );
#endif

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}

//Trim any characters 
unsigned char *trim (unsigned char *msg, char *trim, int len, int *nlen) {
	//Define stuff
	unsigned char *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


