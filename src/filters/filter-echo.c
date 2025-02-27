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





const int filter_echo ( const server_t *p, conn_t *conn ) {

	//Allocate a big buffer and do work
	const char urlfmt[] = "<h2>URL</h2>\n%s<br>\n";
	uint8_t *buf = NULL;
	int bl = 0, progress = 0;
	struct n { const char *name; zhttpr_t **records; } **ttt = 
	(struct n *[]){
		&(struct n){ "Headers", conn->req->headers },
		&(struct n){ "GET", conn->req->url },
		&(struct n){ "POST", conn->req->body },
		NULL
	};

	//Sanity checks
	if ( !conn->req->path || !( bl = ( sizeof(urlfmt) - 1 ) + strlen( conn->req->path ) ) )
		return http_set_error( conn->res, 500, "Path cannot be zero..." );

	//Reallocate a buffer
	if ( !( buf = realloc( buf, bl ) ) || !memset( buf, 0, bl ) )
		return http_set_error( conn->res, 500, strerror( errno ) );

	if ( ( bl = snprintf( (char *)buf, bl, urlfmt, conn->req->path )) == -1 )
		return http_set_error( conn->res, 500, "Failed to zero memory..." );

	//Switch to whiles, b/c it's just easier to follow...
	while ( ttt && *ttt ) {
		zhttpr_t **r = (*ttt)->records; 
		char *endstr = r ? "\n" : "\n-<br>\n";

		append_to_uint8t( &buf, &bl, (uint8_t *)"<h2>", 4 );
		append_to_uint8t( &buf, &bl, (uint8_t *)(*ttt)->name, strlen( (*ttt)->name ) );
		append_to_uint8t( &buf, &bl, (uint8_t *)"</h2>", 5 );
		append_to_uint8t( &buf, &bl, (uint8_t *)endstr, strlen( endstr ) );

		while ( r && *r ) {
			append_to_uint8t( &buf, &bl, (uint8_t *)(*r)->field, strlen( (*r)->field ) ); 
			append_to_uint8t( &buf, &bl, (uint8_t *)" => ", 4 );
			append_to_uint8t( &buf, &bl, (uint8_t *)(*r)->value, (*r)->size ); 
			append_to_uint8t( &buf, &bl, (uint8_t *)"<br>\n", 5 ); 
			r++;	
		}
		ttt++;
	}

	//Package a message
	http_set_status( conn->res, 200 );
	http_set_ctype( conn->res, "text/html" );
	http_set_content( conn->res, buf, bl );

	if ( !http_finalize_response( conn->res, conn->err, sizeof( conn->err ) ) ) {
		free( buf );
		return http_set_error( conn->res, 500, conn->err );
	}

	return 1;
}
