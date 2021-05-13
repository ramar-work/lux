/* ------------------------------------------- * 
 * zhttp.c
 * ---------
 * A C-based HTTP parser, request and response builder 
 *
 * Usage
 * -----
 * Soon to come...
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 12-02-20 - 
 * 
 * ------------------------------------------- */
#include "zhttp.h"

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

const char text_html[] = "text/html";

//Set http errors
static int set_http_error( zhttp_t *entity, HTTP_Error code ) {
	entity->error = code;
	return 0; //Always return false
}


#if 0
static const char *errors[] = {
	
};

//Set fatal errors
static int set_fatal_error( zhttp_t *entity, HTTP_Error code ) {
	entity->error = code;
	return 0; //Always return false
}
#endif


//Copy a string from unsigned data
static char *zhttp_copystr ( unsigned char *src, int len ) {
	len++;
	char *dest = malloc( len );
	memset( dest, 0, len );
	memcpy( dest, src, len - 1 );
	return dest;
}


#if 0 
//Duplicate a string
char * zhttp_dupstr ( const char *v ) {
	int len = strlen( v );
	char * vv = malloc( len + 1 );
	memset( vv, 0, len + 1 );
	memcpy( vv, v, len );
	return vv;
} 
#endif


//Duplicate a block 
unsigned char * zhttp_dupblk( const unsigned char *v, int vlen ) {
	unsigned char * vv = malloc( vlen );
	memset( vv, 0, vlen );
	memcpy( vv, v, vlen );
	return vv;
}


//Generate random characters
char *zhttp_rand_chars ( int len ) {
	const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char * a = malloc( len + 1 );
	memset( a, 0, len + 1 );
	for ( int i = 0; i < len; i ++ ) {
		a[ i ] = chars[ rand() % sizeof(chars) ];
	}
	return a;	
}


//Add to series
static void * zhttp_add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		ZHTTP_PRINTF( "Failed to reallocate block from %d to %d\n", size, size * ((*len) + 2) ); 
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}


//Safely convert numeric buffers...
static int * zhttp_satoi( const char *value, int *p ) {
	//Make sure that content-length numeric
	const char *v = value;
	while ( *v ) {
		if ( (int)*v < 48 || (int)*v > 57 ) return NULL;
		v++;
	}
	*p = atoi( value );
	return p; 
}


//Add to a buffer
unsigned char *zhttp_append_to_uint8t ( unsigned char **dest, int *len, unsigned char *src, int srclen ) {
	if ( !( *dest = realloc( *dest, (*len) + srclen ) ) ) {	
		return NULL;
	}

	if ( !memcpy( &(*dest)[ *len ], src, srclen ) ) {
		return NULL;
	}

	(*len) += srclen;
	return *dest;
}


//Extract value (a simpler code that can be used to grab values)
static char * zhttp_msg_get_value ( const char *value, const char *chrs, unsigned char *msg, int len ) {
	int start=0, end=0;
	char *bContent = NULL;

	if ((start = memstrat( msg, value, len )) > -1) {
		start += strlen( value );
		msg += start;
		int pend = -1;

		//If chrs is more than one character, accept only the earliest match
		while ( *chrs ) {
			end = memchrat( msg, *chrs, len - start );
			if ( end > -1 && pend > -1 && pend < end ) {
				end = pend;	
			}
			pend = end;
			chrs++;	
		}

		//Set 'end' if not already...	
		if ( end == -1 && pend == -1 ) {
			end = len - start; 
		}

		//Prepare for edge cases...
		if ( ( bContent = malloc( len ) ) == NULL ) {
			return NULL; 
		}

		//Prepare the raw buffer..
		memset( bContent, 0, len );	
		memcpy( bContent, msg, end );
	}

	return bContent;
}


//...
const char *http_get_status_text ( HTTP_Status status ) {
	//TODO: This should error out if HTTP_Status is not received...
	if ( status < 100 || status > sizeof( http_status ) / sizeof( char * ) ) {
		return http_status[ 200 ];	
	}
	return http_status[ status ];	
}


