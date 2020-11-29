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



static struct HTTPRecord * init_record() {
	struct HTTPRecord *record = NULL;
	record = malloc( sizeof( struct HTTPRecord ) );
	if ( !record ) {
		return NULL;
	}
	memset( record, 0, sizeof( struct HTTPRecord ) );
	return record;
}



static struct HTTPRecord ** parse_url( struct HTTPBody *entity, char *err, int errlen ) {
	zWalker set;
	memset( &set, 0, sizeof( zWalker ) );
	int len = 0;

	//Always process the URL (specifically GET vars)
	if ( strlen( entity->path ) == 1 ) {
		entity->url = NULL;
	}
	else {
		int index = 0;
		while ( strwalk( &set, entity->path, "?&" ) ) {
			uint8_t *t = (uint8_t *)&entity->path[ set.pos ];
#if 1
			int at = 0;
			struct HTTPRecord *b = NULL; 
			if ( !( b = init_record() ) ) {
				snprintf( err, errlen, "[%s:%d] zWalkerory allocation failure at URL parsing: %s", __FILE__, __LINE__, entity->method );
				return NULL;
			} 
#else	
			struct HTTPRecord *b = NULL;
			if ( !(b = malloc( sizeof( struct HTTPRecord ) )) ) {
				snprintf( err, errlen, "[%s:%d] zWalkerory allocation failure at URL parsing: %s", __FILE__, __LINE__, entity->method );
				( entity->path ) ? free( entity->path ) : 0;
				( entity->method ) ? free( entity->method ) : 0;
				( entity->protocol ) ? free( entity->protocol ) : 0;
				return NULL;
			}
			memset( b, 0, sizeof( struct HTTPRecord ) );
#endif
			if ( !b || (at = memchrat( t, '=', set.size )) == -1 || !set.size ) 
				;
			else {
				int klen = at;
				b->field = copystr( t, klen );
				klen += 1, t += klen, set.size -= klen;
				b->value = t;
				b->size = set.size;
				add_item( &entity->url, b, struct HTTPRecord *, &len );
			}
		}
	}
	return NULL;
}

static struct HTTPRecord ** parse_headers( struct HTTPBody *entity, char *err, int errlen ) {
	//Always process the headers
	zWalker set;
	memset( &set, 0, sizeof( zWalker ) );
	int len = 0;
	int flLen = 0;

	uint8_t *h = &entity->msg[ flLen - 1 ];
	while ( memwalk( &set, h, (uint8_t *)"\r", entity->hlen, 1 ) ) {
		//Break on newline, and extract the _first_ ':'
		uint8_t *t = &h[ set.pos - 1 ];
		if ( *t == '\r' ) {  
			int at = memchrat( t, ':', set.size );
			struct HTTPRecord *b = malloc( sizeof( struct HTTPRecord ) );
			if ( !b ) {
				snprintf( err, errlen, "[%s:%d] zWalkerory allocation failure at header parsing: %s", __FILE__, __LINE__, entity->method );
#if 0
				( entity->path ) ? free( entity->path ) : 0;
				( entity->method ) ? free( entity->method ) : 0;
				( entity->protocol ) ? free( entity->protocol ) : 0;
#endif
				return NULL;
			}
			memset( b, 0, sizeof( struct HTTPRecord ) );
			if ( !b || at == -1 || at > 127 )
				;
			else {
				at -= 2, t += 2;
				b->field = copystr( t, at );
FPRINTF( "b->field '%s'\n", b->field );
				at += 2 /*Increment to get past ': '*/, t += at, set.size -= at;
				b->value = t;
				b->size = set.size - 1;
				//ADDITEM( b, struct HTTPRecord, entity->headers, len, NULL );
				add_item( &entity->headers, b, struct HTTPRecord *, &len );
			}
		}
	}
	return NULL;
}

static struct HTTPRecord ** parse_body( struct HTTPBody *entity, char *err, int errlen ) {
	//Always process the body 
	zWalker set;
	memset( &set, 0, sizeof( zWalker ) );
	int len = 0;
	uint8_t *p = &entity->msg[ entity->hlen + strlen( "\r\n" ) ];
	int plen = entity->mlen - entity->hlen;
	
