/* ------------------------------------------- * 
 * zjson.c 
 * =======
 * 
 * Summary 
 * -------
 * JSON deserialization / serialization 
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * No entries yet.
 *
 * TESTING
 * -------
 * TBD, but the tests will be shipping with
 * the code in the very near future.
 *
 * ------------------------------------------- */
#include "zjson.h"

#define ZJSON_MAX_KEY_LENGTH 256

#define ZJSON_MAX_VALUE_LENGTH 256

#ifndef DEBUG_H
 #define ZJSON_PRINTF(...)
#else
 #define ZJSON_PRINTF(...) \
		fprintf( stderr, "[%s:%d] ", __FILE__, __LINE__ ) && fprintf( stderr, __VA_ARGS__ )
#endif

struct zjd { int inText, isObject, isVal, index; }; 

//Trim any characters 
unsigned char *zjson_trim 
	( unsigned char *msg, char *trim, int msglen, int *nlen ) {
	//Define stuff
	unsigned char *m = msg, *u = &msg[ msglen ];
	int nl = msglen, sl = strlen( trim );

	//Adjust for end delimiter
	if ( *u == '\0' && !memchr( trim, '\0', sl ) ) {
		u--, nl--;
	}

	//Adjust for empty strings
	if ( !msg || msglen == 0 ) {
		*nlen = 0;
		return m;
	}

	//Adjust for small strings
	if ( msglen == 1 ) {
		*nlen = 1;
		return m;
	}

	//Move backwards
	for ( int s = 0, e = 0; !s || !e; ) {
//ZJSON_PRINTF( "%c -> %c\n", *m, *u );
		if ( !s ) {
			if ( !memchr( trim, *m, sl ) )
				s = 1;
			else {
				nl--, m++;
			}
		}

		if ( !e ) {
			if ( !memchr( trim, *u, sl ) )
				e = 1;
			else {
				nl--, u--;
			}
		}
	}

	*nlen = nl;
	return m;
}

#if 0
//Test for zjson_trim( ... )
int main (int argc, char *argv[]) {
const char *abcd[] = {
	"asfasdfasdfb ;;;",
	"   asfasdfasdfb ;;  >>",
	"asfasdfasdfb ;!;     \t",
	"bacon",
	"",
	NULL
};

for ( const char **a = abcd; *a; a++ ) {
	int l = 0;
	unsigned char * z = zjson_trim( (unsigned char *)*a, " >;\t", strlen( *a ), &l );
	write( 2, "'", 1 );
	write( 2, z, l );
	write( 2, "'", 1 );
	ZJSON_PRINTF( "%d\n", l );
}
return 0;
}
#endif



//Get an approximation of the number of keys needed
static int zjson_count ( unsigned char *src, int len ) {
	int sz = 0;

	//You can somewhat gauge the size needed by looking for all commas
	for ( int c = len; c; c-- ) {
		sz += ( memchr( "{[,]}", *src,  5 ) ) ? 1 : 0, src++;
	}
	return sz;
}




//Check and make sure it's "balanced"
int zjson_check ( const char *str, int len, char *err, int errlen ) {
	int os=0, oe=0, as=0, ae=0;
	for ( int a=0; len; str++, len-- ) {
		//In a string
#if 1
		if ( *str == '"' ) {
			a = !a;
			continue;
		}

		if ( !a ) {
#else
		if ( *str != '"' && ( a = !a ) ) {
#endif
			if ( *str == '{' )
				os++;
			else if ( *str == '[' )
				as++;
			else if ( *str == '}' )
				oe++;
			else if ( *str == ']' ) {
				ae++;
			}
		}
	}

	return ( os == oe && as == ae );
}



struct mjson {
	unsigned char *value;
	int size;
	char type;
	int index; // For debugging, so you can see where things are...
};

char * zjson_compress ( const char *str, int len, int *newlen ) {
	//Loop through the entire thing and "compress" it.
	char *cmp = NULL;
	int marked = 0;
	int cmplen = 0;
	if ( !( cmp = malloc( len ) ) ) {
		return NULL;
	}

	memset( cmp, 0, len );
	for ( char *s = (char *)str, *x = cmp; len; s++, len-- ) {
		//If there is any text, mark it.
		if ( memchr( "'\"", *s, 2 ) ) {
			marked = !marked;
		}
		//If the character is whitespace, skip it
		if ( !marked && memchr( " \r\t\n", *s, 4 ) ) {
			continue;
		}
		*x = *s, x++, cmplen++;
	}

	cmp = realloc( cmp, cmplen + 1 );
	write( 2, cmp, cmplen );
	*newlen = cmplen + 1;
	return cmp;
}

void * mjson_add_item_to_list( void ***, void *, int, int * );

void * mjson_add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}