//Trim whitespace
unsigned char *httpvtrim (unsigned char *msg, int len, int *nlen) {
	//Define stuff
	unsigned char *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr("\r\n\t ", *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr("\r\n\t ", *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


//Trim any characters 
unsigned char *httptrim (unsigned char *msg, const char *trim, int len, int *nlen) {
	//Define stuff
	unsigned char *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


//...
static zhttpr_t * init_record() {
	zhttpr_t *record = NULL;
	record = malloc( sizeof( zhttpr_t ) );
	if ( !record ) {
		return NULL;
	}
	memset( record, 0, sizeof( zhttpr_t ) );
	return record;
}


//Parse out the URL (or path requested) of an HTTP request 
static int parse_url( zhttp_t *entity, char *err, int errlen ) {
	int l = 0, len = 0;
	zhttpr_t *b = NULL; 
	zWalker set = {0};

	if ( ( l = strlen( entity->path )) == 1 || !memchr( entity->path, '?', l ) ) {
		entity->url = NULL;
		return 1;
	}

	while ( strwalk( &set, entity->path, "?&=" ) ) {
		unsigned char *t = (unsigned char *)&entity->path[ set.pos ];
		if ( set.chr == '?' ) 
			continue;
		else if ( set.chr == '=' ) {
			if ( !( b = init_record() ) ) {
				snprintf( err, errlen, "Memory allocation failure at URL parser" );
				return set_http_error( entity, ZHTTP_OUT_OF_MEMORY );
			}
			b->field = zhttp_copystr( t, set.size - 1 ); 
		}
		else { 
			b->value = t; 
			b->size = ( set.chr == '&' ) ? set.size - 1 : set.size; 
			zhttp_add_item( &entity->url, b, zhttpr_t *, &len );
			b = NULL;
		}
	}
	//headers_length = len; //TODO: some time in the future
	return 1;
}


//Parse out the headers of an HTTP body
static int parse_headers( zhttp_t *entity, char *err, int errlen ) {
	zWalker set = {0};
	int len = 0;
	int flen = strlen( entity->path ) 
						+ strlen( entity->method ) 
						+ strlen( entity->protocol ) + 4;
	unsigned char *rawheaders = &entity->msg[ flen ];
	const unsigned char chars[] = "\r\n";

	while ( memwalk( &set, rawheaders, chars, entity->hlen - flen, 2 ) ) {
		unsigned char *t = &rawheaders[ set.pos ];
		zhttpr_t *b = NULL;
		int pos = -1;
		
		//Character is useless, skip it	
		if ( *t == '\n' )
			continue;
	
		//Copy the header field and value
		if ( ( pos = memchrat( t, ':', set.size ) ) >= 0 ) { 
			//Make a record for the new header line	
			if ( !( b = init_record() ) ) {
				snprintf( err, errlen, "Memory allocation failure at header parser" );
				return set_http_error( entity, ZHTTP_OUT_OF_MEMORY );
			}
			b->field = zhttp_copystr( t, pos ); 
			b->value = ( t += pos + 2 ); 
			b->size = ( set.size - pos - (( set.chr == '\r' ) ? 3 : 2 ) ); 
			zhttp_add_item( &entity->headers, b, zhttpr_t *, &len );
		}
	}
	return 1;
}


//Parse out the parts of an HTTP body
static int parse_body( zhttp_t *entity, char *err, int errlen ) {
	//Always process the body 
	zWalker set;
	memset( &set, 0, sizeof( zWalker ) );
	int len = 0;
	unsigned char *p = &entity->msg[ entity->hlen + 4 ];
	const char *idem = "POST,PUT,PATCH";
	const char *multipart = "multipart/form-data";

	//TODO: If this is a xfer-encoding chunked msg, entity->clen needs to get filled in when done.
	//TODO: Bitmasking is 1% more efficient, go for it.

	//Return early on methods that should have no body 
	if ( !memstr( idem, entity->method, strlen(idem) ) ) {
		//set_http_procstatus( entity );
		entity->body = NULL;
		//ADDITEM( NULL, zhttpr_t, entity->body, len, NULL );
		return 0;
	}

	//Check the content-type and die if it's wrong
	if ( !entity->ctype ) { 
		//set_error( "No content type received." );
		return 0;
	}

	//url encoded is a little bit different.  no real reason to use the same code...
	if ( strcmp( entity->ctype, "application/x-www-form-urlencoded" ) == 0 ) {
		zhttpr_t *b = NULL;
		while ( memwalk( &set, p, (unsigned char *)"=&", entity->clen, 2 ) ) {
			unsigned char *m = &p[ set.pos ];  
			if ( set.chr == '=' ) {
				//TODO: Should be checking that allocation was successful
				b = init_record();
				b->field = zhttp_copystr( m, set.size - 1 );
			}
			else { 
				b->value = m;
				b->size = set.size - (( set.chr == '&' ) ? 1 : 2);
				zhttp_add_item( &entity->body, b, zhttpr_t *, &len );
				b = NULL;
			}
		}
	}
	
	if ( memcmp( multipart, entity->ctype, strlen(multipart) ) == 0 ) {
		char bd[ 128 ];
		memset( &bd, 0, sizeof( bd ) );
		snprintf( bd, 64, "--%s", entity->boundary );
		const int bdlen = strlen( bd );
		int len1 = entity->clen, pp = 0;

		while ( ( pp = memblkat( p, bd, len1, bdlen ) ) > -1 ) {
			int len2 = pp, inner = 0, count = 0;
			unsigned char *i = p;
		#if 0 
			fprintf( stderr, "START POINT======== len: %d \n", len2 ); write( 2, i, len2 );  getchar();
		#endif
			if ( len2 > 0 ) {	
				zhttpr_t *b = init_record();
				b->type = ZHTTP_MULTIPART;
				i += bdlen + 1, len2 -= bdlen - 1;
				//char *name, *filename, *ctype;

				//Boundary was found, so we need to move up again
				while ( ( inner = memblkat( i, "\r\n", len2, 2 ) ) > -1 ) {
				#if 0
					write( 2, i, inner ); fprintf( stderr, "count: %d, inner: %d\n", count, inner ); getchar();
				#endif
					if ( inner == 1 ) 
						; //skip me
					else if ( count > 1 )
						b->size = inner - 1, b->value = i + 1;
					else if ( count == 1 )
						b->ctype = zhttp_msg_get_value( "Content-Type: ", "\r", i, inner );
					else if ( count == 0 ) {
						b->disposition = 
							zhttp_msg_get_value( "Content-Disposition: ", ";", i, inner + 1 );
						b->field = zhttp_msg_get_value( "name=\"", "\"", i, inner + 1 );
						if ( memblkat( i, "filename=", inner, 9 ) > -1 ) {
							b->filename = zhttp_msg_get_value( "filename=\"", "\"", i, inner - 1 );
						}
					}
					++inner, len2 -= inner, i += inner, count++;
				}
			#if 0
				fprintf( stderr, "NAME:\n" );
				fprintf( stderr, "%s\n", b->field );
				fprintf( stderr, "VALUE: %p, %d\n", b->value, b->size );
				write( 2, b->value, b->size );
				getchar();
			#endif
				zhttp_add_item( &entity->body, b, zhttpr_t *, &len );
				b = NULL;
			}
			++pp, len1 -= pp, p += pp;	
		}
	}
	return 1;
}



//Marks the important parts of an HTTP request
static int parse_http_header ( zhttp_t *entity, char *err, int errlen ) {
	//Define stuffs
	const char *methods = "HEAD,GET,POST,PUT,PATCH,DELETE";
	const char *protocols = "HTTP/1.1,HTTP/1.0,HTTP/1,HTTP/0.9";
	int walker = 0;
	zWalker z = {0};

	//Set pointers to zero?
	entity->headers = entity->body = entity->url = NULL;

	//Walk through the first line
	while ( memwalk( &z, entity->msg, (unsigned char *)" \r\n", entity->mlen, 3 ) ) {
		char **ptr = NULL;
		if ( z.chr == ' ' && memchr( "HGPD", entity->msg[ z.pos ], 4 ) )
			ptr = &entity->method;
		else if ( z.chr == ' '  && '/' == entity->msg[ z.pos ] )
			ptr = &entity->path;
		else if ( z.chr == '\r' && entity->protocol )
			break;	
		else if ( z.chr == '\r' )
			ptr = &entity->protocol;
		else if ( z.chr == '\n' )
			break;
		else {
			//TODO: Solve the possible leak that can result here
			snprintf( err, errlen, "Malformed first line of HTTP request." );
			return set_http_error( entity, ZHTTP_MALFORMED_FIRSTLINE );
		}	

		if ( walker++ < 3 ) {  
			*ptr = malloc( z.size );
			memset( *ptr, 0, z.size );
			memcpy( *ptr, &entity->msg[ z.pos ], z.size - 1 ); 
		}
	}

	//Return null if method, path or version are not present
	ZHTTP_PRINTF( "%p %p %p\n", entity->method, entity->path, entity->protocol ); 
	if ( !entity->method || !entity->path || !entity->protocol ) {
		snprintf( err, errlen, "Method, path or HTTP protocol are not present." );
		return set_http_error( entity, ZHTTP_MALFORMED_FIRSTLINE );
	}

	//Fatal 
	if ( !memstr( protocols, entity->protocol, strlen( protocols ) ) ) {
		snprintf( err, errlen, "HTTP protocol '%s' not supported.", entity->protocol );
		return set_http_error( entity, ZHTTP_UNSUPPORTED_PROTOCOL );
	}

	//Fatal
	if ( !memstr( methods, entity->method, strlen( methods ) ) ) {
		snprintf( err, errlen, "HTTP method '%s' not supported", entity->method );
		return set_http_error( entity, ZHTTP_UNSUPPORTED_METHOD );
	}

	//Next, set header length
	if ( ( entity->hlen = memblkat( entity->msg, "\r\n\r\n", entity->mlen, 4 ) ) == -1 ) {
		snprintf( err, errlen, "Header not completely sent." );
		return set_http_error( entity, ZHTTP_INCOMPLETE_HEADER );
	}

	//Get host requested (not always going to be there)
	entity->host = zhttp_msg_get_value( "Host: ", "\r", entity->msg, entity->hlen );

	//If we expect a body, parse it
	if ( memstr( "POST,PUT,PATCH", entity->method, strlen( "POST,PUT,PATCH" ) ) ) {
		char *clenbuf = NULL; 
		int clen;	

		if ( !( clenbuf = zhttp_msg_get_value( "Content-Length: ", "\r", entity->msg, entity->hlen ) ) ) {
			snprintf( err, errlen, "Content-Length header not present..." );
			return set_http_error( entity, ZHTTP_INCOMPLETE_HEADER );
		}

		if ( !zhttp_satoi( clenbuf, &clen ) ) {
			snprintf( err, errlen, "Content-Length doesn't appear to be a number." );
			return set_http_error( entity, ZHTTP_INCOMPLETE_HEADER );
		}

		entity->clen = clen;
		entity->ctype = zhttp_msg_get_value( "Content-Type: ", ";\r", entity->msg, entity->hlen );
	#if 1
		//This is a pretty ugly way to do this; but until I move over to all static allocations, this will have to do.
		char *b = NULL;
		b = zhttp_msg_get_value( "boundary=", "\r", entity->msg, entity->hlen );
		if ( b ) {
			memcpy( entity->boundary, b, strlen( b ) );
			free( b );
		}
	#endif
		free( clenbuf );
	}
	return 1;	
}



//Parse an HTTP request
zhttp_t * http_parse_request ( zhttp_t *entity, char *err, int errlen ) {

	//Set error to none
	entity->error = ZHTTP_NONE;

	//Parse the header
	if ( !parse_http_header( entity, err, errlen ) ) {
		return entity;
	}

	ZHTTP_PRINTF( "Calling parse_url( ... )\n" );
	if ( !parse_url( entity, err, errlen ) ) {
		return entity;
	}

	ZHTTP_PRINTF( "Calling parse_headers( ... )\n" );
	if ( !parse_headers( entity, err, errlen ) ) {
		return entity;
	}

	ZHTTP_PRINTF( "Calling parse_body( ... )\n" );
	if ( !parse_body( entity, err, errlen ) ) {
		return entity;
	}

	//ZHTTP_PRINTF( "Dump http body." );
	//print_httpbody( entity );
	return entity;
} 


//Parse an HTTP response
zhttp_t * http_parse_response ( zhttp_t *entity, char *err, int errlen ) {
	//Prepare the rest of the request
	int hdLen = memstrat( entity->msg, "\r\n\r\n", entity->mlen );

	//Initialize the remainder of variables 
	entity->headers = entity->body = entity->url = NULL;
	entity->hlen = hdLen; 
	entity->host = zhttp_msg_get_value( "Host: ", "\r", entity->msg, hdLen );
	return NULL;
} 



//Finalize an HTTP request (really just returns a unsigned char, but this can handle it)
zhttp_t * http_finalize_request ( zhttp_t *entity, char *err, int errlen ) {
	unsigned char *msg = NULL, *hmsg = NULL;
	int msglen = 0, hmsglen = 0;
	int multipart = 0;
	zhttpr_t **headers = entity->headers;
	zhttpr_t **body = entity->body;
	char clen[ 32 ] = {0};

	if ( !entity->protocol )
		entity->protocol = "HTTP/1.1";

	if ( !entity->path ) {
		snprintf( err, errlen, "%s", "No path specified with request." );
		return NULL;
	}

	if ( !entity->method ) {
		snprintf( err, errlen, "%s", "No method specified with request." );
		return NULL;
	}

	if ( !strcmp( entity->method, "POST" ) || !strcmp( entity->method, "PUT" ) ) {
		if ( !entity->body && !entity->ctype ) {
			snprintf( err, errlen, "Content-type not specified for %s request.", entity->method );
			return NULL;
		}

		if ( ( multipart = ( memcmp( entity->ctype, "multi", 5 ) == 0 ) ) ) {
			char *b = zhttp_rand_chars( 32 );
			memcpy( entity->boundary, b, strlen( b ) );
			memset( entity->boundary, '-', 6 );
			free( b );
		}
	}


	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	while ( headers && *headers ) {
		zhttpr_t *r = *headers;
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->field, strlen( r->field ) ); 
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)": ", 2 ); 
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->value, r->size ); 
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 ); 
		headers++;
	}

	if ( msglen ) {
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 );
		entity->hlen = msglen;
	}
 
	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	if ( strcmp( entity->method, "POST" ) == 0 || strcmp( entity->method, "PUT" ) == 0 ) {
		//app/xwww is % encoded
		//multipart is not (but seperated differently)
		if ( !multipart ) {
			int n = 0;
			while ( body && *body ) {
				zhttpr_t *r = *body;
			#if 0
				zhttp_uchar_join( &msg, &msglen, "&", r->field, "=" ); 
				zhttp_uchar_join( &msg, &msglen, r->value, r->size - 1 );
			#else
				( n ) ? zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"&", 1 ) : 0;
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->field, strlen( r->field ) ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"=", 1 ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->value, r->size - 1 ); 
			#endif
				body++;
			}
		}
		else {
			static const char cdisph[] = "Content-Disposition: " ;
			static const char cdispt[] = "form-data;" ;
			static const char nameh[] = "name=";

			while ( body && *body ) {
				zhttpr_t *r = *body;
			#if 0
				zhttp_uchar_join( &msg, &msglen, entity->boundary, "\r\n" )
			#else
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)entity->boundary, strlen( entity->boundary ) ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)cdisph, sizeof( cdisph ) ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)cdispt, sizeof( cdispt ) ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)nameh, sizeof( nameh ) ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\"", 1 );
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->field, strlen( r->field ) ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\"", 1 );
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n\r\n", 4 ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->value, r->size ); 
				zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 ); 
			#endif
				body++;
			}
			zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)entity->boundary, strlen( entity->boundary ) ); 
			zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"--", 2 ); 
		}
	}

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
			unsigned char buf[ 1024 ] = { 0 };
			int len = snprintf( (char *)buf, sizeof(buf), m[i].fmt, m[i].value ); 
			zhttp_append_to_uint8t( &hmsg, &hmsglen, buf, len );
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
	return entity;
}