	//TODO: If this is a xfer-encoding chunked msg, entity->clen needs to get filled in when done.
	if ( strcmp( "POST", entity->method ) != 0 ) {
		entity->body = NULL;
		//ADDITEM( NULL, struct HTTPRecord, entity->body, len, NULL );
	}
	else {
		struct HTTPRecord *b = NULL;
		//TODO: Bitmasking is 1% more efficient, go for it.
		int name=0, value=0, index=0;

		//Check the content-type and die if it's wrong
		if ( !entity->ctype ) {
			snprintf( err, errlen, "[%s:%d] No content-type given for method %s.", __FILE__, __LINE__, entity->method);
			( entity->path ) ? free( entity->path ) : 0;
			( entity->method ) ? free( entity->method ) : 0;
			( entity->protocol ) ? free( entity->protocol ) : 0;
			return NULL;
		}
		if ( memcmp( entity->ctype, "appli", 5 ) != 0 && memcmp( entity->ctype, "multi", 5 ) != 0 ) {
			//Free headers and what not
			snprintf( err, errlen, "[%s:%d] Unsupported content-type '%s' for method %s.", __FILE__, __LINE__, entity->ctype , entity->method );
			( entity->path ) ? free( entity->path ) : 0;
			( entity->method ) ? free( entity->method ) : 0;
			( entity->protocol ) ? free( entity->protocol ) : 0;
			return NULL;
		} 

		//url encoded is a little bit different.  no real reason to use the same code...
		if ( strcmp( entity->ctype, "application/x-www-form-urlencoded" ) == 0 ) {
			//NOTE: clen is up by two to assist our little tokenizer...
			while ( memwalk( &set, p, (uint8_t *)"\n=&", entity->clen + 2, 3 ) ) {
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( *m == '\n' || *m == '&' ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
					//TODO: Should be checking that allocation was successful
					b->field = copystr( ++m, set.size );
				}
				else if ( *m == '=' ) {
					b->value = ++m;
					b->size = set.size;
					//ADDITEM( b, struct HTTPRecord, entity->body, len, NULL );
					add_item( &entity->body, b, struct HTTPRecord *, &len );
					b = NULL;
				}
			}
		}
		else {
			while ( memwalk( &set, p, (uint8_t *)"\r:=;", entity->clen, 4 ) ) {
				//TODO: If we're being technical, set.pos - 1 can point to a negative index.  
				//However, as long as headers were sent (and 99.99999999% of the time they will be)
				//this negative index will point to valid allocated memory...
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( memcmp( m, "; name=", 7 ) == 0 )
					name = 1;
				//"\r\n\r\n"
				else if ( memcmp( m, "\r\n\r\n", 4 ) == 0 && !value )
					value = 1;
				else if ( memcmp( m, "\r\n-", 3 ) == 0 && !value ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
				}
				else if ( memcmp( m, "\r\n", 2 ) == 0 && value == 1 ) {
					m += 2;
					b->value = m;//++t;
					b->size = set.size - 1;
					//ADDITEM( b, struct HTTPRecord, entity->body, len, NULL );
					add_item( &entity->body, b, struct HTTPRecord *, &len );
					value = 0;
					b = NULL;
				}
				else if ( *m == '=' && name == 1 ) {
					//fprintf( stderr, "copying name field... pass %d\n", ++index );
					int size = *(m + 1) == '"' ? set.size - 2 : set.size;
				#if 1
					m += ( *(m + 1) == '"' ) ? 2 : 1 ;
				#else
					int ptrinc = *(m + 1) == '"' ? 2 : 1;
					m += ptrinc;
				#endif
					b->field = copystr( m, size );
					name = 0;
				}
			}
		}

		//Add a terminator element
		//ADDITEM( NULL, struct HTTPRecord, entity->body, len, NULL );
		//This MAY help in handling malformed messages...
		( b && (!b->field || !b->value) ) ? free( b ) : 0;
	}
	return NULL;
}


