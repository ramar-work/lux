#include "http.h"

const char http_200_fixed[] = ""
	"HTTP/1.1 200 OK\r\n"
	"Content-Length: 11\r\n"
	"Content-Type: text/html\r\n\r\n"
	"<h2>Ok</h2>";

const char http_200_custom[] = ""
	"HTTP/1.1 200 OK\r\n"
	"Content-Length: %d\r\n"
	"Content-Type: text/html\r\n\r\n";

const char http_404_fixed[] = ""
	"HTTP/1.1 404 Internal Server Error\r\n"
	"Content-Length: 21\r\n"
	"Content-Type: text/html\r\n\r\n"
	"<h2>Not Found...</h2>";

const char http_404_custom[] = ""
	"HTTP/1.1 500 Internal Server Error\r\n"
	"Content-Length: %d\r\n"
	"Content-Type: text/html\r\n\r\n";

const char http_500_fixed[] = ""
	"HTTP/1.1 500 Internal Server Error\r\n"
	"Content-Length: 18\r\n"
	"Content-Type: text/html\r\n\r\n"
	"<h2>Not OK...</h2>";

const char http_500_custom[] = ""
	"HTTP/1.1 500 Internal Server Error\r\n"
	"Content-Length: %d\r\n"
	"Content-Type: text/html\r\n\r\n";

typedef enum {
	HTTP_100 = 100,
	HTTP_101 = 101,
	HTTP_200 = 200,
	HTTP_201 = 201,
	HTTP_202 = 202,
	HTTP_204 = 204,
	HTTP_206 = 206,
	HTTP_300 = 300,
	HTTP_301 = 301,
	HTTP_302 = 302,
	HTTP_303 = 303,
	HTTP_304 = 304,
	HTTP_305 = 305,
	HTTP_307 = 307,
	HTTP_400 = 400,
	HTTP_401 = 401,
	HTTP_403 = 403,
	HTTP_404 = 404,
	HTTP_405 = 405,
	HTTP_406 = 406,
	HTTP_407 = 407,
	HTTP_408 = 408,
	HTTP_409 = 409,
	HTTP_410 = 410,
	HTTP_411 = 411,
	HTTP_412 = 412,
	HTTP_413 = 413,
	HTTP_414 = 414,
	HTTP_415 = 415,
	HTTP_416 = 416,
	HTTP_417 = 417,
	HTTP_418 = 418,
	HTTP_500 = 500,
	HTTP_501 = 501,
	HTTP_502 = 502,
	HTTP_503 = 503,
	HTTP_504 = 504
} HTTP_Status;


static const char *http_status[] = {
	[HTTP_100] = "Continue",
	[HTTP_101] = "Switching Protocols", 
	[HTTP_200] = "OK",
	[HTTP_201] = "Created",
	[HTTP_202] = "Accepted",
	[HTTP_204] = "No Content",
	[HTTP_206] = "Partial Content",
	[HTTP_300] = "Multiple Choices",
	[HTTP_301] = "Moved Permanently",
	[HTTP_302] = "Found",
	[HTTP_303] = "See Other",
	[HTTP_304] = "Not Modified",
	[HTTP_305] = "Use Proxy",
	[HTTP_307] = "Temporary Redirect",
	[HTTP_400] = "Bad Request",
	[HTTP_401] = "Unauthorized",	
	[HTTP_403] = "Forbidden",			
	[HTTP_404] = "Not Found",				
	[HTTP_405] = "Method Not Allowed",
	[HTTP_406] = "Not Acceptable",
	[HTTP_407] = "Proxy Authentication Required",
	[HTTP_408] = "Request Timeout",
	[HTTP_409] = "Conflict",
	[HTTP_410] = "Gone",
	[HTTP_411] = "Length Required",
	[HTTP_412] = "Precondition Failed",
	[HTTP_413] = "Request Entity Too Large",
	[HTTP_414] = "Request URI Too Long",
	[HTTP_415] = "Unsupported Media Type",
	[HTTP_416] = "Requested Range",
	[HTTP_417] = "Expectation Failed",
	[HTTP_418] = "I'm a teapot",
	[HTTP_500] = "Internal Server Error",
	[HTTP_501] = "Not Implemented",
	[HTTP_502] = "Bad Gateway",
	[HTTP_503] = "Service Unavailable",
	[HTTP_504] = "Gateway Timeout"
};