//Finalize an HTTP response (really just returns a unsigned char, but this can handle it)
zhttp_t * http_finalize_response ( zhttp_t *entity, char *err, int errlen ) {
	unsigned char *msg = NULL;
	int msglen = 0;
	int http_header_len = 0;
	zhttpr_t **headers = entity->headers;
	zhttpr_t **body = entity->body;
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

	if ( body && *body && ( !(*body)->value || !(*body)->size ) ) {
		snprintf( err, errlen, "%s", "No body length specified with response." );
		return NULL;
	}

	//This assumes (perhaps wrongly) that ctype is already set.
	entity->clen = (*entity->body)->size;
	http_header_len = snprintf( http_header_buf, sizeof(http_header_buf) - 1, http_header_fmt,
		entity->status, http_get_status_text( entity->status ), entity->ctype, (*entity->body)->size );

	if ( !zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)http_header_buf, http_header_len ) ) {
		snprintf( err, errlen, "%s", "Failed to add default HTTP headers to response." );
		return NULL;
	}

	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	while ( headers && *headers ) {
		zhttpr_t *r = *headers;
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->field, strlen( r->field ) ); 
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)": ", 2 ); 
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)r->value, r->size ); 
		zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 ); 
		headers++;
	}

	if ( !msg ) {
		snprintf( err, errlen, "Failed to append all headers" );
		return NULL;
	}

	if ( !zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 ) ) {
		snprintf( err, errlen, "%s", "Could not add header terminator to message." );
		return NULL;
	}

	if ( !zhttp_append_to_uint8t( &msg, &msglen, (*entity->body)->value, (*entity->body)->size ) ) {
		snprintf( err, errlen, "%s", "Could not add content to message." );
		return NULL;
	}

	entity->msg = msg, entity->mlen = msglen;
	return entity;
}