//Marks the important parts of an HTTP request
static struct HTTPBody * stake_body ( struct HTTPBody *entity, char *err, int errlen ) {

	//Check that mlen is NOT blank
	FPRINTF( "Message length at stake body is %d\n", entity->mlen );
	FPRINTF( "Message ptr at stake body is %p\n", entity->msg );

	//Prepare the rest of the request
	char *header = (char *)entity->msg;
	int pLen = memchrat( entity->msg, '\n', entity->mlen ) - 1;
	const int flLen = pLen + strlen( "\r\n" );
	FPRINTF( "ptr: %p, len: %d\n", entity->msg, entity->mlen );
	int hdLen = memstrat( entity->msg, "\r\n\r\n", entity->mlen );

	//Initialize the remainder of variables 
	entity->headers = NULL;
	entity->body = NULL;
	entity->url = NULL;
	entity->method = get_lstr( &header, ' ', &pLen );
	entity->path = get_lstr( &header, ' ', &pLen );
	entity->protocol = get_lstr( &header, ' ', &pLen ); 
	entity->hlen = hdLen; 
	entity->host = msg_get_value( "Host: ", "\r", entity->msg, hdLen );

	//The protocol parsing can happen here...
	if ( strcmp( entity->method, "HEAD" ) == 0 )
		;
	else if ( strcmp( entity->method, "GET" ) == 0 )
		;
	else if ( strcmp( entity->method, "POST" ) == 0 || strcmp( entity->method, "PUT" ) ) {
		char *clen = msg_get_value( "Content-Length: ", "\r", entity->msg, hdLen ); 
		entity->clen = safeatoi( clen );
		entity->ctype = msg_get_value( "Content-Type: ", ";\r", entity->msg, hdLen );
		entity->boundary = msg_get_value( "boundary=", "\r", entity->msg, hdLen );
		//entity->mlen = hdLen; 
		//If clen is -1, ... hmmm.  At some point, I still need to do the rest of the work. 
		free( clen );
	}
	else {
		snprintf( err, errlen, "Unsupported method requested: %s", entity->method );
		return NULL;
	}
	return entity;	
}


//Parse an HTTP request
struct HTTPBody * http_parse_request ( struct HTTPBody *entity, char *err, int errlen ) {

#if 1
	FPRINTF( "Calling stake_body( ... )\n" );
	if ( !stake_body( entity, err, errlen ) ) {
		return NULL;
	}
#else
	//Prepare the rest of the request
	char *header = (char *)entity->msg;
	int pLen = memchrat( entity->msg, '\n', entity->mlen ) - 1;
	const int flLen = pLen + strlen( "\r\n" );
	int hdLen = memstrat( entity->msg, "\r\n\r\n", entity->mlen );

	//Initialize the remainder of variables 
	entity->headers = NULL;
	entity->body = NULL;
	entity->url = NULL;
	entity->method = get_lstr( &header, ' ', &pLen );
	entity->path = get_lstr( &header, ' ', &pLen );
	entity->protocol = get_lstr( &header, ' ', &pLen ); 
	entity->hlen = hdLen; 
	entity->host = msg_get_value( "Host: ", "\r", entity->msg, hdLen );

	//The protocol parsing can happen here...
	if ( strcmp( entity->method, "HEAD" ) == 0 )
		;
	else if ( strcmp( entity->method, "GET" ) == 0 )
		;
	else if ( strcmp( entity->method, "POST" ) == 0 || strcmp( entity->method, "PUT" ) ) {
		char *clen = msg_get_value( "Content-Length: ", "\r", entity->msg, hdLen ); 
		entity->clen = safeatoi( clen );
		entity->ctype = msg_get_value( "Content-Type: ", ";\r", entity->msg, hdLen );
		entity->boundary = msg_get_value( "boundary=", "\r", entity->msg, hdLen );
		//entity->mlen = hdLen; 
		//If clen is -1, ... hmmm.  At some point, I still need to do the rest of the work. 
	}
	else {
		snprintf( err, errlen, "[%s:%d] Unsupported method requested: %s", __FILE__, __LINE__, entity->method );
#if 0
		( entity->path ) ? free( entity->path ) : 0;
		( entity->method ) ? free( entity->method ) : 0;
		( entity->protocol ) ? free( entity->protocol ) : 0;
#endif
		return NULL;
	}
#endif

#if 0
#if 1
	FPRINTF( "Calling parse_url( ... )\n" );
	if ( !parse_url( entity, err, errlen ) ) {
		return NULL;
	}
#else
	//Define records for each type here...
	int len = 0;
	zWalker set;
	memset( &set, 0, sizeof( zWalker ) );

