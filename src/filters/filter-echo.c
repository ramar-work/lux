/* ------------------------------------------- * 
 * filter-echo.c
 * ===========
 * 
 * Summary 
 * -------
 * Functions comprising the echo filter for stress testing Hypno's capabilities. 
 *
 * Usage
 * -----
 * filter-echo.c forces hypno to simply echo back what was sent.  This is mostly
 * for testing and has little use for anything but diagnostics.  It can safely
 * be disabled in production.
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "filter-echo.h"

//

const int
filter_echo ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, struct cdata *conn ) {

	//Allocate a big buffer and do work
	char err[ 2048 ] = { 0 };
	uint8_t *buf = NULL;
	struct n { const char *name; struct HTTPRecord **records; } **ttt = 
	(struct n *[]){
		&(struct n){ "Headers", rq->headers },
		&(struct n){ "GET", rq->url },
		&(struct n){ "POST", rq->body },
		NULL
	};
	int buflen = 0; 
	int progress = 0;

	const char urlfmt[] = "<h2>URL</h2>\n%s<br>\n";
	//Sanity checks
	if ( !rq->path || !( buflen = (strlen(urlfmt) - 1) + strlen(rq->path) ) ) {
		return http_set_error( rs, 500, "Path cannot be zero..." );
	}

	//Reallocate a buffer
	if ( !( buf = realloc( buf, buflen ) ) || !memset( buf, 0, buflen ) ) {
		return http_set_error( rs, 500, strerror( errno ) );
	}

	if ( ( buflen = snprintf( (char *)buf, buflen, urlfmt, rq->path )) == -1 ) {
		return http_set_error( rs, 500, "Failed to zero memory..." );
	}

	//Switch to whiles, b/c it's just easier to follow...
	while ( ttt && *ttt ) {
		struct HTTPRecord **r = (*ttt)->records; 
		char *endstr = r ? "\n" : "\n-<br>\n";

		append_to_uint8t( &buf, &buflen, (uint8_t *)"<h2>", 4 );
		append_to_uint8t( &buf, &buflen, (uint8_t *)(*ttt)->name, strlen( (*ttt)->name ) );
		append_to_uint8t( &buf, &buflen, (uint8_t *)"</h2>", 5 );
		append_to_uint8t( &buf, &buflen, (uint8_t *)endstr, strlen( endstr ) );

		while ( r && *r ) {
			append_to_uint8t( &buf, &buflen, (uint8_t *)(*r)->field, strlen( (*r)->field ) ); 
			append_to_uint8t( &buf, &buflen, (uint8_t *)" => ", 4 );
			append_to_uint8t( &buf, &buflen, (uint8_t *)(*r)->value, (*r)->size ); 
			append_to_uint8t( &buf, &buflen, (uint8_t *)"<br>\n", 5 ); 
			r++;	
		}
		ttt++;
	}

	//Package a message
	http_set_status( rs, 200 );
	http_set_ctype( rs, "text/html" );
	http_set_content( rs, buf, buflen );

	if ( !http_finalize_response( rs, err, sizeof(err) ) ) {
		free( buf );
		return http_set_error( rs, 500, err );
	}

	return 1;
}