struct mjson * create_mjson () {
	struct mjson *m = malloc( sizeof ( struct mjson ) );
	memset( m, 0, sizeof( struct mjson ) );
	return m;
}

void zjson_dump ( struct mjson **mjson ) {
	//Dump out the list of what we found
	for ( struct mjson **v = mjson; v && *v; v++ ) {
		fprintf( stderr, "\n[ '%c', %d, ", (*v)->type, (*v)->index );
		if ( (*v)->value && (*v)->size ) {
			fprintf( stderr, "size: %d, ", (*v)->size );
			write( 2, (*v)->value, (*v)->size );
		}
		write( 2, " ]", 3 );
	}
}

#define mjson_add_item(LIST,ELEMENT,SIZE,LEN) \
	mjson_add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

/**
 * zjson_decode2( const char *str, int len, char *err, int errlen )
 *
 * Decodes JSON strings and turns them into something different.
 *
 * 
 */
struct mjson ** zjson_decode2 ( const char *str, int len, char *err, int errlen ) {
	//const char tokens[] = "\"{[}]:, \t\r\n\\"; // this should catch the backslash
	const char tokens[] = "\"{[}]:,\\"; // this should catch the backslash
	int mjson_len = 0;
	int cmplen = 0;
	zWalker w = {0};
	struct mjson **mjson = NULL;
	struct mjson *c = NULL, *d = NULL;

	//"Compress" the JSON string
	char * cmp = zjson_compress( str, len, &cmplen );	

	//Walk through it and deserialize
	for ( int i = 0, text = 0, ind = 0, size = 0; 
		memwalk( &w, (unsigned char *)cmp, (unsigned char *)tokens, cmplen, strlen( tokens ) ); 
	) {
		char *srcbuf, *copybuf, statbuf[ ZJSON_MAX_STATIC_LENGTH ] = { 0 };
		int blen = 0;

	#if 0
		//Show me the characters we're dealing with...
		fprintf( stderr, "'%c' - %d\n", w.chr, w.size );
	#endif

		if ( text && w.chr != '"' ) {
			//fprintf( stderr, "got text, skipping\n" );
			size += w.size;
			continue;
		}
		else if ( text && w.chr == '"' ) {
			//fprintf( stderr, "got end of text, finalizing\n" );
			text = 0;
			size += w.size;
			c->size = size; //Need to add the total size of whatever it may be.
		#if 0
			//Set the thing.
			fprintf( stderr, "type: %c, tab: %d, size: %d, text: ", c->type, c->index, c->size );
			write( 2, "'", 1 );
			write( 2, c->value, c->size );
			write( 2, "'", 1 );
			write( 2, "\n", 1 );
//fprintf( stderr, "got text, continue\n" );getchar();
		#endif
			continue;
		}

		if ( w.chr == '"' ) {
			text = 1;
			size = -1;
			//fprintf( stderr, "type: %c, tab: %d, size: %d\n", 'S', ind, size );
		#if 1
			struct mjson *m = create_mjson();
			m->index = ind;
			m->type = 'S';
			m->value = w.ptr;
			c = m;
		#endif
		}
		else if ( w.chr == '{' || w.chr == '[' ) {
			//fprintf( stderr, "type: %c, tab: %d\n", '{', ++ind );
		#if 1
			struct mjson *m = create_mjson();
			m->type = w.chr;
			m->value = NULL;	
			mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
		#endif
		}
		else {
			// }, ], :, ,
#if 0
			fprintf( stderr, "CHR: Got '%c' ", w.chr );
		fprintf( stderr, "Sequence looks like: " );
		write( 2, "'", 1 ),	write( 2, w.ptr - 128, w.size + 128 ), write( 2, "'\n", 2 );
#endif
			if ( c ) {
			#if 0
				fprintf( stderr, "Got saved word." );
				write( 2, "'", 1 ),	write( 2, c->value, c->size ), write( 2, "'\n", 2 );
			#endif
				mjson_add_item( &mjson, c, struct mjson, &mjson_len );	
				c = NULL;
			}

			//We may not need this after all	
		#if 1
			//Instead of readahead, read backward	& see if we find text
			int len = 0;
			//int numeric = 1;
			unsigned char *p = w.ptr - 2;
			for ( ; ; p-- ) {
				if ( memchr( "{}[]:,'\"", *p, 8 ) ) {
					//fprintf( stderr, "Got a significant character. %c\n", *p );
					break;
				}
				else {
				#if 0
					int a = memchr( "0123456789", *p, 10 ) ? 1 : 0;
					numeric += a;
				#endif
					len += 1;
				}
			}
		#endif

			if ( len ) {
			#if 0
				fprintf( stderr, "NUM: %d vs LEN: %d\n", numeric, len );
			#endif
			#if 0
				write( 2, "'", 1 ), write( 2, p + 1, len ), write( 2, "'", 1 );
				getchar();
			#endif
				//Make a new struct mjson and mark the pointer top
				struct mjson *m = create_mjson();
				m->value = p + 1;
				m->size = len;
				m->type = 'V';
				mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
			}

			//Always save whatever character it might be
			if ( w.chr != ':' && w.chr != ',' ) {
				struct mjson *m = create_mjson();
				m->value = NULL;
				m->type = w.chr; 
				m->size = 0;
				mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
			}

		}
	}
	return mjson;
}