	//Always process the URL (specifically GET vars)
	if ( strlen( entity->path ) == 1 ) {
		entity->url = NULL;
	}
	else {
		int index = 0;
		while ( strwalk( &set, entity->path, "?&" ) ) {
			uint8_t *t = (uint8_t *)&entity->path[ set.pos ];
#if 1
			int at = 0;
			struct HTTPRecord *b = NULL; 
			if ( !( b = init_record() ) ) {
				snprintf( err, errlen, "[%s:%d] zWalkerory allocation failure at URL parsing: %s", __FILE__, __LINE__, entity->method );
				return NULL;
			} 
#else	
			struct HTTPRecord *b = NULL;
			if ( !(b = malloc( sizeof( struct HTTPRecord ) )) ) {
				snprintf( err, errlen, "[%s:%d] zWalkerory allocation failure at URL parsing: %s", __FILE__, __LINE__, entity->method );
				( entity->path ) ? free( entity->path ) : 0;
				( entity->method ) ? free( entity->method ) : 0;
				( entity->protocol ) ? free( entity->protocol ) : 0;
				return NULL;
			}
			memset( b, 0, sizeof( struct HTTPRecord ) );
#endif
			if ( !b || (at = memchrat( t, '=', set.size )) == -1 || !set.size ) 
				;
			else {
				int klen = at;
				b->field = copystr( t, klen );
				klen += 1, t += klen, set.size -= klen;
				b->value = t;
				b->size = set.size;
				add_item( &entity->url, b, struct HTTPRecord *, &len );
			}
		}
	}
#endif


#if 1
	FPRINTF( "Calling parse_headers( ... )\n" );
	if ( !parse_headers( entity, err, errlen ) ) {
		return NULL;
	}
#else
	//Always process the headers
	memset( &set, 0, sizeof( zWalker ) );
	len = 0;
	uint8_t *h = &entity->msg[ flLen - 1 ];
	while ( memwalk( &set, h, (uint8_t *)"\r", entity->hlen, 1 ) ) {
		//Break on newline, and extract the _first_ ':'
		uint8_t *t = &h[ set.pos - 1 ];
		if ( *t == '\r' ) {  
			int at = memchrat( t, ':', set.size );
			struct HTTPRecord *b = malloc( sizeof( struct HTTPRecord ) );
			if ( !b ) {
				snprintf( err, errlen, "[%s:%d] zWalkerory allocation failure at header parsing: %s", __FILE__, __LINE__, entity->method );
				( entity->path ) ? free( entity->path ) : 0;
				( entity->method ) ? free( entity->method ) : 0;
				( entity->protocol ) ? free( entity->protocol ) : 0;
				return NULL;
			}
			memset( b, 0, sizeof( struct HTTPRecord ) );
			if ( !b || at == -1 || at > 127 )
				;
			else {
				at -= 2, t += 2;
				b->field = copystr( t, at );
FPRINTF( "b->field '%s'\n", b->field );
				at += 2 /*Increment to get past ': '*/, t += at, set.size -= at;
				b->value = t;
				b->size = set.size - 1;
				//ADDITEM( b, struct HTTPRecord, entity->headers, len, NULL );
				add_item( &entity->headers, b, struct HTTPRecord *, &len );
			}
		}
	}
#endif



#if 1
	FPRINTF( "Calling parse_body( ... )\n" );
	if ( !parse_body( entity, err, errlen ) ) {
		return NULL;
	}
#else
	//Always process the body 
	memset( &set, 0, sizeof( zWalker ) );
	len = 0;
	uint8_t *p = &entity->msg[ entity->hlen + strlen( "\r\n" ) ];
	int plen = entity->mlen - entity->hlen;
	