//...
int http_set_int( int *k, int v ) {
	return ( *k = v );
}


//...
char * http_set_char( char **k, const char *v ) {
	return ( *k = zhttp_dupstr( v ) );	
}


//...
void * http_set_record
 ( zhttp_t *entity, zhttpr_t ***list, int type, const char *k, unsigned char *v, int vlen, int free ) {
	zhttpr_t *r = NULL;

	//Block bad types in lieu of an enum
	if ( type < 0 || type > 2 )
		return NULL;

	//Block empty arguments
	if ( !k || ( !v && vlen < 0 ) )
		return NULL;

	//Create a record
	if ( !( r = malloc( sizeof( zhttpr_t ) ) ) ) {
		return NULL;
	}

	//Set the members
	int len = 0;
	len = entity->lengths[ type ];
	r->field = zhttp_dupstr( k ), r->size = vlen, r->value = v, r->free = free;
	zhttp_add_item( list, r, zhttpr_t *, &len );
	entity->lengths[ type ] = len; //entity->size = vlen;
	return r;
}


//...
static void http_free_records( zhttpr_t **records ) {
	zhttpr_t **r = records;
	while ( r && *r ) {
		if ( (*r)->free ) {
			free( (*r)->value );
		}

		if ( (*r)->field ) {
			free( (void *)(*r)->field ); 
		}

		if ( (*r)->type == ZHTTP_MULTIPART ) { 
			(*r)->disposition ? free( (void *)(*r)->disposition ) : 0;
			(*r)->filename ? free( (void *)(*r)->filename ) : 0;
			(*r)->ctype ? free( (void *)(*r)->ctype ) : 0;
		}

		free( *r );
		r++;
	}
	free( records );
}