zTable * zjson_to_ztable ( struct mjson **mjson ) {
	//Dump out the list of what we found
	int count = 0;
	for ( struct mjson **v = mjson; v && *v; v++ ) {
		count++;
	}

	fprintf( stderr, "%d\n", count );
	zTable *t = lt_make( count * 2 );

	for ( struct mjson **v = mjson; v && *v; v++ ) {
		struct mjson *m = *v;
		fprintf( stderr, "\n[ '%c', %d, ", (*v)->type, (*v)->index );
		if ( (*v)->value && (*v)->size ) {
			fprintf( stderr, "size: %d, ", (*v)->size );
			write( 2, (*v)->value, (*v)->size );
		}
		write( 2, " ]", 3 );
#if 0
		if ( m->type == '{' ) {
			// make an alpha table
		}
		else if ( m->type == '[' ) {
			// make an num table
		}
		else if ( m->type == '}' ) {
			// end an alpha table

		}
		else if ( m->type == ']' ) {
			// end a num table

		}
		else if ( m->type == 'S' ) {
			// add a string

		}
		else if ( m->type == 'V' ) {
			// add another type of value
			// these can be any type, so check if it's 'true', 'false' or numeric 
			// and respond accordingly

		}
#endif
	}
	return NULL;
}



/**
 * zjson_decode( const char *str, int len, char *err, int errlen )
 *
 * Decodes JSON strings and turns them into a "table".
 *
 * 
 */
