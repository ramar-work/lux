/* ------------------------------------------- * 
 * zwalker.c
 * ---------
 * A less error prone way of iterating through strings and unsigned character
 * data.
 *
 * Usage
 * -----
 *
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 12-01-20 - Fixed a seek bug in memstr, adding tests
 *          - Added memblk functions. 
 * 12-02-20 - Fixed an off-by-one bug in memblk* functions.
 * 
 * ------------------------------------------- */
#include "zwalker.h"

//Return count of occurences of a character in some block.
int memchrocc (const void *a, const char b, int size) {
	int occ = 0;
	while ( size-- ) {
		*( (const unsigned char *)a++ ) == b ? occ++ : 0; 
	}
	return occ;
}


//Where exactly is a character in memory
int memchrat (const void *a, const char b, int size) {
	int pos = 0, osize = size;
	while ( size-- ) {
		if ( *( (unsigned char *)a++ ) == b ) {
			return pos;
		}
		pos++;
	}
	return ( pos == osize ) ? -1 : pos;
}


//Find a specific string in memory
void * memblk (const void *a, const void *b, int size_a, int size_b) {
	while ( size_a >= size_b ) {
		if ( *(unsigned char *)a == *(unsigned char *)b && !memcmp(a, b, size_b) ) {
			return (void *)a;
		}
		a++, size_a--;
	}
	return NULL;	
}


//Where exactly is a substr in memory
int memblkat (const void *a, const void *b, int size_a, int size_b) {
	int pos = 0;
	while ( size_a >= size_b ) {
		if ( *(unsigned char *)a == *(unsigned char *)b && !memcmp(a, b, size_b) ) {
			return pos;
		}
		a++, pos++, size_a--;
	}
	return -1;	
}


//Return count of occurences of a string in some block.
int memblkocc (const void *a, const void *b, int size_a, int size_b ) {
	int occ = 0;
	while ( size_a >= size_b ) {
		if ( *(unsigned char *)a == *(unsigned char *)b && !memcmp(a, b, size_b) ) {
			occ++, a += size_b, size_a -= size_b;
			continue;	
		}
		a++, size_a--; 
	}
	return occ;
}


//Walk through unsigned character data 
int memwalk ( zw_t *w
	, const unsigned char * data
	, const unsigned char * tokens
	, const int datalen
	, const int toklen )
{
#if 0
fprintf( stderr, "[ %s - %d ] POS: %d, SIZE: %d, NEXT: %d, LEN: %d, TOKLEN: %d\n", 
	__FILE__, __LINE__, w->pos, w->size, w->next, datalen, toklen );
#endif

	//ptr will be the front pointer in our data block as we walk through
	w->ptr = (unsigned char *)( !w->ptr ? data : w->ptr );
	#if 0
	w->src = &data[ w->pos ];
	//w->src = ( !w->src ) ? (unsigned char *)data : ( w->src += w->size ) ; 
	#else
	//src will be the rear pointer in our data block as we walk through
	if ( !w->src )
		w->src = (unsigned char *)data;
	else {
		w->src += w->size;
	}
	#endif

	// Stop if we are at the end of the data
	if ( ( w->pos = w->next ) >= datalen ) { 
		return 0;
	}
#if 0
fprintf( stderr, "datalen - w->pos = %d\n", datalen - w->pos );
getchar();
#endif

	//Find the tokens specified, and bring back that position
	while ( w->next++ < datalen && !memchr( tokens, *(w->ptr++), toklen ) ) { 
		;//fprintf( stderr, "%d, %d\n", w->next, datalen );
	}
#if 0
	//Die if no tokens were found
	if ( w->next == datalen ) {
		//fprintf(stderr, "No tokens found, stopping.\n" );
		return 0;
	}	
#endif
	//TODO: If you want to include the token, specify it...
	w->size = w->next - w->pos;
	w->chr = *( w->ptr - 1 );
	w->rptr = w->ptr - 1;
	return 1; 
}


//"Jump" through unsigned character data (by looking for blocks larger than one character)
int memjump ( zw_t *w
	, const unsigned char * data
	, const unsigned char ** tokens
	, const int datalen
	, const int * toklen )
{
	//Setup the structure
	w->ptr = (unsigned char *)( !w->ptr ? data : w->ptr );
	w->rsize = 0;
	w->pos = w->next;
	if ( ( w->pos = w->next ) == datalen ) {
		return 0;
	}

	//Find the tokens specified, and bring back that position
	int match = 0;
	while ( ( w->next < datalen ) && !match ) {
		const unsigned char **t = tokens;
		const int *l = toklen;
		while ( *t ) {
			//Skip the token if it's too big
			if ( *l > ( datalen - w->next ) ) {
				t++, l++;
				continue;
			}

			//Move up and report our match if we found one
			if ( memchr( *t, *w->ptr, *l ) && memcmp( *t, w->ptr, *l ) == 0 ) {
				w->next += *l, w->ptr += *l, match = 1, w->rsize = *l;
				break;	
			}
			t++, l++;
		}

		if ( match ) {
			break;
		}

		w->ptr++, w->next++;
	}

	//TODO: If you want to include the token, specify it...
	w->size = w->next - w->pos;
	w->chr = *( w->ptr - 1 );
	w->rptr = w->ptr - w->rsize;
	return 1;
}


//Initialize a zw_t structure
void zwalker_init( zw_t *w ) {
	memset( w, 0, sizeof( zw_t ) );
} 