//...
void http_free_body ( zhttp_t *entity ) {
	//Free all of the header info
	entity->path ? free( entity->path ) : 0;
	entity->ctype ? free( entity->ctype ) : 0;
	entity->host ? free( entity->host ) : 0;
	entity->method ? free( entity->method ) : 0;
	entity->protocol ? free( entity->protocol ) : 0;

	http_free_records( entity->headers );
	http_free_records( entity->url );
	http_free_records( entity->body );

	//Free big message buffer
	if ( entity->msg ) {
		free( entity->msg );
	}	
}


//...
int http_set_error ( zhttp_t *entity, int status, char *message ) {
	char err[ 2048 ];
	memset( err, 0, sizeof( err ) );
	ZHTTP_PRINTF( "status: %d, mlen: %ld, msg: '%s'\n", status, strlen(message), message );

	http_set_status( entity, status );
	http_set_ctype( entity, text_html );
	http_copy_content( entity, (unsigned char *)message, strlen( message ) );

	if ( !http_finalize_response( entity, err, sizeof(err) ) ) {
		ZHTTP_PRINTF( "FINALIZE FAILED!: %s", err );
		return 0;
	}

#if 0
	fprintf(stderr, "msg: " );
	ZHTTP_WRITE( entity->msg, entity->mlen );
#endif
	return 0;
}