zTable * zjson_decode ( const char *str, int len, char *err, int errlen ) {
	const char tokens[] = "\"{[}]:,\\"; // this should catch the backslash
	unsigned char *b = NULL;
	zWalker w = {0};
	zTable *t = NULL;
	struct zjd zjdset[ ZJSON_MAX_DEPTH ], *d = zjdset;
	memset( zjdset, 0, ZJSON_MAX_DEPTH * sizeof( struct zjd ) );
	struct bot { char *key; unsigned char *val; int size; } bot;

	int size = zjson_count( (unsigned char *)str, len );
	if ( size < 1 ) {
		snprintf( err, errlen, "%s", "Got invalid JSON count." );
		return NULL;
	}

	ZJSON_PRINTF( "Got JSON block consisting of roughly %d values\n", size );

	//Return zTable
	if ( !( t = lt_make( size * 2 ) ) ) {
		snprintf( err, errlen, "%s", "Create table failed." );
		return NULL;
	}

	ZJSON_PRINTF( "Creating a table capable of holding %d values\n", size * 2 );

	//Walk through everything
	for ( int i = 0; memwalk( &w, (unsigned char *)str, (unsigned char *)tokens, len, strlen( tokens ) ); ) {
		char *srcbuf, *copybuf, statbuf[ ZJSON_MAX_STATIC_LENGTH ] = { 0 };
		int blen = 0;
#if 1
fprintf( stderr, "%c - %p - %d\n", w.chr, w.src, w.size );
//write( 2, w.src, w.size );write( 2, "\n", 1 );
getchar();
#endif

		if ( w.chr == '"' && !( d->inText = !d->inText ) ) { 
			//Rewind until we find the beginning '"'
			unsigned char *val = w.src;
			int size = w.size - 1, mallocd = 0;
			for ( ; *val != '"'; --val, ++size ) ;
			srcbuf = ( char * )zjson_trim( val, "\" \t\n\r", size, &blen );

			if ( ++blen < ZJSON_MAX_STATIC_LENGTH )
				memcpy( statbuf, srcbuf, blen ), copybuf = statbuf; 
			else {
			#if 0
				snprintf( err, errlen, "%s", "zjson max length is too large." );
				return NULL;
			#else
				if ( !( copybuf = malloc( blen + 1 ) ) ) {
					snprintf( err, errlen, "%s", "zjson out of memory." );
					return NULL;
				}
				memset( copybuf, 0, blen + 1 );
				memcpy( copybuf, srcbuf, blen );
				mallocd = 1;
			#endif
			}
	
			if ( !d->isObject ) {
				lt_addintkey( t, d->index ), lt_addtextvalue( t, copybuf ), lt_finalize( t );
				d->inText = 0, d->isVal = 0;
			}
			else {
				if ( !d->isVal ) {
					ZJSON_PRINTF( "Adding text key: %s\n", copybuf );
					lt_addtextkey( t, copybuf ), d->inText = 0; //, d->isVal = 1;
				}
				else {
					//ZJSON_PRINTF( "blen: %d\n", blen );
					//ZJSON_PRINTF( "ptr: %p\n", copybuf );
					//ZJSON_PRINTF( "Adding text value: %s\n", copybuf );
					ZJSON_PRINTF( "Adding text value: %s\n", copybuf );
					lt_addtextvalue( t, copybuf ), lt_finalize( t );
					ZJSON_PRINTF( "Finalizing.\n" );
					d->isVal = 0, d->inText = 0;
					( mallocd ) ? free( copybuf ) : 0;
				}		 
			}
			continue;
		}

		if ( !d->inText ) {
			if ( w.chr == '{' ) {
				//++i;
				if ( ++i > 1 ) {
					if ( !d->isObject ) {
						//ZJSON_PRINTF( "Adding int key: %d\n", d->index );
						lt_addintkey( t, d->index );
					}
					++d, d->isObject = 1, lt_descend( t );
				}
				else {
					d->isObject = 1;
				}
			}
			else if ( w.chr == '[' ) {
				if ( ++i > 1 ) {
					++d, d->isObject = 0, lt_descend( t );
				}
			}
			else if ( w.chr == '}' ) {
				if ( d->isVal ) {
					int mallocd = 0;
					//This should only run when values are null, t/f or numeric
					srcbuf = ( char * )zjson_trim( w.src, "\t\n\r ", w.size - 1, &blen );

					if ( blen < ZJSON_MAX_STATIC_LENGTH )
						memcpy( statbuf, srcbuf, blen ), copybuf = statbuf; 
					else {
					#if 0
						snprintf( err, errlen, "%s", "zjson max length is too large." );
						return NULL;
					#else
						if ( !( copybuf = malloc( blen + 1 ) ) ) {
							snprintf( err, errlen, "%s", "zjson out of memory." );
							return NULL;
						}
						memset( copybuf, 0, blen + 1 );
						memcpy( copybuf, srcbuf, blen );
						mallocd = 1;
					#endif
					}
					lt_addtextvalue( t, copybuf );
					lt_finalize( t );
					d->isVal = 0, d->inText = 0;
					if ( mallocd ) {
						free( copybuf );
					}
				}
				if ( --i > 0 ) {
					d->index = 0, d->inText = 0;
					--d, d->isVal = 0, lt_ascend( t );
				}
			}
			else if ( w.chr == ']' ) {
				if ( --i > 0 ) {
					d->index = 0, d->inText = 0;
					--d, d->isVal = 0, lt_ascend( t );
				}
			}
			else if ( w.chr == ',' || w.chr == ':' /*|| w.chr == '}'*/ ) {
				( w.chr == ',' && !d->isObject ) ? d->index++ : 0; 
				srcbuf = ( char * )zjson_trim( w.src, "\",: \t\n\r", w.size - 1, &blen );
				if ( blen >= ZJSON_MAX_STATIC_LENGTH ) {
					snprintf( err, errlen, "%s", "Key is too large." );
					//lt_free( t ), free( t );
					return NULL;
				}
				else if ( blen ) {
					memcpy( statbuf, srcbuf, blen + 1 );	
					if ( w.chr == ':' ) {
						ZJSON_PRINTF( "Adding text key: %s\n", statbuf );
						lt_addtextkey( t, statbuf );
					}
					else {
						ZJSON_PRINTF( "Adding text value: %s\n", statbuf );
						lt_addtextvalue( t, statbuf );
						lt_finalize( t );
					}
				}
				d->isVal = ( w.chr == ':' );
			}
		}
	}

	lt_lock( t );
	return t;
}



