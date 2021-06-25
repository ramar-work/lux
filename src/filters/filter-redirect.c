/* ------------------------------------------- * 
 * filter-redirect.c
 * =========
 * 
 * Summary 
 * -------
 * -
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
#include "filter-redirect.h"

const int 
filter_redirect ( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn ) {
	char err[2048] = {0};

	//Generate a message
	http_set_status( res, 302 );
	http_set_ctype( res, "text/plain" );
	http_copy_header( res, "Location", "http://collinsdesign.net" );
	http_copy_content( res, " ", 1 );

	//This should just work everytime
	if ( !http_finalize_response( res, err, sizeof(err) ) ) {
		return http_set_error( res, 500, err );
	}

	//Set connection count to -3?, will cause the connection to close, no reason to leave it open.
	conn->count = -1;
	return 1;
}
