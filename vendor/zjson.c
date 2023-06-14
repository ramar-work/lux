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
 * ------------------------------------------- */
#include "zjson.h"

#define ZJSON_MAX_KEY_LENGTH 256

#define ZJSON_MAX_VALUE_LENGTH 256

#define ZJSON_TERMINATOR -2

#define ZJSON_FREE_VALUE -1

#ifndef DEBUG_H
 #define ZJSON_PRINTF(...)
#else
 #define ZJSON_PRINTF(...) \
		fprintf( stderr, "[%s:%d] ", __FILE__, __LINE__ ) && fprintf( stderr, __VA_ARGS__ )
#endif

struct zjd { int inText, isObject, isVal, index; }; 

//Trim any characters 
unsigned char *zjson_trim ( unsigned char *msg, char *trim, int msglen, int *nlen ) {
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



#if 0
//Get an approximation of the number of keys needed
static int zjson_count ( unsigned char *src, int len ) {
	int sz = 0;

	//You can somewhat gauge the size needed by looking for all commas
	for ( int c = len; c; c-- ) {
		sz += ( memchr( "{[,]}", *src,  5 ) ) ? 1 : 0, src++;
	}
	return sz;
}


/**
 * zjson_decode( const char *str, int len, char *err, int errlen )
 *
 * Decodes JSON strings and turns them into a "table".
 *
 */
ztable_t * zjson_decode ( const char *str, int len, char *err, int errlen ) {
	const char tokens[] = "\"{[}]:,\\"; // this should catch the backslash
	unsigned char *b = NULL;
	zWalker w = {0};
	ztable_t *t = NULL;
	struct zjd zjdset[ ZJSON_MAX_DEPTH ], *d = zjdset;
	memset( zjdset, 0, ZJSON_MAX_DEPTH * sizeof( struct zjd ) );
	struct bot { char *key; unsigned char *val; int size; } bot;

	int size = zjson_count( (unsigned char *)str, len );
	if ( size < 1 ) {
		snprintf( err, errlen, "%s", "Got invalid JSON count." );
		return NULL;
	}

	ZJSON_PRINTF( "Got JSON block consisting of roughly %d values\n", size );

	//Return ztable_t
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


/**
 * zjson_encode( ztable_t *t, char *err, int errlen )
 *
 * Converts from ztable_t to JSON string.
 * 
 */
char * zjson_encode ( ztable_t *t, char *err, int errlen ) {
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
#endif


/**
 * zjson_check_syntax( const char *str, int len, int *newlen )
 *
 * Checks for syntax errors. 
 * 
 */
int zjson_check_syntax( const char *str, int len, char *err, int errlen ) { 
	//
	char tk = 0, *start = NULL;
	int nlen = len, arr = 0, text = 0,  p = 0;

	//Die if the first non-whitespace character is not a valid JSON object token
	for ( char *s = (char *)str; nlen; s++, nlen-- ) {
		if ( memchr( " \r\t\n", *s, 4 ) ) {
			continue;
		}
	
		if ( ( tk = *s ) == '{' || tk == '[' ) {
			start = s;
			break;
		}
		else {	
			snprintf( err, errlen, "%s: %c\n", "Got invalid JSON object token at start", tk );
			return 0;	
		}
	}

	//We will also need to backtrack and make sure that we have properly closing pairs
	for ( char *s = (char *)&str[ len - 1 ]; ; s--, nlen-- ) {
		if ( memchr( " \r\t\n", *s, 4 ) ) {
			continue;
		}

		if ( ( tk == '{' && *s == '}' ) || ( tk == '[' && *s == ']' ) )
			break;
		else {	
			snprintf( err, errlen, "%s: %c\n", "Got invalid JSON object token at end", *s );
			return 0;	
		}
	}

	for ( char w = 0, *s = (char *)str; p < len; s++, p++ ) {
		if ( !text && memchr( "{}[]:", *s, 5 ) ) {
			//Check for invalid object or array closures
			//TODO: There MUST be a way to clean this up...
			if ( ( w == '{' && ( *s == ']' || *s == '[' ) ) ) {
				snprintf( err, errlen, "%s '%c' at position %d\n", "Got invalid JSON sequence", *s, p );
				return 0;	
			}

			if ( ( w == '[' && ( *s == '}' || *s == ':' ) ) ) {
				snprintf( err, errlen, "%s '%c' at position %d\n", "Got invalid JSON sequence", *s, p );
				return 0;	
			}

			//Do not increment if it's a ':'
			if ( ( w = *s ) != ':' ) {
				arr++;
			}
		}
		else if ( !text && *s == '\'' ) {
			snprintf( err, errlen, "%s '%c' at position %d\n", "JSON does not allow single quotes for strings", *s, p );
			return 0;	
		}
		else if ( !text && memchr( " \r\t\n", *s, 4 ) ) {
			continue;
		}
		else if ( *s == '"' && *( s - 1 ) != '\\' ) {
			arr++, text = !text;	
		}
	}

	//If it's divisible by 2, we're good... if not, we're not balanced
	if ( ( arr % 2 ) > 0 || text ) {
		snprintf( err, errlen, "%s\n", "Got unbalanced JSON object or unterminated string" );
		return 0;
	}
 
	return 1;	
}	


/**
 * zjson_compress( const char *str, int len, int *newlen )
 *
 * Creates a copy of JSON string with no spaces for easier 
 * parsing and less memory usage.  
 * 
 */
char * zjson_compress ( const char *str, int len, int *newlen ) {
	//Loop through the entire thing and "compress" it.
	char *cmp = NULL;
	int marked = 0;
	int cmplen = 0;

	//TODO: Replace with calloc() for brevity
	if ( !( cmp = malloc( len ) ) ) {
		return NULL;
	}

	memset( cmp, 0, len );

	//Move through, check for any other syntactical errors as well as removing whitespace
	for ( char *s = (char *)str, *x = cmp; *s && len; s++, len-- ) {
		//If there are any backslashes, we need to get the character immediately after
		if ( marked && ( *s == '\\' ) ) {
			fprintf( stderr, "Got a backslash\n" );
			//If the next character is any of these, copy it and move on
			if ( len > 1 ) {
				if ( memchr( "\"\\/bfnqt", *(s + 1), 8 ) ) {
					*x = *s, x++, cmplen++, s++;
					*x = *s, x++, cmplen++;
				}
			#if 0
				//TODO: Would really like Unicode point support, but it has to wait
				else if ( len > 4 && *(s + 1) == 'u' || *(s + 1) == 'U' ) {
					memcpy(  
					x += 5;
				} 
			#endif
			}
			//If there is a 3rd backslash with no character behind it, then just skip it.
			continue;
		}
		//If there is any text, mark it.
		else if ( *s == '"' ) { 
//fprintf( stderr, "Got a quote\n" );
			marked = !marked;
		}
	#if 0
		//TODO: I want to reject single quotes
		else if ( *s == '\'' ) {
fprintf( stderr, "Got a quote\n" );
		}
	#endif
		//If the character is whitespace, skip it
		if ( !marked && memchr( " \r\t\n", *s, 4 ) ) {
			continue;
		}
		*x = *s, x++, cmplen++;
	}

	cmp = realloc( cmp, cmplen + 1 );
	*newlen = cmplen + 1;
	return cmp;
}


/**
 * void *mjson_add_item_to_list
 * 	( void ***list, void *element, int size, int * len )
 *
 * Adds a struct mjson to a dynamically sized list.
 * 
 */
static void * mjson_add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}


/**
 * struct mjson * create_mjson ( const char *str, int len, char *err, int errlen )
 *
 * Creates and initializes a 'struct mjson'
 *
 */
static struct mjson * create_mjson () {
	struct mjson *m = NULL;
	if ( !( m = malloc( sizeof ( struct mjson ) ) ) ) {
		return NULL;
	}
	memset( m, 0, sizeof( struct mjson ) );
	return m;
}


/**
 * void zjson_dump_item ( struct mjson *mjson )
 *
 * Converts serialized JSON into a ztable_t
 * 
 */
void zjson_dump_item ( struct mjson *m ) {
	fprintf( stderr, "[ '%c', %d, ", m->type, m->index );
	if ( m->value && m->size ) {
		fprintf( stderr, "size: %d, ", m->size );
		write( 2, m->value, m->size );
	}
	write( 2, " ]\n", 3 );
}


/**
 * void zjson_dump( struct mjson ** )
 *
 * Dumps a list of 'struct mjson' structures
 *
 */
void zjson_dump ( struct mjson **mjson ) {
	//Dump out the list of what we found
	fprintf( stderr, "\n" );
	for ( struct mjson **v = mjson; v && *v; v++ ) {
	#if 1
		zjson_dump_item( *v );
	#else
		fprintf( stderr, "\n[ '%c', ", (*v)->type ); 
		if ( (*v)->value && (*v)->size ) {
			fprintf( stderr, "size: %d, ", (*v)->size );
			write( 2, (*v)->value, (*v)->size );
		}
		write( 2, " ]", 3 );
	#endif
	}
}


/**
 * zjson_decode( const char *str, int len, char *err, int errlen )
 *
 * Decodes JSON strings and turns them into something different.
 *
 */
struct mjson ** zjson_decode ( const char *str, int len, char *err, int errlen ) {
	const char tokens[] = "\"{[}]:,"; // this should catch the backslash
	struct mjson **mjson = NULL, *c = NULL, *d = NULL;
	int mjson_len = 0;
	zWalker w = {0};

	//Before we start, the parent must be set as the first item...
	struct mjson *m = create_mjson();
	m->type = *str;
	m->value = NULL; //(unsigned char *)str;
	m->size = 0; // Carry the length of the string here...
	m->index = 0; // Carry the length of the string here...
	mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
	str++;

	//Walk through it and deserialize
	for ( int text = 0, esc = 0, size = 0; strwalk( &w, str, tokens ); ) {
		//fprintf( stderr, "CHR: Got '%c' ", w.chr ); getchar();

		if ( text ) {
			if ( w.chr == '"' ) {
				//fprintf( stderr, "got end of text, finalizing\n" );
				//Check the preceding char, and see if it's a backslash.
				text = *( w.rptr - 1 ) == '\\';
#if 0
fprintf( stderr, "closing quote reached, got '%c', '%c', '%c' is escaped: %s\n", 
*( w.ptr ),
*( w.rptr - 1 ),
( w.chr ),
text ? "Y" : "N" );
#endif
				size += w.size;
				c->size = size; //Need to add the total size of whatever it may be.
				continue;
			}
			else {
				size += w.size;
				continue;
			}
		}

		if ( w.chr == '"' ) {
			text = 1;
			size = -1;
			//fprintf( stderr, "type: %c, tab: %d, size: %d\n", 'S', ind, size );
		#if 1
			struct mjson *m = create_mjson();
			m->type = 'S', m->value = w.ptr, m->size = 0;
		#else
			struct mjson *m = create_mjson( 'S', w.ptr, 0 );
		#endif
			c = m;
		}
		else if ( w.chr == '{' || w.chr == '[' ) {
			//fprintf( stderr, "type: %c, tab: %d\n", '{', ++ind );
		#if 1
			struct mjson *m = create_mjson();
			m->type = w.chr, m->value = NULL, m->size = 0;
		#else
			struct mjson *m = create_mjson( w.chr, NULL, 0 );
		#endif
			mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
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
			unsigned char *p = w.ptr - 2;
			for ( ; ; p-- ) {
				if ( memchr( "{}[]:,'\"", *p, 8 ) ) {
					break;
				}
				else {
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
			#if 1
				struct mjson *m = create_mjson();
				m->value = p + 1, m->size = len, m->type = 'V';
			#else
				struct mjson *m = create_mjson( 'V', p + 1, len );
			#endif
				mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
			}

			//Always save whatever character it might be
			if ( w.chr != ':' && w.chr != ',' ) {
			#if 1
				struct mjson *m = create_mjson();
				m->value = NULL, m->size = 0, m->type = w.chr; 
			#else
				struct mjson *m = create_mjson( w.chr, NULL, 0 );
			#endif
				mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
			}
		}
	}

	//Terminate the last entry
	( mjson[ mjson_len - 1 ] )->index = ZJSON_TERMINATOR;
	return mjson;
}


/**
 * zjson_free( struct mjson **mjson )
 *
 * Free a list of 'struct mjson'.
 * 
 */
void zjson_free ( struct mjson **mjson ) {
	//free( (*mjson)->value );
	for ( struct mjson **v = mjson; v && *v; v++ ) {
		if ( (*v)->index == ZJSON_FREE_VALUE ) {
			free( (*v)->value );
		}
		free( *v );
	}
	free( mjson );
}


/**
 * static int zjson_numeric( char *val, int len )
 *
 * Check if some value is numeric or not.
 *
 */
static int zjson_check_numeric( char *val, int len ) {
	for ( char *v = val; len; len--, v++ ) {
		if ( !memchr( "0123456789", *v, 10 ) ) {
			return 0;
		}
	}
	return 1;
}


/**
 * zjson_get_count( struct mjson **mjson )
 *
 * Get a count of entries in an 'struct mjson' list.
 * 
 */
static int zjson_get_count ( struct mjson **mjson ) {
	int count = 0;
	for ( struct mjson **v = mjson; v && *v; v++ ) {
		count++;
	}
	return count;
}


/**
 * zjson_get_entries ( struct mjson **mjson )
 *
 * Get a count of valid entries.  Use this to count the 
 * number of actual values in a list. If this number is zero, 
 * this means that we received an empty object or array.
 * 
 * If mjson is an object, then we need at least 4 values 
 * If mjson is an array, then we need at least 3 values
 * 
 */
int zjson_has_real_values ( struct mjson **mjson ) {
	int count = 1;
	int limit = ( (*mjson)->type == '{' ) ? 4 : 3;
	for ( struct mjson **v = ++mjson; v && *v; v++ ) {
		count++;
	}
	return ( count >= limit );
}

/**
 * zjson_to_ztable( struct mjson **mjson, void *null, char *err, int errlen )
 *
 * Converts serialized JSON into a ztable_t
 * 
 */
ztable_t * zjson_to_ztable ( struct mjson **mjson, char *err, int errlen ) {
	ztable_t *t = NULL;
	struct mjson *ptr[100] = { NULL }, **p = ptr;

	if ( !( t = lt_make( zjson_get_count( mjson ) * 2 ) ) ) {
		snprintf( err, errlen, "Failed to allocate space for ztable_t.\n" );
		return NULL;
	}

	//Use a static list of mjsons to keep track of which objects we're in
	*p = *mjson;

	for ( struct mjson **v = ++mjson; v && *v && ((*v)->index > ZJSON_TERMINATOR ); v++ ) {
		if ( (*v)->type == '{' || (*v)->type == '[' ) {
//fprintf( stderr, "%s - %c - ", "DESCENDING", (*v)->type == '{' ? 'A' : 'N' );
			//If the immediate member before is a [, then you need to add a key before descending
			if ( (*p)->type == '[' ) {
//fprintf( stderr, "Current (*p) is: %c - ", (*p)->type ), fprintf( stderr, "adding key (%d)\n", (*p)->index );
				lt_addintkey( t, (*p)->index );
				(*p)->index++;
			}
//fprintf( stderr, "\n" );
			lt_descend( t ), ++p, *p = *v;
		}
		else if ( (*v)->type == '}' || (*v)->type == ']' ) {
//fprintf( stderr, "%s - %c - ", "ASCENDING", (*v)->type == '{' ? 'A' : 'N' ), fprintf( stderr, "\n" );
			lt_ascend( t ), --p;
			//If the parent is 1, you need to reset it here, since this is most likely the value
			if ( ( (*p)->type == '{' ) && ( (*p)->index == 1 ) ) {
				(*p)->index = 0;
			}
		}
		else if ( (*v)->type == 'S' || (*v)->type == 'V' ) {
			// Put the value in a temporary buffer
			char *val = malloc( (*v)->size + 1 );
			memset( val, 0, (*v)->size + 1 );
		#if 0
			memcpy( val, (*v)->value, (*v)->size );
		#else	
			//Do not copy back slashes.
			if ( !memchr( (*v)->value, '\\', (*v)->size ) )
				memcpy( val, (*v)->value, (*v)->size );
			else {
				char *x = (char *)(*v)->value, *y = val;
				for ( int vsize = (*v)->size; vsize; vsize--, x++ ) {
					if ( *x == '\\' ) {
						continue;
					}
					*y = *x, y++;
				}
			}
		#endif

//fprintf( stderr, "Current (*p) is: %c - ", (*p)->type );
			if ( (*p)->type == '[' ) {
//fprintf( stderr, "adding key value pair (%d => '%s')\n", (*p)->index, val );
				lt_addintkey( t, (*p)->index );
				lt_addtextvalue( t, val );
				lt_finalize( t );
				(*p)->index++;
			}
			else if ( (*p)->type == '{' ) {
				if ( ( (*p)->index = !(*p)->index ) ) {
//fprintf( stderr, "adding key ('%s')\n", val );
					lt_addtextkey( t, val );
				}
				else {
//fprintf( stderr, "adding value ('%s') and finalizing\n", val );
					lt_addtextvalue( t, val );
					lt_finalize( t );
				}
			}
			free( val );
		}
	}
	return t;
}


/**
 * ztable_to_zjson ( ztable_t *t, char *err, int errlen )
 *
 * Converts from ztable_t to regular JSON structure.
 * 
 */
struct mjson ** ztable_to_zjson ( ztable_t *t, char *err, int errlen ) {
	const char emptystr[] = "''";
	struct mjson **mjson = NULL;
	int mjson_len = 0;
	char ptr[100] = { 0 }, *p = ptr;
	zKeyval *kv = NULL;

	//Die if no table was given.
	if ( !t ) {
		snprintf( err, errlen, "Table for JSON conversion not initialized" );
		return NULL;
	}

	//Reset the table (TODO: The behavior on this is a bit wonky, need to fix it...)
	lt_reset( t );

	//Initialize the first member in the set.
	struct mjson *fm = create_mjson();
	fm->size = 0;
	fm->type = 0;
	fm->index = 0;
	fm->value = NULL;
	mjson_add_item( &mjson, fm, struct mjson, &mjson_len );	

	//Then figure out how to start the JSON body
	kv = lt_next( t );
	if ( (kv->key).type == ZTABLE_TXT )
		*p = fm->type = '{';
	else if ( (kv->key).type == ZTABLE_INT || (kv->key).type == ZTABLE_FLT )
		*p = fm->type = '[';
	else if ( lt_countall( t ) > 1 ) {
		const char fmt[] = "Got invalid first key type %s for table.";
		snprintf( err, errlen, fmt, lt_typename( (kv->key).type ) );
		return NULL;
	}

	//Loop through all values and copy
	for ( ; kv; ( kv = lt_next( t ) ) ) {
		for ( int isValue = 0; isValue < 2; isValue++ ) {
			int len = 0;
			char t = 0, *v = NULL, nb[ 64 ] = {0};
			zhValue x = !isValue ? kv->key : kv->value;//fprintf( stderr, "%c: ", !isValue ? 'L' : 'R' );

			//Get the value and the length of the value
			if ( x.type == ZTABLE_NON )
				break;
			else if ( x.type == ZTABLE_NUL || ( x.type == ZTABLE_INT && isValue == 0 ) )
				0;//v = "Z", len = 1;
			else if ( x.type == ZTABLE_BLB )
				t = 'S', len = x.v.vblob.size, v = (char *)x.v.vblob.blob; 
			else if ( x.type == ZTABLE_FLT )
				t = 'S', len = snprintf( nb, sizeof( nb ), "%f", x.v.vfloat ), v = nb;
			else if ( x.type == ZTABLE_INT )
				t = 'S', len = snprintf( nb, sizeof( nb ), "%d", x.v.vint ), v = nb;
			else if ( x.type == ZTABLE_TRM )
				t = ( *p == '{' ) ? '}' : ']', p--;
			else if ( x.type == ZTABLE_TXT ) {
				if ( x.v.vchar )
					t = 'S', len = strlen( x.v.vchar ), v = x.v.vchar;
				else {
					t = 'E';
				}
			}
			else if ( x.type == ZTABLE_TBL ) {
				if ( ( kv + 1 )->key.type == ZTABLE_TXT ) 
					t = '{';
				else if ( ( kv + 1 )->key.type == ZTABLE_INT ) 
					t = '[';
				else {
					//Handle blank tables correctly with this.
					t = ( (kv->key).type == ZTABLE_TXT ) ? '{' : '[';
				}
				*(++p) = t;
			}
			else {
				snprintf( err, errlen, "Got invalid key type: %s", lt_typename( x.type ) );
				return NULL;	
			}

			if ( t ) {
				struct mjson *m = create_mjson();
				m->type = t;
				if ( v ) {
					if ( !( m->value = malloc( len + 1 ) ) ) {
						//free the last mjson and the list up to this point...
						free( m ), zjson_free( mjson );
						return NULL;
					}
					memset( m->value, 0, len + 1 );
					memcpy( m->value, v, len );
					m->index = ZJSON_FREE_VALUE; 
					m->size = len; 
				}
				mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
			}
		}
	}

	//Add the last struct mjson
	struct mjson *m = create_mjson();
	m->type = fm->type == '{' ? '}' : ']'; 
	m->index = ZJSON_TERMINATOR; 
	mjson_add_item( &mjson, m, struct mjson, &mjson_len );	
	return mjson;
}


/**
 * zjson_stringify( struct mjson **mjson, char *err, int errlen )
 *
 * Create a "compressed" JSON string out of a list of 'struct mjson'.
 *
 */
char * zjson_stringify( struct mjson **mjson, char *err, int errlen ) {
	int jslen = 0;
	char ptr[100] = { 0 }, *p = ptr, *js = NULL, *t = NULL;

	//Initialize the first child
	if ( !( js = realloc( js, jslen + 1 ) ) || !memcpy( &js[ jslen ], &(*mjson)->type, 1 ) ) {
		snprintf( err, errlen, "Failed to terminate JSON string\n" );
		return NULL;
	}

	//Initialize our first "parent", and needed types
	*p = (*mjson)->type, jslen++;

	//Loop through and add to the string
	for ( struct mjson *nx, **v = ++mjson; v && *v && ((*v)->index > ZJSON_TERMINATOR ); v++ ) {
		int len = 0, encl = 0;
		char *val = NULL;
		//zjson_dump_item( *v );

		//Add the object/array dividers first
		if ( !(*v)->value && ( (*v)->type == '{' || (*v)->type == '[' ) )
			len = 1, val = &(*v)->type, *(++p) = (*v)->type;
		else if ( !(*v)->value && ( (*v)->type == '}' || (*v)->type == ']' ) )
			len = 1, val = &(*v)->type, p--;
		else if ( !(*v)->value && (*v)->type == 'E' )
			len = 2, val = "\"\"";
		else {
			len = (*v)->size;
			val = (char *)(*v)->value;
			encl = 1;

			//Check if the value is [ null, true, false or a number ]
			if ( len == 4 && !memcmp( "true", val, 4 ) )
				encl = 0;
			else if ( len == 4 && !memcmp( "null", val, 4 ) ) 
				encl = 0;
			else if ( len == 5 && !memcmp( "false", val, 5 ) )
				encl = 0;
			else {
				encl = !zjson_check_numeric( val, len );
			}
		}	

		//Get the original end
		char k = js[ jslen - 1 ];

		//Add to the current buffer
		if ( (*v)->index > ZJSON_FREE_VALUE ) {
			if ( !( js = realloc( js, jslen + len ) ) || !memcpy( &js[ jslen ], val, len ) ) {
				snprintf( err, errlen, "Failed to allocate memory for JSON string\n" );
				return NULL;
			}
			jslen += len;
		}
		else {
			if ( !( js = realloc( js, jslen + len + ( encl ? 2 : 0 ) ) ) ) { 
				snprintf( err, errlen, "Failed to allocate memory for JSON string\n" );
				return NULL;
			}
			
			if ( !encl )
				memcpy( &js[ jslen ], val, len ), jslen += len;
			else {
				memcpy( &js[ jslen ], "\"", 1 ), jslen++;
				memcpy( &js[ jslen ], val, len ), jslen += len;
				memcpy( &js[ jslen ], "\"", 1 ), jslen++;
			}
		}

		//Peek ahead at the next member to add a ':' or ','
		if ( ( nx = *( v + 1 ) )->index != ZJSON_TERMINATOR ) {
			len = 0;
			if ( *p == '{' ) {
				if ( memchr( "SE", (*v)->type, 2 ) && memchr( "[{SE", nx->type, 4 ) )
					len = 1, val = k == ':' ? "," : ":";
				//else if ( ( (*v)->type == ']' || ( (*v)->type == '}' ) && nx->type != '}' ) ) {
				//else if ( memchr( "]}", (*v)->type, 2 ) && nx->type != '}' ) {
				else if ( memchr( "]}", (*v)->type, 2 ) && !memchr( "]}", nx->type, 2 ) ) {
					len = 1, val = ",";
				}
			}
			else /* ( *p == '[' ) */ {
				//if ( nx->type != ']' && (*v)->type != '[' ) {
				//if ( !memchr( "]}", nx->type, 2 ) && (*v)->type != '[' ) {
				if ( (*v)->type != '[' && !memchr( "]}", nx->type, 2 ) ) {
					len = 1, val = ",";
				}
			}

			if ( len ) {
				if ( !( js = realloc( js, jslen + len ) ) || !memcpy( &js[ jslen ], val, len ) ) {
					snprintf( err, errlen, "Failed to allocate memory for JSON seperator\n" );
					return NULL;
				}
				jslen += len;
			}
		}
		
	}

	//Terminate the sequence...
	t = ( *p == '{' ) ? "}" : "]";
	if ( !( js = realloc( js, jslen + 2 ) ) || !memcpy( &js[ jslen ], t, 2 ) ) {
		snprintf( err, errlen, "Failed to terminate JSON string\n" );
		return NULL;
	}

	return js;
}




#ifdef ZJSON_TEST

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ztable.h>
#include <zwalker.h>

/**
 * int main(...)
 *
 * Simple command-line program using -decode and -encode flags to 
 * select a file and test decoding/encoding JSON.
 *
 */
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
	if ( !( mjson = zjson_decode_oneshot( con, sb.st_size, err, sizeof( err ) ) ) ) {
		fprintf( stderr, "Failed to decode JSON at zjson_decode_oneshot(): %s", err );
		return 0;
	}
#else
	// In lieu of a "oneshot" function, we'll need to define some more variables
	char *cmpstr = NULL;
	int cmplen = 0;

	// Dump the original string
	//write( 2, "'", 1 ), write( 2, con, sb.st_size), write( 2, "'", 1 );getchar();

	// Check if the string is valid
	if ( !zjson_check_syntax( con, sb.st_size, err, sizeof( err ) ) ) {
		free( con );
		fprintf( stderr, "JSON string failed syntax check: %s\n", err );
		return 1;
	}

	// Compress the string first
	if ( !( cmpstr = zjson_compress( con, sb.st_size, &cmplen ) ) ) {
		free( con );
		fprintf( stderr, "Failed to compress JSON at zjson_compress(): %s", err );
		return 1;
	}

	// Dump the new string
	write( 2, "'", 1 ), write( 2, cmpstr, cmplen ), write( 2, "'", 1 ); //, getchar();

	// Free the original string
	free( con );

	// Then decode the new string 
	if ( !( mjson = zjson_decode( cmpstr, cmplen - 1, err, sizeof( err ) ) ) ) {
		fprintf( stderr, "Failed to deserialize JSON at json_decode(): %s", err );
		return 1;
	}
#endif

	// Dump the serialized JSON
	//zjson_dump( mjson ); //, getchar();

	// Only turn lists with actual items into tables
	if ( zjson_has_real_values( mjson ) ) {
		// Make a ztable out of it
		ztable_t *t = NULL;
		if ( !( t = zjson_to_ztable( mjson, err, sizeof( err ) ) ) ) {
			fprintf( stderr, "Failed to make table out of JSON at zjson_to_ztable(): %s", err );
			return 1;
		}

		// Dump the table (everything is duplicated)
		lt_kfdump( t, 2 );

		// Free the first set of deserialized JSON, then...
		zjson_free( mjson );

		// Convert back to JSON for fun and joy
		if ( !( mjson = ztable_to_zjson( t, err, sizeof( err ) ) ) ) {
			fprintf( stderr, "Failed to deserialize table to JSON structures: %s", err );
			return 1;
		}

		// Dump JSON again
		//zjson_dump( mjson );//, getchar();

		// And free it to reclaim resources
		lt_free( t ), free( t );
	}

	// Convert to string, and it should look the same provided we made it this far.
	char *jsonstr = NULL;	
	if ( !( jsonstr = zjson_stringify( mjson, err, sizeof( err ) ) ) ) {
		fprintf( stderr, "Failed to stringify JSON: %s", err );
		return 1;
	}
 
	// Free the JSON structure and the source string
	zjson_free( mjson ); //, free( cmp );

	// Dump the new string
	write( 2, "\n'", 2 ), write( 2, jsonstr, strlen( jsonstr ) ), write( 2, "'", 1 ); //getchar();

	// If we've gotten here, we need to test that the input is the same as output.
	fprintf( stdout, "%s\n", jsonstr );
	int status = strcmp( jsonstr, cmpstr ) != 0;
	//int status = memcmp( jsonstr, cmpstr, cmplen ) != 0;
	free( jsonstr ), free( cmpstr );
	return status;
}
#endif