	//TODO: If this is a xfer-encoding chunked msg, entity->clen needs to get filled in when done.
	if ( strcmp( "POST", entity->method ) != 0 ) {
		entity->body = NULL;
		//ADDITEM( NULL, struct HTTPRecord, entity->body, len, NULL );
	}
	else {
		struct HTTPRecord *b = NULL;
		//TODO: Bitmasking is 1% more efficient, go for it.
		int name=0, value=0, index=0;

		//Check the content-type and die if it's wrong
		if ( !entity->ctype ) {
			snprintf( err, errlen, "[%s:%d] No content-type given for method %s.", __FILE__, __LINE__, entity->method);
			( entity->path ) ? free( entity->path ) : 0;
			( entity->method ) ? free( entity->method ) : 0;
			( entity->protocol ) ? free( entity->protocol ) : 0;
			return NULL;
		}
		if ( memcmp( entity->ctype, "appli", 5 ) != 0 && memcmp( entity->ctype, "multi", 5 ) != 0 ) {
			//Free headers and what not
			snprintf( err, errlen, "[%s:%d] Unsupported content-type '%s' for method %s.", __FILE__, __LINE__, entity->ctype , entity->method );
			( entity->path ) ? free( entity->path ) : 0;
			( entity->method ) ? free( entity->method ) : 0;
			( entity->protocol ) ? free( entity->protocol ) : 0;
			return NULL;
		} 

		//url encoded is a little bit different.  no real reason to use the same code...
		if ( strcmp( entity->ctype, "application/x-www-form-urlencoded" ) == 0 ) {
			//NOTE: clen is up by two to assist our little tokenizer...
			while ( memwalk( &set, p, (uint8_t *)"\n=&", entity->clen + 2, 3 ) ) {
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( *m == '\n' || *m == '&' ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
					//TODO: Should be checking that allocation was successful
					b->field = copystr( ++m, set.size );
				}
				else if ( *m == '=' ) {
					b->value = ++m;
					b->size = set.size;
					//ADDITEM( b, struct HTTPRecord, entity->body, len, NULL );
					add_item( &entity->body, b, struct HTTPRecord *, &len );
					b = NULL;
				}
			}
		}
		else {
			while ( memwalk( &set, p, (uint8_t *)"\r:=;", entity->clen, 4 ) ) {
				//TODO: If we're being technical, set.pos - 1 can point to a negative index.  
				//However, as long as headers were sent (and 99.99999999% of the time they will be)
				//this negative index will point to valid allocated memory...
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( memcmp( m, "; name=", 7 ) == 0 )
					name = 1;
				//"\r\n\r\n"
				else if ( memcmp( m, "\r\n\r\n", 4 ) == 0 && !value )
					value = 1;
				else if ( memcmp( m, "\r\n-", 3 ) == 0 && !value ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
				}
				else if ( memcmp( m, "\r\n", 2 ) == 0 && value == 1 ) {
					m += 2;
					b->value = m;//++t;
					b->size = set.size - 1;
					//ADDITEM( b, struct HTTPRecord, entity->body, len, NULL );
					add_item( &entity->body, b, struct HTTPRecord *, &len );
					value = 0;
					b = NULL;
				}
				else if ( *m == '=' && name == 1 ) {
					//fprintf( stderr, "copying name field... pass %d\n", ++index );
					int size = *(m + 1) == '"' ? set.size - 2 : set.size;
				#if 1
					m += ( *(m + 1) == '"' ) ? 2 : 1 ;
				#else
					int ptrinc = *(m + 1) == '"' ? 2 : 1;
					m += ptrinc;
				#endif
					b->field = copystr( m, size );
					name = 0;
				}
			}
		}

		//Add a terminator element
		//ADDITEM( NULL, struct HTTPRecord, entity->body, len, NULL );
		//This MAY help in handling malformed messages...
		( b && (!b->field || !b->value) ) ? free( b ) : 0;
	}
#endif
#endif

	FPRINTF( "Dump http body." );
print_httpbody( entity );
	return entity;
} 


