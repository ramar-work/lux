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



//This is the important one
zTable * zjson_decode ( const char *str, int len, char *err, int errlen ) {
	const char tokens[] = "\"{[}]:,\\"; // this should catch the backslash
	unsigned char *b = NULL;
	//int size = zjson_count( (unsigned char *)str, strlen( str ) );
	zWalker w = {0};
	zTable *t = NULL;
	struct zjd zjdset[ ZJSON_MAX_DEPTH ], *d = zjdset;
	memset( zjdset, 0, ZJSON_MAX_DEPTH * sizeof( struct zjd ) );
	struct bot { char *key; unsigned char *val; int size; } bot;

	//Return zTable
	if ( !( t = lt_make( 10000 * 2 ) ) ) {
		snprintf( err, errlen, "%s", "Create table failed." );
		return NULL;
	}

	//Walk through everything
	for ( int i = 0; memwalk( &w, (unsigned char *)str, (unsigned char *)tokens, len, strlen( tokens ) ); ) {
		char *srcbuf, *copybuf, statbuf[ ZJSON_MAX_STATIC_LENGTH ] = { 0 };
		int blen = 0;

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
					ZJSON_PRINTF( "Adding text value: %s\n", copybuf );
					lt_addtextvalue( t, copybuf ), lt_finalize( t );
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
						ZJSON_PRINTF( "Adding int key: %d\n", d->index );
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
			else if ( w.chr == ',' || w.chr == ':' ) {
				( w.chr == ',' && !d->isObject ) ? d->index++ : 0; 
				srcbuf = ( char * )zjson_trim( w.src, "\",: \t\n\r", w.size - 1, &blen );
				if ( blen ) {
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
				else if ( blen >= ZJSON_MAX_STATIC_LENGTH ) {
					snprintf( err, errlen, "%s", "Key is too large." );
					//lt_free( t ), free( t );
					return NULL;
				}
				d->isVal = ( w.chr == ':' );
			}
		}
	}

	lt_lock( t );
lt_kdump( t );
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
		char kbuf[ 256 ] = { 0 }, vbuf[ 2048 ] = { 0 };
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
		char kbuf[ 256 ] = {0}, vbuf[ 2048 ] = {0};
		int lk = 0, lv = 0;

		if ( yy->keysize ) {
			char *k = kbuf;
			if ( yy->type != ZTABLE_TRM ) {
				memcpy( k, "\"", 1 ), k++, lk++;
				memcpy( k, yy->key, yy->keysize ), k += yy->keysize, lk += yy->keysize;
				memcpy( k, "\": ", 3 ), k += 3, lk += 3;
			}
		}

		if ( yy->valsize ) {
			char *v = vbuf;
			if ( yy->type == ZTABLE_TBL )
				memcpy( v, yy->val, yy->valsize ), v += yy->valsize, lv += yy->valsize;
			else if ( yy->type == ZTABLE_INT || yy->type == ZTABLE_FLT ) {
				memcpy( v, yy->vint, yy->valsize ), v += yy->valsize, lv += yy->valsize;
				memcpy( v, yy->comma, 1 ), v += 1, lv += 1;
			}
			else if ( yy->type == ZTABLE_TRM ) {
				memcpy( v, yy->val, yy->valsize ), v += yy->valsize, lv += yy->valsize;
				memcpy( v, yy->comma, 1 ), v += 1, lv += 1;
			}
			else {
				memcpy( v, "\"", 1 ), v++, lv++;
				memcpy( v, yy->val, yy->valsize ), v += yy->valsize, lv += yy->valsize;
				memcpy( v, "\"", 1 ), v += 1, lv += 1;
				memcpy( v, yy->comma, 1 ), v += 1, lv += 1;
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
		memcpy( &json[ jp ], vbuf, lv ), jp += lv;
	}

	//This is kind of ugly
	if ( !( json = realloc ( json, jp + 3 ) ) ) {
		snprintf( err, errlen, "Could not re-allocate source JSON" );
		free( br ), free( json );
		return NULL;
	}

	json[ jp - 1 ] = ' ', json[ jp ] = '}', json[ jp + 1 ] = '\0';
	free( br );
	return json;
}




