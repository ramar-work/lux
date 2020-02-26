#include "filter-echo.h"
int filter_echo ( struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {

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

#if 0
	//The path check should be useless, since the first function will catch that...
	//If this works, then I can add proper checks...
	append_to_uint8t( &buf, &buflen, "<h2>URL</h2>\n", rq->path, "<br>\n" );	
#else
	const char urlfmt[] = "<h2>URL</h2>\n%s<br>\n";
	//Sanity checks
	if ( !rq->path || !( buflen = (strlen(urlfmt) - 1) + strlen(rq->path) ) )
		return http_set_error( rs, 500, "Path cannot be zero..." );

	//Reallocate a buffer
	if ( !( buf = realloc( buf, buflen ) ) || !memset( buf, 0, buflen ) )
		return http_set_error( rs, 500, strerror( errno ) );

	if ( ( buflen = snprintf( (char *)buf, buflen, urlfmt, rq->path )) == -1 )
		return http_set_error( rs, 500, "Failed to zero memory..." );
#endif

	//Switch to whiles, b/c it's just easier to follow...
	while ( ttt && *ttt ) {
		char *endstr = (*ttt)->records ? "\n" : "\n-<br>\n";
	#if 0
		append_to_uint8t( &buf, &buflen, "<h2>", (*ttt)->name, "</h2>", endstr );
	#else
		append_to_uint8t( &buf, &buflen, (uint8_t *)"<h2>", 4 );
		append_to_uint8t( &buf, &buflen, (uint8_t *)(*ttt)->name, strlen( (*ttt)->name ) );
		append_to_uint8t( &buf, &buflen, (uint8_t *)"</h2>", 5 );
		append_to_uint8t( &buf, &buflen, (uint8_t *)endstr, strlen( endstr ) );
	#endif

		while ( (*ttt)->records && (*(*ttt)->records)->field ) {
			struct HTTPRecord *r = (*(*ttt)->records);
		#if 0
			//In this case, r->value really should be a string...
			append_to_uint8t( &buf, &buflen, r->field, " => ", r->value, "<br>\n" );
		#else
			append_to_uint8t( &buf, &buflen, (uint8_t *)r->field, strlen( r->field ) ); 
			append_to_uint8t( &buf, &buflen, (uint8_t *)" => ", 4 );
			append_to_uint8t( &buf, &buflen, (uint8_t *)r->value, r->size ); 
			append_to_uint8t( &buf, &buflen, (uint8_t *)"<br>\n", 5 ); 
		#endif
			(*ttt)->records++;
		}
		ttt++;
	}

	//Package a message
	http_set_status( rs, 200 );
	http_set_ctype( rs, "text/html" );
	http_set_content( rs, buf, buflen );
	if ( !http_finalize_response( rs, err, sizeof(err) ) ) {
		return http_set_error( rs, 500, err );
	}

	free( buf );
	return 1;
}