#ifdef DEBUG_H
//list out all rows in an HTTPRecord array
void print_httprecords ( zhttpr_t **r ) {
	if ( *r == NULL ) return;
	while ( *r ) {
		ZHTTP_PRINTF( "'%s' -> ", (*r)->field );
		//ZHTTP_PRINTF( "%s\n", (*r)->field );
		ZHTTP_WRITE( "'", 1 );
		ZHTTP_WRITE( (*r)->value, (*r)->size );
		ZHTTP_WRITE( "'\n", 2 );
		r++;
	}
}


//list out everything in an HTTPBody
void print_httpbody ( zhttp_t *r ) {
	if ( r == NULL ) return;
	ZHTTP_PRINTF( "r->mlen: '%d'\n", r->mlen );
	ZHTTP_PRINTF( "r->clen: '%d'\n", r->clen );
	ZHTTP_PRINTF( "r->hlen: '%d'\n", r->hlen );
	ZHTTP_PRINTF( "r->status: '%d'\n", r->status );
	ZHTTP_PRINTF( "r->ctype: '%s'\n", r->ctype );
	ZHTTP_PRINTF( "r->method: '%s'\n", r->method );
	ZHTTP_PRINTF( "r->path: '%s'\n", r->path );
	ZHTTP_PRINTF( "r->protocol: '%s'\n", r->protocol );
	ZHTTP_PRINTF( "r->host: '%s'\n", r->host );
	ZHTTP_PRINTF( "r->boundary: '%s'\n", r->boundary );

	//Print out headers and more
	const char *names[] = { "r->headers", "r->url", "r->body" };
	zhttpr_t **rr[] = { r->headers, r->url, r->body };
	for ( int i=0; i<sizeof(rr)/sizeof(zhttpr_t **); i++ ) {
		ZHTTP_PRINTF( "%s: %p\n", names[i], rr[i] );
		if ( rr[i] ) {
			zhttpr_t **w = rr[i];
			while ( *w ) {
				ZHTTP_WRITE( " '", 2 ); 
				ZHTTP_WRITE( (*w)->field, strlen( (*w)->field ) );
				ZHTTP_WRITE( "' -> '", 6 );
				ZHTTP_WRITE( (*w)->value, (*w)->size );
				ZHTTP_WRITE( "'\n", 2 );
				if ( (*w)->type == ZHTTP_MULTIPART ) {
					ZHTTP_PRINTF( "  Content-Type: %s\n", (*w)->ctype );
					ZHTTP_PRINTF( "  Filename: %s\n", (*w)->filename );
					ZHTTP_PRINTF( "  Content-Disposition: %s\n", (*w)->disposition );
				}
				w++;
			}
		}
	}	
}
#endif