//Parse an HTTP response
struct HTTPBody * http_parse_response ( struct HTTPBody *entity, char *err, int errlen ) {
	//Prepare the rest of the request
	char *header = (char *)entity->msg;
	int pLen = memchrat( entity->msg, '\n', entity->mlen ) - 1;
	const int flLen = pLen + strlen( "\r\n" );
	int hdLen = memstrat( entity->msg, "\r\n\r\n", entity->mlen );

	//Initialize the remainder of variables 
	entity->headers = NULL;
	entity->body = NULL;
	entity->url = NULL;
	entity->protocol = get_lstr( &header, ' ', &pLen ); 
	char *status = get_lstr( &header, ' ', &pLen );
	entity->status = safeatoi( status );
	free( status );
	//char *status_text = get_lstr( &header, ' ', &pLen );
	entity->hlen = hdLen; 
	entity->host = msg_get_value( "Host: ", "\r", entity->msg, hdLen );

print_httpbody( entity );
	return NULL;
} 



//Finalize an HTTP request (really just returns a uint8_t, but this can handle it)
struct HTTPBody * http_finalize_request ( struct HTTPBody *entity, char *err, int errlen ) {
	uint8_t *msg = NULL;
	int msglen = 0;
	int http_header_len = 0;
	int multipart = 0;
	struct HTTPRecord **headers = NULL;
	struct HTTPRecord **body = NULL;
	char http_header_buf[ 2048 ] = { 0 };
	char http_header_fmt[] = "%s %s %s\r\n";
	const char http_host_header_str[] = "Host: %s";
	const char http_content_length_header_str[] = "Content-Length: %d";
	const char http_content_type_header_str[] = "Content-Type: %s";

	if ( entity->boundary ) {
		free( entity->boundary );
	}

	if ( !entity->protocol ) {
		entity->protocol = "HTTP/1.1";
	}

	if ( !entity->path ) {
		snprintf( err, errlen, "%s", "No path specified with request." );
		return NULL;
	}

	if ( !entity->method ) {
		snprintf( err, errlen, "%s", "No method specified with request." );
		return NULL;
	}