//Allow deep copying or not...
char * zjson_encode ( zTable *t, char *err, int errlen ) {
	//Define more
	struct ww {
		struct ww *ptr;
		int type, keysize, valsize;
		char *comma, *key, *val, vint[ 64 ];
	} **ptr, *ff, *rr, *br = NULL, *sr[ 1024 ] = { NULL };
	unsigned int tcount = 0, size = 0, jl = 0, jp = 0;
	char * json = NULL;
	const char emptystring[] = "''";

	if ( !t ) {
		snprintf( err, errlen, "Table for JSON conversion not initialized" );
		return NULL;
	}

	if ( !( tcount = lt_countall( t ) ) ) {
		snprintf( err, errlen, "Could not get table count" );
		return NULL;
	}

	//Always allocate tcount + 1, b/c we start processing ahead of time.
	if ( ( size = ( tcount + 1 ) * sizeof( struct ww ) ) < 0 ) {
		snprintf( err, errlen, "Could not allocate source JSON" );
		return NULL;
	}

	//?
	if ( !( br = malloc( size ) ) || !memset( br, 0, size ) ) { 
		snprintf( err, errlen, "Could not allocate source JSON" );
		return NULL;
	}

	//Initialize the first element
	br->type = ZTABLE_TBL, br->val = "{", br->valsize = 1, br->comma = " ";

	//Then initialize our other pointers
	ff = br + 1, rr = br, ptr = sr;

	//Initialize JSON string
	if ( !( json = malloc( 16 ) ) || !memset( json, 0, 16 ) ) {
		snprintf( err, errlen, "Could not allocate source JSON" );
		free( br );
		return NULL;
	}

	//Initialize things
	lt_reset( t );

	//Loop through all values and copy
	for ( zKeyval *kv ; ( kv = lt_next( t ) ); ) {
		zhValue k = kv->key, v = kv->value;
		char kbuf[ 256 ] = { 0 }, vbuf[ 2048 ] = { 0 }, *vv = NULL;
		int lk = 0, lv = 0;
		ff->comma = " ", ff->type = v.type;

		if ( k.type == ZTABLE_NON ) 
			ff->key = NULL, ff->keysize = 0;
		else if ( k.type == ZTABLE_TXT )
			ff->key = k.v.vchar, ff->keysize = strlen( k.v.vchar );
		else if ( k.type == ZTABLE_BLB )
			ff->key = (char *)k.v.vblob.blob, ff->keysize = k.v.vblob.size;
		else if ( k.type == ZTABLE_INT ) {
			ff->key = NULL, ff->keysize = 0;
			if ( rr->type == ZTABLE_TBL && *rr->val == '{' ) {
				rr->val = "[", rr->valsize = 1;
			}
		}
		else if ( k.type == ZTABLE_TRM ) {
			ff->key = NULL, ff->keysize = 0;
			ff->val = ( *(*ptr)->val == '{' ) ? "}" : "]", ff->valsize = 1;
			rr->comma = " ", ff->comma = ",", ff->type = ZTABLE_TRM; 
			ff++, rr++;
			ptr--;
			continue;	
		}
		else {
			snprintf( err, errlen, "Got invalid key type: %s", lt_typename( k.type ) );
			free( br ), free( json );
			return NULL;	
		}

		//TODO: Add rules to replace " in blobs and text
		if ( v.type == ZTABLE_NUL )
			0;
		else if ( v.type == ZTABLE_NON )
			break; 
		else if ( v.type == ZTABLE_INT )
			ff->valsize = snprintf( ff->vint, 64, "%d", v.v.vint ), ff->val = ff->vint, ff->comma = ",";
		else if ( v.type == ZTABLE_FLT )
			ff->valsize = snprintf( ff->vint, 64, "%f", v.v.vfloat ), ff->val = ff->vint, ff->comma = ",";
		else if ( v.type == ZTABLE_TXT ) {
			if ( v.v.vchar ) 	
				ff->val = v.v.vchar, ff->valsize = strlen( v.v.vchar );
			else {
				ff->val = (char *)emptystring; 
				ff->valsize = 2; 
			}
			ff->comma = ",";
		}
		else if ( v.type == ZTABLE_BLB )
			ff->val = (char *)v.v.vblob.blob, ff->valsize = v.v.vblob.size, ff->comma = ",";
		else if ( v.type == ZTABLE_TBL ) {
			ff->val = "{", ff->valsize = 1, ++ptr, *ptr = ff;	
		}
		else { /* ZTABLE_TRM || ZTABLE_NON || ZTABLE_USR */
			snprintf( err, errlen, "Got invalid value type: %s", lt_typename( v.type ) );
			free( br ), free( json );
			return NULL;
		}
		ff++, rr++;
	}

	//No longer null, b/c it exists...
	ff->keysize = -1;

	//TODO: There is a way to do this that DOES NOT need a second loop...
	for ( struct ww *yy = br; yy->keysize > -1; yy++ ) {
		char kbuf[ ZJSON_MAX_STATIC_LENGTH ] = {0}, vbuf[ 2048 ] = {0}, *v = vbuf, vmallocd = 0;
		int lk = 0, lv = 0;

		if ( yy->keysize ) {
			char *k = kbuf;
			// Stop on keys that are just too big...
			if ( yy->keysize >= ZJSON_MAX_STATIC_LENGTH ) {
				snprintf( err, errlen, "Key too large" );
				free( br ), free( json );
				return NULL;
			}

			if ( yy->type != ZTABLE_TRM ) {
				memcpy( k, "\"", 1 ), k++, lk++;
				memcpy( k, yy->key, yy->keysize ), k += yy->keysize, lk += yy->keysize;
				memcpy( k, "\": ", 3 ), k += 3, lk += 3;
			}
		}

		if ( yy->valsize ) {
			char *p = v; 

			// Values can be quite long when encoding
			if ( yy->valsize > ZJSON_MAX_STATIC_LENGTH ) {
				if ( !( p = v = malloc( yy->valsize + 3 ) ) ) {
					snprintf( err, errlen, "Could not claim memory for JSON value." );
					free( br ), free( json );
					return NULL;
				}
				vmallocd = 1;
			}

			memset( p, 0, yy->valsize );
			if ( yy->type == ZTABLE_TBL )
				memcpy( p, yy->val, yy->valsize ), p += yy->valsize, lv += yy->valsize;
			else if ( yy->type == ZTABLE_INT || yy->type == ZTABLE_FLT ) {
				memcpy( p, yy->vint, yy->valsize ), p += yy->valsize, lv += yy->valsize;
				memcpy( p, yy->comma, 1 ), p += 1, lv += 1;
			}
			else if ( yy->type == ZTABLE_TRM ) {
				memcpy( p, yy->val, yy->valsize ), p += yy->valsize, lv += yy->valsize;
				memcpy( p, yy->comma, 1 ), p += 1, lv += 1;
			}
			else {
				memcpy( p, "\"", 1 ), p++, lv++;
				memcpy( p, yy->val, yy->valsize ), p += yy->valsize, lv += yy->valsize;
				memcpy( p, "\"", 1 ), p += 1, lv += 1;
				memcpy( p, yy->comma, 1 ), p += 1, lv += 1;
			}
		}

		//Allocate and re-copy, starting with upping the total size
		jl += ( lk + lv );
		if ( !( json = realloc( json, jl ) ) ) {
			snprintf( err, errlen, "Could not re-allocate source JSON" );
			free( br ), free( json );
			return NULL;
		}

		//Copy stuff (don't try to initialize)
		memcpy( &json[ jp ], kbuf, lk ), jp += lk;
		memcpy( &json[ jp ], v, lv ), jp += lv;
		( vmallocd ) ? free( v ) : 0;
	}

	//This is kind of ugly
	if ( !( json = realloc ( json, jp + 3 ) ) ) {
		snprintf( err, errlen, "Could not re-allocate source JSON" );
		free( br ), free( json );
		return NULL;
	}

	json[ jp - 1 ] = ' '; 
	json[ jp     ] = ( *json == '[' ) ? ']' : '}'; 
	json[ jp + 1 ] = '\0';
	free( br );
	return json;
}


