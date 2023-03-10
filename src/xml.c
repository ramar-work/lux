/* ------------------------------------------- * 
 * xml.c
 * ======
 * 
 * Summary 
 * -------
 * Simple module for converting zTables to
 * XML. (and back at some point)
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 * 
 * ------------------------------------------- */
#include "xml.h"

char * xml_encode ( ztable_t *t, const char *rootname ) {
	//Define things
	unsigned char * content = NULL;
	char rootbuf[ 256] = { 0 }, * pset[ 100 ] = { NULL }, **parent = pset;

	//char **parent = malloc( sizeof( char * ) * 100 );
	//memset( parent, 0, sizeof( char * ) * 100 );
	int clen = 0, tab = 1;

	//Initialize things
	lt_reset( t );

	//
	const char *root = !rootname ? "root" : rootname;
	snprintf( rootbuf, sizeof( rootbuf ) - 1, "<%s>\n", root );
	append_to_uint8t( &content, &clen, (unsigned char *)rootbuf, strlen( rootbuf ) );

	//Loop through all values and copy
	for ( zKeyval *kv ; ( kv = lt_next( t ) ); ) {
		zhValue k = kv->key, v = kv->value;
		char kk[ 256 ] = { 0 }, vbuf[ 2048 ] = { 0 };
		int vlen = 0;

		if ( k.type == ZTABLE_TXT )
			memcpy( kk, k.v.vchar, strlen( k.v.vchar ) );
		else if ( k.type == ZTABLE_BLB )
			memcpy( kk, k.v.vblob.blob, k.v.vblob.size );
		else if ( k.type == ZTABLE_INT ) //TODO: If there is a parent use that
			snprintf( kk, sizeof( kk ) - 1, "%d", k.v.vint );
		else if ( k.type == ZTABLE_TRM ) {
			tab--; 
			for ( int i = 0; i < tab; i++ ) {
				append_to_uint8t( &content, &clen, (unsigned char *)"\t", 1 );
			}
			snprintf( kk, sizeof( kk ) - 1, "</%s>\n", *parent );
			append_to_uint8t( &content, &clen, (unsigned char *)kk, strlen( kk ) );
			free( *parent );
			parent--;
			continue;
		}
		else if ( k.type == ZTABLE_NON ) {
			break;
		}
		else {
			continue;
		}

		//Add any tabs first
		for ( int i = 0; i < tab; i++ ) {
			append_to_uint8t( &content, &clen, (unsigned char *)"\t", 1 );
		}

		//Set the parent
		if ( v.type == ZTABLE_TXT )
			vlen = snprintf( vbuf, sizeof( vbuf ) - 1, "<%s>%s</%s>\n", kk, v.v.vchar, kk );
		else if ( v.type == ZTABLE_INT )
			vlen = snprintf( vbuf, sizeof( vbuf ) - 1, "<%s>%d</%s>\n", kk, v.v.vint, kk );
		else if ( v.type == ZTABLE_BLB ) {
			vlen = snprintf( &vbuf[ vlen ], sizeof( vbuf ) - vlen - 1, "<%s>\n", kk );
			memcpy( &vbuf[ vlen ], v.v.vblob.blob, v.v.vblob.size ), vlen += v.v.vblob.size;
			vlen += snprintf( &vbuf[ vlen ], sizeof( vbuf ) - vlen - 1, "</%s>\n", kk );
		}
		else if ( v.type == ZTABLE_TBL ) {
			//TODO: Use the key name and make it the parent (you'll have to move)
			++parent, *parent = zhttp_dupstr( kk );
			++tab;
			vlen = snprintf( vbuf, sizeof( vbuf ) - 1, "<%s>\n", *parent );
		}

		//If we get here, append...
		append_to_uint8t( &content, &clen, (unsigned char *)vbuf, vlen );
	}

	snprintf( rootbuf, sizeof( rootbuf ) - 1, "</%s>", root );
	append_to_uint8t( &content, &clen, (unsigned char *)rootbuf, strlen( rootbuf ) );
	append_to_uint8t( &content, &clen, (unsigned char *)"\0", 1 );
	return (char *)content;
}

#if 0
//We'll have to do this eventually
char * xml_decode ( ztable_t *t ) {
	return NULL;
}
#endif