	if ( strcmp( entity->method, "POST" ) == 0 || strcmp( entity->method, "PUT" ) == 0 ) {
		if ( !entity->body && !entity->ctype ) {
			snprintf( err, errlen, "Content-type not specified for %s request.", entity->method );
			return NULL;
		}

		if ( ( multipart = ( memcmp( entity->ctype, "multi", 5 ) == 0 ) ) ) {
			entity->boundary = mrand_chars( 32 );
			memset( entity->boundary, '-', 6 );
		}
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

	if ( msglen ) {
		append_to_uint8t( &msg, &msglen, (uint8_t *)"\r\n", 2 );
		entity->hlen = msglen;
	}
 
	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	if ( strcmp( entity->method, "POST" ) == 0 || strcmp( entity->method, "PUT" ) == 0 ) {
		//app/xwww is % encoded
		//multipart is not (but seperated differently)
		if ( !multipart ) {
			int n = 0;
			while ( entity->body && (*entity->body)->field ) {
				struct HTTPRecord *r = *entity->body;
				( n ) ? append_to_uint8t( &msg, &msglen, (uint8_t *)"&", 1 ) : 0;
				append_to_uint8t( &msg, &msglen, (uint8_t *)r->field, strlen( r->field ) ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)"=", 1 ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)r->value, r->size - 1 ); 
				entity->body++, n++;
			}
		}
		else {
			while ( entity->body && (*entity->body)->field ) {
				struct HTTPRecord *r = *entity->body;
				const char cdisph[] = "Content-Disposition: " ;
				const char cdispt[] = "form-data;" ;
				const char nameh[] = "name=";
				const char filename[] = "filename=";
				append_to_uint8t( &msg, &msglen, (uint8_t *)entity->boundary, strlen( entity->boundary ) ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)"\r\n", 2 ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)cdisph, sizeof( cdisph ) ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)cdispt, sizeof( cdispt ) ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)nameh, sizeof( nameh ) ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)"\"", 1 );
				append_to_uint8t( &msg, &msglen, (uint8_t *)r->field, strlen( r->field ) ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)"\"", 1 );
				append_to_uint8t( &msg, &msglen, (uint8_t *)"\r\n\r\n", 4 ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)r->value, r->size ); 
				append_to_uint8t( &msg, &msglen, (uint8_t *)"\r\n", 2 ); 
				entity->body++;
			}
			append_to_uint8t( &msg, &msglen, (uint8_t *)entity->boundary, strlen( entity->boundary ) ); 
			append_to_uint8t( &msg, &msglen, (uint8_t *)"--", 2 ); 
		}
	}

	char clen[ 32 ] = {0};
	uint8_t *hmsg = NULL;
	int hmsglen = 0;
	entity->clen = msglen - entity->hlen;
	snprintf( clen, sizeof( clen ), "%d", entity->clen );

	//There should be a cleaner way to handle this
	struct t { const char *value, *fmt, reqd; } m[] = {
		{ entity->method, "%s ", 1 },
		{ entity->path, "%s ", 1 },
		{ entity->protocol, "%s\r\n", 1 },
		{ clen, "Content-Length: %s\r\n", strcmp(entity->method,"POST") == 0 ? 1 : 0 },
		{ entity->ctype, "Content-Type: %s", strcmp(entity->method,"POST") == 0 ? 1 : 0 },
		{ (multipart) ? entity->boundary : "", (multipart) ? ";boundary=\"%s\"\r\n" : "%s\r\n", strcmp(entity->method,"POST") == 0 ? 1 : 0 },
		{ entity->host, "Host: %s\r\n" },
	};
	for ( int i = 0; i < sizeof(m)/sizeof(struct t); i++ ) {
		if ( !m[i].reqd && !m[i].value )
			continue;
		else if ( m[i].reqd && !m[i].value ) {
			snprintf( err, errlen, "%s", "Failed to add HTTP metadata to request." );
			return NULL;
		}
		else {
			//Add whatever value
			uint8_t buf[ 1024 ] = { 0 };
			int len = snprintf( (char *)buf, sizeof(buf), m[i].fmt, m[i].value ); 
			append_to_uint8t( &hmsg, &hmsglen, buf, len );
			entity->hlen += len;
		}
	}

	if ( !( entity->msg = malloc( msglen + hmsglen ) ) ) {
		snprintf( err, errlen, "%s", "Failed to reallocate message buffer." );
		return NULL;
	}

	memcpy( &entity->msg[0], hmsg, hmsglen );
	entity->mlen = hmsglen; 
	memcpy( &entity->msg[entity->mlen], msg, msglen );
	entity->mlen += msglen; 

	free( msg );
	free( hmsg );
	free( entity->boundary );
	return entity;
}


//Finalize an HTTP response (really just returns a uint8_t, but this can handle it)
struct HTTPBody * http_finalize_response ( struct HTTPBody *entity, char *err, int errlen ) {
	uint8_t *msg = NULL;
	int msglen = 0;
	int http_header_len = 0;
	struct HTTPRecord **headers = NULL;
	struct HTTPRecord **body = NULL;
	char http_header_buf[ 2048 ] = { 0 };
	char http_header_fmt[] = "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n";

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

FPRINTF( "HTTP BODY ptr: %p, size: %d\n", (*entity->body)->value, (*entity->body)->size ); 
	if ( (*entity->body) && ( !(*entity->body)->value || !(*entity->body)->size ) ) {
		snprintf( err, errlen, "%s", "No body length specified with response." );
		return NULL;
	}

	//This assumes (perhaps wrongly) that ctype is already set.
	entity->clen = (*entity->body)->size;
	http_header_len = snprintf( http_header_buf, sizeof(http_header_buf) - 1, http_header_fmt,
		entity->status, http_get_status_text( entity->status ), entity->ctype, (*entity->body)->size );