#ifdef ZJSON_TEST

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ztable.h>
#include <zwalker.h>

int main (int argc, char *argv[]) {
	int encode = 0;
	int decode = 0;
	char *arg = NULL;

	if ( argc < 2 ) {
		fprintf( stderr, "Not enough arguments..." );
		fprintf( stderr, "usage: ./zjson [ -decode, -encode ] <file>\n" );
		return 0;
	}

	argv++;
	for ( int ac = argc; *argv; argv++, argc-- ) {
		if ( strcmp( *argv, "-decode" ) == 0 ) {
			decode = 1;
			argv++;
			arg = *argv;
			break;
		}
		else if ( strcmp( *argv, "-encode" ) == 0 ) {
			encode = 1;
			argv++;
			arg = *argv;
			break;
		}
		else {
			fprintf( stderr, "Got invalid argument: %s\n", *argv );
			return 0;
		}
	}

	//Load the whole file if it's somewhat normally sized...
	struct stat sb = { 0 };
	if ( stat( arg, &sb ) == -1 ) {
		fprintf( stderr, "stat failed on: %s: %s\n", arg, strerror(errno) );
		return 0;
	}

	int fd = open( arg, O_RDONLY );
	if ( fd == -1 ) {
		fprintf( stderr, "open failed on: %s: %s\n", arg, strerror(errno) );
		return 0;
	}

	char *con = malloc( sb.st_size + 1 );
	memset( con, 0, sb.st_size + 1 );
	if ( read( fd, con, sb.st_size ) == -1 ) {
		fprintf( stderr, "read failed on: %s: %s\n", arg, strerror(errno) );
		return 0;
	}

	char err[ 1024 ] = {0};
	struct mjson **mjson = NULL; 

	#if 0 
	if ( !zjson_compress( con, sb.st_size, &conlen ) ) {
		fprintf( stderr, "Failed to compress JSON at zjson_compress(): %s", err );
		return 0;
	}
	#endif

	//if ( !( t = zjson_decode( con, sb.st_size, err, sizeof( err ) ) ) ) {
	if ( !( mjson = zjson_decode2( con, sb.st_size, err, sizeof( err ) ) ) ) {
		fprintf( stderr, "Failed to deserialize JSON at json_decode(): %s", err );
		return 0;
	}

	zjson_to_ztable( mjson );

	//lt_reset( t );
	//lt_kfdump( t, 2 );
	free( con );
	return 0;
}
#endif