const char *http_get_status_text ( HTTP_Status status ) {
	//TODO: This should error out if HTTP_Status is not received...
	if ( status < 100 || status > sizeof( http_status ) / sizeof( char * ) ) {
		return http_status[ 200 ];	
	}
	return http_status[ status ];	
}


//Trim whitespace
unsigned char *httpvtrim (uint8_t *msg, int len, int *nlen) {
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr("\r\n\t ", *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr("\r\n\t ", *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


//Trim any characters 
unsigned char *httptrim (uint8_t *msg, const char *trim, int len, int *nlen) {
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}

//list out all rows in an HTTPRecord array
void print_httprecords ( struct HTTPRecord **r ) {
	if ( *r == NULL ) return;
	while ( *r ) {
		fprintf( stderr, "'%s' -> ", (*r)->field );
		//fprintf( stderr, "%s\n", (*r)->field );
		write( 2, "'", 1 );
		write( 2, (*r)->value, (*r)->size );
		write( 2, "'\n", 2 );
		r++;
	}
}


//list out everything in an HTTPBody
void print_httpbody ( struct HTTPBody *r ) {
	if ( r == NULL ) return;
	fprintf( stderr, "r->mlen: '%d'\n", r->mlen );
	fprintf( stderr, "r->clen: '%d'\n", r->clen );
	fprintf( stderr, "r->hlen: '%d'\n", r->hlen );
	fprintf( stderr, "r->status: '%d'\n", r->status );
	fprintf( stderr, "r->ctype: '%s'\n", r->ctype );
	fprintf( stderr, "r->method: '%s'\n", r->method );
	fprintf( stderr, "r->path: '%s'\n", r->path );
	fprintf( stderr, "r->protocol: '%s'\n", r->protocol );
	fprintf( stderr, "r->host: '%s'\n", r->host );
	fprintf( stderr, "r->boundary: '%s'\n", r->boundary );
}

#if 0
//Like lt_* - this can be done with a bunch of #defines
int http_response_set_headers ( struct HTTPBody *entity );
int http_request_set_headers ( struct HTTPBody *entity );
#endif

//Pack an HTTP request
int http_pack_request ( struct HTTPBody *entity, struct HTTPRecord **headers, struct HTTPRecord **body ) {
	return 0;
}

//Finalize an HTTP request (really just returns a uint8_t, but this can handle it)
struct HTTPBody * http_finalize_request ( struct HTTPBody *entity, char *err, int errlen ) {
	return NULL;
}


//Finalize an HTTP response (really just returns a uint8_t, but this can handle it)
struct HTTPBody * http_finalize_response ( struct HTTPBody *entity, char *err, int errlen ) {
	uint8_t *msg = NULL;
	int msglen = 0;
	int http_header_len = 0;
	struct HTTPRecord **headers = NULL;
	struct HTTPRecord **body = NULL;
	char http_header_buf[ 2048 ] = { 0 };
	char http_header_fmt[] = "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\n";

	#if 0
	fprintf( stderr, "%p\n", entity );
	fprintf( stderr, "headerlist: %p\n", entity->headers );
	if ( entity->headers && entity->headers[0] ) {
		fprintf( stderr, "header (1e): %p %p\n", entity->headers[0], *(entity->headers) );
		fprintf( stderr, "size: %d\n", (*entity->headers)->size );
		fprintf( stderr, "value: %p\n", (*entity->headers)->value );
	}

	fprintf( stderr, "bodylist:   %p\n", entity->body );
	if ( entity->body && (*entity->body) ) {
		fprintf( stderr, "body (1e): %p %p\n", entity->body[0], *(entity->body) );
		fprintf( stderr, "%d\n", (*entity->body)->size );
		fprintf( stderr, "%p\n", (*entity->body)->value );
	}
	#endif

	if ( !entity->headers && !entity->body ) {
		snprintf( err, errlen, "%s", "No headers or body specified with response." );
		return NULL;
	}

	if ( !entity->ctype ) {
		snprintf( err, errlen, "%s", "No Content-Type specified with response." );
		return NULL;
	}

	if ( !entity->status ) {
		snprintf( err, errlen, "%s", "No status specified with response." );
		return NULL;
	}

	if ( (*entity->body) && ( !(*entity->body)->value || !(*entity->body)->size ) ) {
		snprintf( err, errlen, "%s", "No body length specified with response." );
		return NULL;
	}

	//This assumes (perhaps wrongly) that ctype is already set.
	entity->clen = (*entity->body)->size;
	http_header_len = snprintf( http_header_buf, sizeof(http_header_buf) - 1, http_header_fmt,
		entity->status, http_get_status_text( entity->status ), entity->ctype, (*entity->body)->size );

	if ( !append_to_uint8t( &msg, &msglen, (uint8_t *)http_header_buf, http_header_len ) ) {
		snprintf( err, errlen, "%s", "Failed to add default HTTP headers to message." );
		return NULL;
	}

	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	while ( entity->headers && (*entity->headers)->field ) {
		struct HTTPRecord *r = *entity->headers;
		append_to_uint8t( &msg, &msglen, (uint8_t *)r->field, strlen( r->field ) ); 
		append_to_uint8t( &msg, &msglen, (uint8_t *)": ", 2 ); 
		append_to_uint8t( &msg, &msglen, (uint8_t *)r->value, r->size ); 
		append_to_uint8t( &msg, &msglen, (uint8_t *)"\r\n", 2 ); 
		entity->headers++;
	}

	if ( !msg ) {
		snprintf( err, errlen, "Failed to append all headers" );
		return NULL;
	}

	if ( !append_to_uint8t( &msg, &msglen, (uint8_t *)"\r\n", 2 ) ) {
		snprintf( err, errlen, "%s", "Could not add header terminator to message." );
		return NULL;
	}

	if ( !append_to_uint8t( &msg, &msglen, (*entity->body)->value, (*entity->body)->size ) ) {
		snprintf( err, errlen, "%s", "Could not add content to message." );
		return NULL;
	}

	entity->msg = msg;
	entity->mlen = msglen;
	//fprintf( stderr, "entity->msg: %p\n", entity->msg );
	//fprintf( stderr, "entity->mlen : %d\n", entity->mlen );
	return entity;
}

//Pack an HTTP response
//Content-Type
//Content-Length
//Status
//Content
//Additional headers
int http_pack_response ( struct HTTPBody *entity, uint8_t *msg, int msglen, struct HTTPRecord **headers ) {
	//A finished message belongs in r->msg
#if 0
	//Define 
	HTTP_Response *res = &h->response;
	uint8_t statline[1024] = { 0 };
	char ff[4] = { 0 };
	const char *fmt   = "HTTP/%s %d %s\r\n";
	const char *cfmt  = "Content-Length: %d\r\n";
	const char *ctfmt = "Content-Type: %s\r\n\r\n";

	//Set defaults
	(!res->version) ? res->version = 1.1              : 0;
	(!res->ctype)   ? res->ctype   = "text/html"      : 0;
	(!res->status)  ? res->status  = 200              : 0;
	(!res->sttext)  ? res->sttext  = http_status[200] : 0;
	snprintf(ff, 4, (res->version < 2) ? "%1.1f" : "%1.0f", res->version);

	//Copy the status line
	res->mlen += sprintf( (char *)&statline[res->mlen], fmt, 
		ff, res->status, res->sttext);

	//All other headers get res->mlen here
	//....?

	//Always have at least a content length line
	res->mlen += sprintf( (char *)&statline[res->mlen], cfmt, res->clen);

	//Finally, set a content-type
	res->mlen += sprintf( (char *)&statline[res->mlen], ctfmt, res->ctype);

	//Stop now if this is a zero length message.
	if (!res->clen) {
		//memcpy( &res->msg[0], statline, res->mlen );
		if ( !bf_append( h->resb, statline, res->mlen ) ) {
			return 0;
		}
		return 1;
	}

	//Move the message (provided there's space)		
	//This will fail when using fixed buffers...
	if ( !bf_prepend( h->resb, statline, res->mlen ) ) {
		return 0;
	}
	//memmove( &res->msg[res->mlen], &res->msg[0], res->clen);
	//memcpy( &res->msg[0], statline, res->mlen ); 
	res->mlen += res->clen;
#endif
	return 1;
}