	if ( !append_to_uint8t( &msg, &msglen, (uint8_t *)http_header_buf, http_header_len ) ) {
		snprintf( err, errlen, "%s", "Failed to add default HTTP headers to response." );
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

	int esize = (*entity->body)->size;
#if 0
FPRINTF( "ENTITY WRITE: %p %d\n", (*entity->body)->value, esize );
	for ( int i=0; i < esize; i++ ) {
FPRINTF( "ENTITY: %2c\n", (*entity->body)->value[ i ] );
	}
#endif

	if ( !append_to_uint8t( &msg, &msglen, (*entity->body)->value, esize ) ) {
		snprintf( err, errlen, "%s", "Could not add content to message." );
		return NULL;
	}

	entity->msg = msg;
	entity->mlen = msglen;
	return entity;
}

int http_set_int( int *k, int v ) {
	return ( *k = v );
}

char * http_set_char( char **k, const char *v ) {
	return ( *k = (char *)v );	
}

void * http_set_record( struct HTTPBody *entity, struct HTTPRecord ***list, int type, const char *k, uint8_t *v, int vlen ) {
FPRINTF( "HTTP BODY: %p %d\n", v, vlen ); 
	int len = 0;
	struct HTTPRecord *r = NULL;

	//Block bad types in lieu of an enum
	if ( type < 0 || type > 2 ) {
		return NULL;
	}

	//Block empty arguments
	if ( !k || ( !v && vlen < 0 ) ) {
		return NULL;
	}

	//We use entity->boundary as a hack to store the current index.
	if ( !entity->boundary ) {
		entity->boundary = malloc( 4 );
		memset( entity->boundary, 0, 4 );
	}

	if ( !( r = malloc( sizeof( struct HTTPRecord ) ) ) ) {
		return NULL;
	}

	//Set the members
	len = entity->boundary[ type ];
	r->field = strdup( k );
	r->size = vlen;
	if ( ( r->value = malloc( vlen ) ) == NULL ) {
		free( r );
		free( entity->boundary );
		return NULL;
	}

	memset( r->value, 0, vlen );
	memcpy( r->value, v, vlen );
FPRINTF( "HTTP BODY ptr: %p, size: %d\n", r->value, r->size );

	add_item( list, r, struct HTTPRecord *, &len );
	//entity->size = vlen;
	entity->boundary[ type ] = len;
	return r;
}


void http_free_records( struct HTTPRecord **records ) {
	struct HTTPRecord **r = records;
	while ( r && *r ) {
		(*r)->field ? free( (void *)(*r)->field ) : 0;
		(*r)->value ? free( (*r)->value ) : 0;
		free( *r );
		r++;
	}
	free( records );
}


void http_free_body ( struct HTTPBody *entity ) {
	//Free all of the header info
	entity->path ? free( entity->path ) : 0;
	entity->method ? free( entity->method ) : 0;
	entity->protocol ? free( entity->protocol ) : 0;
	entity->host ? free( entity->host ) : 0;
	entity->boundary ? free( entity->boundary ) : 0;
	http_free_records( entity->headers );
	http_free_records( entity->url );
	http_free_records( entity->body );

	//Free big message buffer
	if ( entity->msg ) {
		free( entity->msg );
	}	
}


int http_set_error ( struct HTTPBody *entity, int status, char *message ) {
	char err[ 2048 ];
	memset( err, 0, sizeof( err ) );
	FPRINTF( "status: %d, mlen: %ld, msg: '%s'\n", status, strlen(message), message );

	if ( !http_set_status( entity, status ) ) {
		fprintf( stderr, "SET STATUS FAILED!" );
		return 0;
	}

	if ( !http_set_ctype( entity, strdup( "text/html" ) ) ) {
		fprintf( stderr, "SET CONTENT FAILED!" );
		return 0;
	}

	if ( !http_set_content( entity, (uint8_t *)message, strlen( message ) ) ) {
		fprintf( stderr, "SET CONTENT FAILED!" );
		return 0;
	}

	if ( !http_finalize_response( entity, err, sizeof(err) ) ) {
		fprintf( stderr, "FINALIZE FAILED!: %s", err );
		return 0;
	}

#if 0
	fprintf(stderr, "msg: " );
	write( 2, entity->msg, entity->mlen );
#endif
	return 0;
}

