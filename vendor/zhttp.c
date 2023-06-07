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

#define nonfatal_error(entity,code) \
	set_error( entity, code, ZHTTP_NONFATAL )

#define fatal_error(entity,code) \
	set_error( entity, code, ZHTTP_FATAL )

#define FIND_AND_REPLACE(p) \
	for ( unsigned char *_w = p; *_w != ' ' || ( *_w = '\0' ); _w++ ) ; 

#define FIND_REPLACE_AND_ADVANCE_PTR(p,x) \
	for ( unsigned char *_w = p; *_w != ' ' || ( *_w = '\0' ); _w++, (x)++ ) ; 

static const char zhttp_multipart[] =
	"multipart/form-data";

static const char zhttp_url_encoded[] =
	"application/x-www-form-urlencoded"; 

static const char zhttp_idempotent_methods[] =
	"POST,PUT,PATCH,DELETE";

static const char zhttp_supported_methods[] =
	"HEAD,GET,POST,PUT,PATCH,DELETE";

static const char zhttp_supported_protocols[] = 
	"HTTP/1.1,HTTP/1.0,HTTP/1,HTTP/0.9";

static const char *zhttp_idempotent_methods2[] = {
	"POST"
, "PUT"
, "PATCH"
, "DELETE"
, NULL
};

static const char *zhttp_supported_methods2[] = {
	"HEAD"
, "GET"
, "POST"
, "PUT"
, "PATCH"
, "DELETE"
, NULL
};

static const char *zhttp_supported_protocols2[] = {
	"HTTP/1.1"
, "HTTP/1.0"
, "HTTP/1"
, "HTTP/0.9"
, NULL
};


static const char *zhttp_content_type_id[] = { 
	"Content-Type"
, "content-type"
, "Content-type"
, NULL 
};

static const char *zhttp_content_length_id[] = { 
	"Content-Length"
, "content-length"
, "Content-length"
, NULL 
};

static const char base[] = { 
	['0'] = 0
,	['1'] = 1
,	['2'] = 2
,	['3'] = 3
,	['4'] = 4
,	['5'] = 5
,	['6'] = 6
,	['7'] = 7
,	['8'] = 8
,	['9'] = 9
,	['a'] = 10 
,	['b'] = 11
,	['c'] = 12 
,	['d'] = 13
,	['e'] = 14
,	['f'] = 15
};

static const char cdisph[] = "Content-Disposition: " ;

static const char cdispt[] = "form-data;" ;

static const char nameh[] = "name=";

static const char default_content_type[] = 
	"application/octet-stream";

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


static const char *errors[] = {
	[ZHTTP_NONE] = "No error"
,	[ZHTTP_PROGRAMMER_ERROR] = "Programmer error."
,	[ZHTTP_HEADER_LENGTH_UNSET] = "Header length unset."
,	[ZHTTP_AWAITING_HEADER ] = "Awaiting HTTP header"
,	[ZHTTP_INCOMPLETE_METHOD] = "HTTP method incomplete"
, [ZHTTP_BAD_PATH] = "Path for HTTP request was incomplete"
, [ZHTTP_INCOMPLETE_PROTOCOL] = "HTTP protocol invalid"
, [ZHTTP_INCOMPLETE_HEADER] = "HTTP header is invalid"
, [ZHTTP_INCOMPLETE_QUERYSTRING] = "HTTP querystring is invalid"
, [ZHTTP_UNSUPPORTED_METHOD] = "Got unsupported HTTP method"
, [ZHTTP_UNSUPPORTED_PROTOCOL] = "Got unsupported HTTP protocol"
, [ZHTTP_UNSPECIFIED_CONTENT_TYPE] = "No content type specified"
, [ZHTTP_INVALID_CONTENT_LENGTH] = "Invalid content length specified"
, [ZHTTP_UNSPECIFIED_MULTIPART_BOUNDARY] = "No boundary speciifed for content type multipart/form-data"
, [ZHTTP_INVALID_PORT] = "Invalid port specified"
,	[ZHTTP_MALFORMED_HEADERS] = "Headers are either malformed or not present."
, [ZHTTP_MALFORMED_FIRSTLINE] = "Got malformed HTTP message"
, [ZHTTP_MALFORMED_FORMDATA] = "Got malformed data from submitted form"
, [ZHTTP_OUT_OF_MEMORY] = "Out of memory"
};

static const char text_html[] = "text/html";

static const char idem[] = "POST,PUT,PATCH,DELETE";

//Set http errors
static zhttp_t * set_error( zhttp_t *en, HTTP_Error code, HTTP_ErrorType type ) {
	en->error = code;
	en->efatal = type;
	en->errmsg = errors[ code ];
	return NULL;
}


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



//Return a single char based on code... 
char zhttp_url_decode ( unsigned char *p, int len ) {
	//const char reserved[] = ";/?:@=&";
	if ( *p != '%' ) {
		return 0;
	}

	char k = 0;
	for ( unsigned char *x = p; len; len--, x++ ) {
		if ( len == 2 )
			k += base[ (int)*x ] * 16;
		else if ( len == 1 )
			k += base[ (int)*x ];
		else if ( len == 3 && *x == '%' )  {
			continue;
		}
	}
	
	return (char)k;
}


//Add to series
static void * zhttp_add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		ZHTTP_PRINTF( stderr, "Failed to reallocate block from %d to %d\n", size, size * ((*len) + 2) ); 
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
	char *content = NULL;

	if ( ( start = memstrat( msg, value, len ) ) > -1) {
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
		if ( ( content = malloc( len ) ) == NULL ) {
			return NULL; 
		}

		//Prepare the raw buffer..
		memset( content, 0, len );	
		memcpy( content, msg, end );
	}

	return content;
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





#if 0

//Parse out the URL (or path requested) of an HTTP request 
static int parse_url( zhttp_t *en, char *err, int errlen ) {
	int l = 0, len = 0;
	zhttpr_t *b = NULL; 
	zWalker set = {0};

	if ( ( l = strlen( en->path )) == 1 || !memchr( en->path, '?', l ) ) {
		en->url = NULL;
		return 1;
	}

	while ( strwalk( &set, en->path, "?&=" ) ) {
		unsigned char *t = (unsigned char *)&en->path[ set.pos ];
		if ( set.chr == '?' ) 
			continue;
		else if ( set.chr == '=' ) {
			if ( !( b = init_record() ) ) {
				snprintf( err, errlen, "Memory allocation failure at URL parser" );
				fatal_error( en, ZHTTP_OUT_OF_MEMORY );
				return 0;
			}
			b->field = zhttp_copystr( t, set.size - 1 ); 
		}
		else {
			if ( !b ) {
				snprintf( err, errlen, "Incomplete or incorrect query string specified in URL" );
				fatal_error( en, ZHTTP_INCOMPLETE_QUERYSTRING );
				return 0;
			}
			b->value = t; 
			b->size = ( set.chr == '&' ) ? set.size - 1 : set.size; 
			zhttp_add_item( &en->url, b, zhttpr_t *, &len );
			b = NULL;
		}
	}
	//headers_length = len; //TODO: some time in the future
	return 1;
}


//Parse out the headers of an HTTP body
static int parse_headers( zhttp_t *en, char *err, int errlen ) {
	zWalker set = {0};
	int len = 0;
	int flen = strlen( en->path ) 
						+ strlen( en->method ) 
						+ strlen( en->protocol ) + 4;
	unsigned char *rawheaders = &en->msg[ flen ];
	const unsigned char chars[] = "\r\n";

	while ( memwalk( &set, rawheaders, chars, en->hlen - flen, 2 ) ) {
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
				fatal_error( en, ZHTTP_OUT_OF_MEMORY );
				return 0;
			}
			b->field = zhttp_copystr( t, pos ); 
			b->value = ( t += pos + 2 ); 
			b->size = ( set.size - pos - (( set.chr == '\r' ) ? 3 : 2 ) ); 
			zhttp_add_item( &en->headers, b, zhttpr_t *, &len );
		}
	}
	return 1;
}


//Parse out the parts of an HTTP body
static int parse_body( zhttp_t *en, char *err, int errlen ) {
	//Always process the body 
	zWalker set;
	memset( &set, 0, sizeof( zWalker ) );
	int len = 0;
	unsigned char *p = &en->msg[ en->hlen + 4 ];
	const char idem[] = "POST,PUT,PATCH,DELETE";
	const char multipart[] = "multipart/form-data";
	const char x_www_form[] = "application/x-www-form-urlencoded"; 
	const char reject[] = "&=+[]{}*";

	//TODO: If this is a xfer-encoding chunked msg, en->clen needs to get filled in when done.
	//TODO: Bitmasking is 1% more efficient, go for it.

	//Return early on methods that should have no body 
	if ( !memstr( idem, en->method, strlen(idem) ) ) {
		//set_http_procstatus( en );
		en->body = NULL;
		//ADDITEM( NULL, zhttpr_t, en->body, len, NULL );
		return 0;
	}

	//Check the content-type and die if it's wrong
	if ( !en->ctype ) { 
		//set_error( "No content type received." );
		return 0;
	}

	//url encoded is a little bit different.  no real reason to use the same code...
	if ( strcmp( en->ctype, x_www_form ) == 0 ) {
		if ( memchr( reject, *p, strlen( reject ) ) ) {
			fatal_error( en, ZHTTP_MALFORMED_FORMDATA );	
			return 0;
		}
		
		for ( zhttpr_t *b = NULL; memwalk( &set, p, (unsigned char *)"=&", en->clen, 2 ); ) {
			unsigned char *m = &p[ set.pos ];  
			//TODO: Should be checking that allocation was successful
			if ( set.chr == '=' )
				b = init_record(), b->field = zhttp_copystr( m, set.size - 1 );
			else { 
				if ( !b )
					break;
				else {	
					b->value = m;
					b->size = set.size - (( set.chr == '&' ) ? 1 : 2);
					zhttp_add_item( &en->body, b, zhttpr_t *, &len );
					b = NULL;
				}
			}
		}
	}
	else if ( memcmp( multipart, en->ctype, strlen(multipart) ) == 0 ) {
		char bd[ 128 ];
		memset( &bd, 0, sizeof( bd ) );
		snprintf( bd, 64, "--%s", en->boundary );
		const int bdlen = strlen( bd );
		int len1 = en->clen, pp = 0;

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
				zhttp_add_item( &en->body, b, zhttpr_t *, &len );
				b = NULL;
			}
			++pp, len1 -= pp, p += pp;	
		}
	}
	else {
		zhttpr_t *b = init_record(); 
		b->field = zhttp_dupstr( "body" );
		b->value = p;
		b->size = en->clen;
		zhttp_add_item( &en->body, b, zhttpr_t *, &len ); 
	}
	return 1;
}



//Marks the important parts of an HTTP request
static int parse_http_header ( zhttp_t *en, char *err, int errlen ) {
	//Define stuffs
	char *port = NULL;
	int walker = 0;
	zWalker z = {0};

	//Set pointers to zero?
	en->headers = en->body = en->url = NULL;

	//Walk through the first line
	while ( memwalk( &z, en->msg, (unsigned char *)" \r\n", en->mlen, 3 ) ) {
		char **ptr = NULL;
		if ( z.chr == ' ' && memchr( "HGPD", en->msg[ z.pos ], 4 ) )
			ptr = &en->method;
		else if ( z.chr == ' '  && '/' == en->msg[ z.pos ] )
			ptr = &en->path;
		else if ( z.chr == '\r' && en->protocol )
			break;	
		else if ( z.chr == '\r' )
			ptr = &en->protocol;
		else if ( z.chr == '\n' )
			break;
		else {
			//TODO: Solve the possible leak that can result here
			snprintf( err, errlen, "Malformed first line of HTTP request." );
			fatal_error( en, ZHTTP_MALFORMED_FIRSTLINE );
			return 0;
		}	

		if ( walker++ < 3 ) {  
			*ptr = malloc( z.size );
			memset( *ptr, 0, z.size );
			memcpy( *ptr, &en->msg[ z.pos ], z.size - 1 ); 
		}
	}

	//Return null if method, path or version are not present
	//ZHTTP_PRINTF( stderr, "%p %p %p\n", en->method, en->path, en->protocol ); 
	if ( !en->method || !en->path || !en->protocol ) {
		snprintf( err, errlen, "Method, path or HTTP protocol are not present." );
		nonfatal_error( en, ZHTTP_MALFORMED_FIRSTLINE );
		return 0;
	}

	//Fatal 
	if ( !memstr( zhttp_supported_protocols, en->protocol, sizeof( zhttp_supported_protocols ) ) ) {
		snprintf( err, errlen, "HTTP protocol '%s' not supported.", en->protocol );
		fatal_error( en, ZHTTP_UNSUPPORTED_PROTOCOL );
		return 0;
	}

	//Fatal
	if ( !memstr( zhttp_supported_methods, en->method, sizeof( zhttp_supported_methods ) ) ) {
		snprintf( err, errlen, "HTTP method '%s' not supported", en->method );
		fatal_error( en, ZHTTP_UNSUPPORTED_METHOD );
		return 0;
	}

	//Next, set header length
	if ( ( en->hlen = memblkat( en->msg, "\r\n\r\n", en->mlen, 4 ) ) == -1 ) {
		snprintf( err, errlen, "Header not completely sent." );
		nonfatal_error( en, ZHTTP_INCOMPLETE_HEADER );
		return 0;
	}

	//Get host requested (not always going to be there)
	en->host = zhttp_msg_get_value( "Host: ", "\r", en->msg, en->hlen );	
	if ( !en->host ) {
		en->host = zhttp_msg_get_value( "host: ", "\r", en->msg, en->hlen );	
	}	
	
	if ( en->host && ( port = strchr( en->host, ':' ) ) ) {
		//Remove colon
		en->port = atoi( port + 1 ); //TODO: zhttp_satoi( port + 1, en->port ); 
		memset( port, 0, strlen( port ) ); 
	}

	//If we expect a body, parse it
	if ( memstr( "POST,PUT,PATCH,DELETE", en->method, strlen( "POST,PUT,PATCH,DELETE" ) ) ) {
		//Get content-length if we didn't already get it
		if ( !en->clen ) {
			int clen;	
			char *clenbuf = NULL;

			if ( !( clenbuf = zhttp_msg_get_value( "Content-Length: ", "\r", en->msg, en->hlen ) ) ) {
				snprintf( err, errlen, "Content-Length header not present..." );
				fatal_error( en, ZHTTP_INCOMPLETE_HEADER );
				return 0;
			}

			if ( !zhttp_satoi( clenbuf, &clen ) ) {
				snprintf( err, errlen, "Content-Length doesn't appear to be a number." );
				fatal_error( en, ZHTTP_INCOMPLETE_HEADER );
				return 0;
			}

			en->clen = clen;
			free( clenbuf );
		}

		//Try as hard as possible to get a content type and throw INCOMPLETE_HEADER if it's not there...
		if ( !en->ctype ) {
			const char *ts[] = { "Content-Type: ", "content-type: ", "Content-type: ", NULL };
			for ( const char **ctypestr = ts; *ctypestr; ctypestr++ ) {
				if ( ( en->ctype = zhttp_msg_get_value( *ctypestr, ";\r", en->msg, en->hlen ) ) ) {
					break;
				}
			}
			if ( !en->ctype ) {
				snprintf( err, errlen, "No Content-Type specified." );
				nonfatal_error( en, ZHTTP_INCOMPLETE_HEADER );
				return 0;
			}
		}

		//Get boundary if we didn't already get it
		if ( !*en->boundary ) {
			char *b = NULL;
			//This is a pretty ugly way to do this; but until I move over to all static allocations, this will have to do.
			if ( ( b = zhttp_msg_get_value( "boundary=", "\r", en->msg, en->hlen ) ) ) {
				memcpy( en->boundary, b, strlen( b ) );
				free( b );
			}
		}
	}
	return 1;	
}


//
int http_parse_preamble( zhttp_t *en, unsigned char *p, int plen ) {
	zWalker z;
	memset( &z, 0, sizeof( zWalker ) ); 

	//Walk through the first line
	for ( int w = 0; memwalk( &z, p, (unsigned char *)" \r\n", plen, 3 );  ) {
		char **ptr = NULL;
		if ( z.chr == ' ' && memchr( "HGPD", p[ z.pos ], 4 ) )
			ptr = &en->method;
		else if ( z.chr == ' '  && '/' == p[ z.pos ] )
			ptr = &en->path;
		else if ( z.chr == '\r' && en->protocol )
			break;	
		else if ( z.chr == '\r' )
			ptr = &en->protocol;
		else if ( z.chr == '\n' )
			break;
		else {
			//TODO: Solve the possible leak that can result here
			//snprintf( err, errlen, "Malformed first line of HTTP request." );
			fatal_error( en, ZHTTP_MALFORMED_FIRSTLINE );
			return 0;
		}	

		if ( w++ < 3 ) {  
			*ptr = malloc( z.size );
			memset( *ptr, 0, z.size );
			memcpy( *ptr, &p[ z.pos ], z.size - 1 ); 
		}
	}

	//Return null if method, path or version are not present
	if ( !en->method || !en->path || !en->protocol ) {
		fatal_error( en, ZHTTP_MALFORMED_FIRSTLINE );
		return 0;
	}

	if ( !memstr( zhttp_supported_protocols, en->protocol, sizeof( zhttp_supported_protocols ) ) ) {
		fatal_error( en, ZHTTP_UNSUPPORTED_PROTOCOL );
		return 0;
	}

	if ( !memstr( zhttp_supported_methods, en->method, sizeof( zhttp_supported_methods ) ) ) {
		fatal_error( en, ZHTTP_UNSUPPORTED_METHOD );
		return 0;
	}

	return 1;
}



int http_parse_port ( zhttp_t *en, unsigned char *p, int plen ) {
	//Get host requested (not always going to be there)
	if ( ( en->host = zhttp_msg_get_value( "Host: ", "\r", p, plen ) ) ) {
		char *port = NULL; 
		if ( ( port = strchr( en->host, ':' ) ) ) {
			//Remove colon
			en->port = atoi( port + 1 ); //TODO: zhttp_satoi( port + 1, en->port ); 
			memset( port, 0, strlen( port ) ); 
		}
	}

	return 1;
}




//Process the query string
int http_process_query_string ( zhttp_t *en ) {
	//Process the query string if there is one...
	if ( strlen( en->path ) == 1 || !memchr( en->path, '?', strlen( en->path ) ) )
		return 1;	
	else {
		zhttpr_t *b = NULL; 
		zWalker set = {0};
		for ( int l, len = 0; strwalk( &set, en->path, "?&=" );  ) {
			unsigned char *t = (unsigned char *)&en->path[ set.pos ];
			if ( set.chr == '?' ) 
				continue;
			else if ( set.chr == '=' ) {
				if ( !( b = init_record() ) ) {
					fatal_error( en, ZHTTP_OUT_OF_MEMORY );
					return 0;
				}
				b->field = zhttp_copystr( t, set.size - 1 ); 
			}
			else {
				if ( !b ) {
					fatal_error( en, ZHTTP_INCOMPLETE_QUERYSTRING );
					return 0;
				}
				b->value = t; 
				b->size = ( set.chr == '&' ) ? set.size - 1 : set.size; 
				zhttp_add_item( &en->url, b, zhttpr_t *, &len );
				b = NULL;
			}
		}
	}
	return 1;
}


//
int http_frame_content ( zhttp_t *en, unsigned char *p, int plen ) {
	//If we expect a body, parse it
	if ( memstr( zhttp_idempotent_methods, en->method, sizeof( zhttp_idempotent_methods ) ) ) {
		en->idempotent = 1;
		//Get content-length if we didn't already get it
		if ( !en->clen ) {
			int clen;
			char *clenbuf = NULL;

			if ( !( clenbuf = zhttp_msg_get_value( "Content-Length: ", "\r", p, plen ) ) ) {
				fatal_error( en, ZHTTP_INCOMPLETE_HEADER );
				return 0;
			}

			if ( !zhttp_satoi( clenbuf, &clen ) ) {
				fatal_error( en, ZHTTP_INCOMPLETE_HEADER );
				return 0;
			}

			en->clen = clen, free( clenbuf );
		}

		//Try as hard as possible to get a content type and throw INCOMPLETE_HEADER if it's not there...
		if ( !en->ctype ) {
			const char *ts[] = { "Content-Type: ", "content-type: ", "Content-type: ", NULL };
			for ( const char **ctypestr = ts; *ctypestr; ctypestr++ ) {
				if ( ( en->ctype = zhttp_msg_get_value( *ctypestr, ";\r", p, plen ) ) ) {
					break;
				}
			}
			if ( !en->ctype ) {
				//snprintf( err, errlen, "No Content-Type specified." );
				nonfatal_error( en, ZHTTP_INCOMPLETE_HEADER );
				return 0;
			}
			if ( !strcmp( zhttp_multipart, en->ctype ) )
				en->formtype = ZHTTP_MULTIPART;
			else if ( !strcmp( zhttp_x_www_form, en->ctype ) )
				en->formtype = ZHTTP_URL_ENCODED;
			else {
				en->formtype = ZHTTP_OTHER;
			}	
		}

		//Get boundary if we didn't already get it
		if ( !*en->boundary ) {
			char *b = NULL;
			//This is a pretty ugly way to do this; but until I move over to all static allocations, this will have to do.
			if ( ( b = zhttp_msg_get_value( "boundary=", "\r", p, plen ) ) ) {
				memcpy( en->boundary, b, strlen( b ) ), free( b );
			}
		}
	}

	return 1;
}
#endif

//Return if the header is fully received
int http_header_received ( unsigned char *p, int size ) {
	return memblkat( p, "\r\n\r\n", size, 4  ); 
}

#if 0
int http_header_xreceived ( zhttp_t *en ) {
	return ( ( en->hlen = memblkat( en->msg, "\r\n\r\n", en->mlen, 4  ) ) > -1 );  
}
#endif


char * http_get_method ( zhttp_t *en, unsigned char **p, int *plen ) {
	//unsigned char * word = memchr( *p, "H", len );s
	char *m = (char *)*p;
	unsigned char *w = *p;

	//Mark with a \0
	for ( ; ( *w != ' ' && *plen > 1 ) || ( *w = '\0' ) ; w++, (*p)++, (*plen)-- );
	(*p)++;

	//Check for whether or not it's supported
	for ( const char **method = zhttp_supported_methods2; *method; method++ ) {
		if ( strcmp( *method, m ) == 0 ) {
			for ( const char **imethod = zhttp_idempotent_methods2; *imethod; imethod++ ) {
				if ( strcmp( *imethod, m ) == 0 ) {
					en->idempotent = 1;	
					break; 
				}
			}
			return en->method = m;
		}
	}

	return NULL;
}


char * http_get_path ( zhttp_t *en, unsigned char **p, int *plen ) {
	char *m = (char *)*p;
	unsigned char *w = *p;
	//Mark with a \0
	for ( ; ( *w != ' ' && *plen > 1 ) || ( *w = '\0' ) ; w++, (*p)++, (*plen)-- ) {
		( *w == '?' ) ? en->expectsURL = 1 : 0;
	}
	(*p)++;
	
	return en->path = m;
}


char * http_get_protocol ( zhttp_t *en, unsigned char **p, int *plen ) {
	char *m = (char *)*p;
	unsigned char *w = *p;
	int supported = 0;

	//Mark with a \0
	for ( ; ( *w != '\r' && *plen > 0 ) || ( *w = '\0' ) ; w++, (*p)++, (*plen)-- );
	(*p)++;
	
	//Check for whether or not it's supported
	for ( const char **protocol = zhttp_supported_protocols2; *protocol; protocol++ ) {
		if ( strcmp( *protocol, m ) == 0 ) {
			return en->protocol = m;	
		}
	}

	return NULL;
}


int http_get_content_length ( zhttpr_t **list ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		for ( const char **id = zhttp_content_length_id; *id; id++ ) { 
			if ( strcmp( (*slist)->field, *id ) == 0 ) {
				char clenbuf[64];
				int clen;

				memset( clenbuf, 0, sizeof( clenbuf ) ), memcpy( clenbuf, (*slist)->value, (*slist)->size );

				if ( zhttp_satoi( clenbuf, &clen ) )
					return ( clen < 0 ) ? -1 : clen;
				else {
					return -1;
				}
			}
		}
	}
	return 0; 
}


char * http_get_content_type ( zhttpr_t **list, HttpContentType *type ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		for ( const char **id = zhttp_content_type_id; *id; id++ ) { 
			if ( strcmp( (*slist)->field, *id ) == 0 ) {
				//This is a long-winded way to nul-terminate a block 
				for ( unsigned char *v = (*slist)->value; ( *v != '\r' && *v != ';' ) || ( *v = '\0' ); v++ );
				char *ctype = (char *)(*slist)->value;
				if ( strcmp( ctype, zhttp_multipart ) == 0 )
					*type = ZHTTP_MULTIPART;
				else if ( strcmp( ctype, zhttp_url_encoded ) == 0 )
					*type = ZHTTP_URL_ENCODED;
				else {
					*type = ZHTTP_OTHER;
				}
				return ctype;
			}
		}
	}

	//This technically can be any content type...
	return (char *)default_content_type;
}


char * http_get_boundary ( zhttpr_t **list ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		if ( strcmp( (*slist)->field, "Content-Type" ) == 0 ) {
			int s = (*slist)->size;
			for ( unsigned char *v = (*slist)->value; ( *v != '\r' && s > 1 ) || ( *v = '\0' ); v++, s-- );
			char *c = memstr( (*slist)->value, "boundary=", (*slist)->size );
			return c += strlen( "boundary=" );
		}
	}
	return NULL;
}


char * http_get_host ( zhttp_t *en, zhttpr_t **list, int *p ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		if ( strcmp( (*slist)->field, "Host" ) == 0 ) {
			int s = (*slist)->size;
			unsigned char *v = (*slist)->value; 
			for ( ; *v != '\r' && *v != ':' ; v++, s-- ) ;
			if ( *v == '\r' ) {
				*v = '\0', v++;
				return (char *)(*slist)->value;
			}
			else {
				int port = 0;
				char pb[ 64 ];
				*v = '\0', v++, s--;
				memset( pb, 0, sizeof( pb ) ), memcpy( pb, v, s );
				if ( !zhttp_satoi( pb, &port ) || port < 1 || port > 65535 ) {
					*p = -1;
					return NULL;
				}
				*p = port;
				return (char *)(*slist)->value;
			}
		}
	}
	return NULL;
}



zhttpr_t ** http_get_query_strings ( char *p, int plen, short *err ) {
	zhttpr_t **list = NULL;

	//Process the query string if there is one...
	if ( strlen( p ) == 1 || !memchr( p, '?', plen ) )
		return NULL;
	else {
		zhttpr_t *b = NULL; 
		zWalker z = {0};
		for ( int l, len = 0; strwalk( &z, p, "?&=" );  ) {
			unsigned char *t = (unsigned char *)&p[ z.pos ];
			if ( z.chr == '?' ) 
				continue;
			else if ( z.chr == '=' ) {
				if ( !( b = init_record() ) ) {
					*err = ZHTTP_OUT_OF_MEMORY;
					return NULL;
				}
				b->field = zhttp_copystr( t, z.size - 1 ); 
			}
			else {
				if ( !b ) {
					*err = ZHTTP_INCOMPLETE_QUERYSTRING;
					return NULL;
				}
				b->value = t; 
				b->size = ( z.chr == '&' ) ? z.size - 1 : z.size; 
				zhttp_add_item( &list, b, zhttpr_t *, &len );
				b = NULL;
			}
		}
	}
	return list;
}

//Store each of the headers
zhttpr_t ** http_get_header_keyvalues ( unsigned char **p, int *plen, short *err ) {
	int len = 0;
	//int flen = strlen( en->path ) + strlen( en->method ) + strlen( en->protocol ) + 4;
	const unsigned char chars[] = "\r\n";
	unsigned char *rawheaders = *p; //&p[ flen ];
	zhttpr_t **list = NULL;

	zWalker z; 
	memset( &z, 0, sizeof( zWalker ) );

	for ( ; memwalk( &z, rawheaders, chars, *plen, 2 ) ; ) {
		unsigned char *t = &rawheaders[ z.pos ];
		zhttpr_t *b = NULL;
		int pos = -1;
		
		//Character is useless, skip it	
		if ( *t == '\n' ) {
			continue;
		}
	
		//Copy the header field and value
		if ( ( pos = memchrat( t, ':', z.size ) ) >= 0 ) { 
			//Make a record for the new header line	
			if ( !( b = init_record() ) ) {
				//fatal_error( en, ZHTTP_OUT_OF_MEMORY );
				*err = ZHTTP_OUT_OF_MEMORY;
				return NULL;
			}
			b->field = zhttp_copystr( t, pos ); 
			
			//We need to trim the values on the other side
		#if 1
			t += ++pos;
			int plen = 0; 
			//unsigned char *a = httptrim( t, "\r\t ", z.size - pos, &plen );  
			b->value = httptrim( t, "\r\t ", z.size - pos, &plen );
			b->size = plen;
		#else
			b->value = ( t += pos + 2 ); 
			b->size = ( z.size - pos - (( z.chr == '\r' ) ? 3 : 2 ) ); 
		#endif
			zhttp_add_item( &list, b, zhttpr_t *, &len );
		}
	}

	//return en->headers = list;
	return list;
}


//Set the chunked "bit" if this is that kind of message...
int http_check_for_chunked_encoding ( zhttp_t *en, zhttpr_t **list ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		if ( strcmp( (*slist)->field, "Transfer-Encoding" ) == 0 ) {
			return en->chunked = 1;
		}
	}
	return 1;
}


#if 0
int http_get_cookie ( zhttp_t *en, zhttpr_t **list ) {
	return 0;
}
#endif

//Just parse the headers
zhttp_t * http_parse_header ( zhttp_t *en, int plen ) {
#if 0
	//Check if the full headers were received	
	if ( !en->hlen && !http_header_received( en ) ) {
		return nonfatal_error( en, ZHTTP_AWAITING_HEADER );	
	}
#endif

	//Define & initialize everything else here...
	unsigned char *p = en->preamble;
	//int plen = en->hlen;
	en->headers = en->body = en->url = NULL;

	if ( plen < 1 )
		return fatal_error( en, ZHTTP_HEADER_LENGTH_UNSET );
#if 1
	if ( !http_get_method( en, &p, &plen ) )
		return fatal_error( en, ZHTTP_UNSUPPORTED_METHOD );

	if ( !http_get_path( en, &p, &plen ) )
		return fatal_error( en, ZHTTP_BAD_PATH );

	//TODO: Handle blank vs invalid?
	if ( !http_get_protocol( en, &p, &plen ) )
		return fatal_error( en, ZHTTP_UNSUPPORTED_PROTOCOL );
#else
	if ( !( en->method = http_get_method( &p, &plen, &en->idempotent ) ) )
		return fatal_error( en, ZHTTP_UNSUPPORTED_METHOD );

	if ( !( en->path = http_get_path( &p, &plen, &en->expectsURL ) ) )
		return fatal_error( en, ZHTTP_BAD_PATH );

	//TODO: Handle blank vs invalid?
	if ( !( en->protocol = http_get_protocol( &p, &plen ) ) )
		return fatal_error( en, ZHTTP_UNSUPPORTED_PROTOCOL );
#endif

#if 1
	if ( !( en->headers = http_get_header_keyvalues( &p, &plen, &(en->error) ) ) )
		return fatal_error( en, ZHTTP_MALFORMED_HEADERS );
#else
	if ( !( en->headers = http_get_header_keyvalues( &p, &plen ) ) )
		return fatal_error( en, ZHTTP_MALFORMED_HEADERS );
#endif

#if 1
	if ( en->expectsURL && !( en->url = http_get_query_strings( en->path, strlen( en->path ), &(en->error) ) ) )
		return fatal_error( en, en->error );
#else
	if ( en->expectsURL && !( en->url = http_get_query_strings( en->path, strlen( en->path ) ) ) )
		return fatal_error( en, en->error );
#endif

	if ( !http_check_for_chunked_encoding( en, en->headers ) )
		0; //return fatal_error( en, ZHTTP_INVALID_PORT );

	if ( !( en->host = http_get_host( en, en->headers, &en->port ) ) && en->port == -1 )
		return fatal_error( en, ZHTTP_INVALID_PORT );

	if ( !( en->ctype = http_get_content_type( en->headers, &en->formtype ) ) )
		return fatal_error( en, ZHTTP_UNSPECIFIED_CONTENT_TYPE );

	if ( en->idempotent && ( en->clen = http_get_content_length( en->headers ) ) < 0 )
		return fatal_error( en, ZHTTP_INVALID_CONTENT_LENGTH );

	if ( en->formtype == ZHTTP_MULTIPART && !( en->boundary = http_get_boundary( en->headers ) ) )
		return fatal_error( en, ZHTTP_UNSPECIFIED_MULTIPART_BOUNDARY );

	return en;
}



//Parse standard application/x-www-url-form data
zhttpr_t ** http_parse_standard_form ( unsigned char *p, int clen ) {

	zhttpr_t **list = NULL;
	zWalker z;
	int len = 0;
	const char reject[] = "&=+[]{}*";
	memset( &z, 0, sizeof( zWalker ) );

	//Block messages that are not very likely to be actual forms...
	if ( memchr( reject, *p, strlen( reject ) ) ) {
		return NULL;
	}
	
	//Walk through and serialize what you can
	//TODO: Handle de-encoding from here?	
	for ( zhttpr_t *b = NULL; memwalk( &z, p, (unsigned char *)"=&", clen, 2 ); ) {
		unsigned char *m = &p[ z.pos ];  
		//TODO: Should be checking that allocation was successful
		if ( z.chr == '=' )
			b = init_record(), b->field = zhttp_copystr( m, z.size - 1 );
	#if 0
		else if ( z.chr == '+' )
			*m = ' ';
		//Percent encoding will drop
		else if ( z.chr == '%' ) {

		}
	#endif
		else { 
			if ( !b )
				break;
			else {
				//You'll have blank spaces, but this is probably ok...
				unsigned char *c = m;
				int osize, a, d; 
				osize = a = d = z.size - (( z.chr == '&' ) ? 1 : 2);

				//Die when a hits 0, because we're done...
				for ( unsigned char *n = m; a; d--, c++ ) {
					if ( *n == '+' )
						*n = ' ' ;
					else if ( *n == '%' && d > 3 ) {
						*c = *n = zhttp_url_decode( n, 3 ), n += 3, a -= 3;
						continue;
					}
					*c = *n, n++, a--;
				}
		
				//Wipe any excess buffer after translation
				( d ) ? memset( c, 0, d ) : 0;
				b->value = m, b->size = osize - d;
				zhttp_add_item( &list, b, zhttpr_t *, &len );
				b = NULL;
			}
		}
	}

	return list;
}



//Parse multipart/form-data
zhttpr_t ** http_parse_multipart_form ( unsigned char *p, int clen, char *bnd ) {
	//Prepare the boundary
	int blen, len = 0, len1 = clen;
	char boundary[ 128 ];
	zhttpr_t **list = NULL;

	//If a boundary does not exist, throw back
	if ( !bnd ) {
		return NULL;
	}

	//Do any initialization
	memset( &boundary, 0, sizeof( boundary ) );
	snprintf( boundary, 64, "--%s", bnd );
	blen = strlen( boundary );

	for ( int cpos, len2, pp = 0; ( len2 = pp = memblkat( p, boundary, len1, blen ) ) > -1; ) {
		if ( len2 > 0 ) {	
			unsigned char *i = p;
			i += blen + 2, len2 -= blen;

			//Find the content within the multipart block
			if ( ( cpos = memblkat( i, "\r\n\r\n", len2, 4 ) ) > -1 ) {
				zhttpr_t *b = init_record();
				b->type = ZHTTP_MULTIPART;
				unsigned char *h = i;
				int hsize = cpos;
				cpos += 4, i += cpos, len2 -= cpos + 4;

				//Set the body first
				b->value = i, b->size = len2;

				//Then set the field name and preamble, etc
				b->ctype = zhttp_msg_get_value( "Content-Type: ", "\r", h, hsize );
				b->disposition = zhttp_msg_get_value( "Content-Disposition: ", ";", h, hsize );
				b->field = zhttp_msg_get_value( "name=\"", "\"", h, hsize );
				if ( memblkat( h, "filename=", hsize, 9 ) > -1 ) {
					b->filename = zhttp_msg_get_value( "filename=\"", "\"", h, hsize );
				}

				//Add it to the body set if we found a valid key
				if ( !b->field ) 
					free( b );	
				else {
					zhttp_add_item( &list, b, zhttpr_t *, &len );
					b = NULL;
				}
			}
		}
		++pp, len1 -= pp, p += pp;	
	}

	return list;
}



//Parse (or at least, store) serializable bodies like XML, JSON and MsgPack
//Can also just store a reference to a large file...
zhttpr_t ** http_parse_freeform_body ( unsigned char *p, int clen ) {
	zhttpr_t *b, **list = NULL;
	int len = 0;

	if ( !( b = init_record() ) )
		return NULL;
	else {
		b->field = zhttp_dupstr( "body" );
		b->value = p;
		b->size = clen;
		zhttp_add_item( &list, b, zhttpr_t *, &len ); 
	}
	return list;
}



//This is intended to serialize chunked encodings as part of a body.
int http_parse_chunked_encoding_part ( zhttp_t *en, unsigned char *p ) {
	//NOTE: This is basically what we were doing before with realloc...
	return 1;
}


const char * print_formtype ( int x ) {
	const char *w[] = { "multipart", "url_enc", "other", "none" };
  switch ( x ) {
    case ZHTTP_MULTIPART:
      return w[0];
    case ZHTTP_URL_ENCODED:
      return w[1];
    case ZHTTP_OTHER:
      return w[2];
    default:
      return w[3];
  }
  return NULL;
}


//Handle different types of content
zhttp_t * http_parse_content( zhttp_t *en, unsigned char *p, int plen ) {
	
	//Block any blank entries	
	if ( !en || !p )
		return fatal_error( en, ZHTTP_PROGRAMMER_ERROR ); 

	//Block zero-length, odd content length or non-idempotent methods
	if ( plen < 1 || en->formtype == ZHTTP_NO_CONTENT )
		return en;
	
	//Final sanity check, return on shady things or clearly incorrect data
	if ( plen < 3 && ( en->formtype == ZHTTP_URL_ENCODED || en->formtype == ZHTTP_MULTIPART ) )
		return fatal_error( en, ZHTTP_MALFORMED_FORMDATA );		

	if ( en->formtype == ZHTTP_URL_ENCODED && !( en->body = http_parse_standard_form( p, plen ) ) )
		return fatal_error( en, ZHTTP_MALFORMED_FORMDATA );		
	else if ( en->formtype == ZHTTP_MULTIPART && !( en->body = http_parse_multipart_form( p, plen, en->boundary ) ) )
		return fatal_error( en, ZHTTP_MALFORMED_FORMDATA );
	else if ( en->formtype == ZHTTP_OTHER && !( en->body = http_parse_freeform_body( p, plen ) ) ) {
		return fatal_error( en, ZHTTP_MALFORMED_FORMDATA );
	}

	return en;
}



//Parse an HTTP request
zhttp_t * http_parse_request ( zhttp_t *en, char *err, int errlen ) {

	//Set error to none
	en->error = ZHTTP_NONE;

#if 1
	#if 0
	int len = 0;
	//Need to know how big message is, but you'll need this
	if ( ( len = http_header_received( NULL, 0 ) ) > -1 ) {
		return NULL;
	}

	//Use an independent reference to the message
	//http_parse_header( en, hp, plen );
	if ( !http_parse_header( en, len ) ) {
		return NULL;
	}

	//Same here
	//http_parse_content( en, cp, plen );
	if ( !http_parse_content( en ) ) {
		return NULL;
	}
	#endif

#else
	//Parse the header
	if ( !parse_http_header( en, err, errlen ) ) {
		return en;
	}

	ZHTTP_PRINTF( stderr, "Calling parse_url( ... )\n" );
	if ( !parse_url( en, err, errlen ) ) {
		return en;
	}

	ZHTTP_PRINTF( stderr, "Calling parse_headers( ... )\n" );
	if ( !parse_headers( en, err, errlen ) ) {
		return en;
	}

	ZHTTP_PRINTF( stderr, "Calling parse_body( ... )\n" );
	if ( !parse_body( en, err, errlen ) ) {
		return en;
	}
#endif
	//ZHTTP_PRINTF( "Dump http body." );
	//print_httpbody_to_file( en, "/tmp/zhttp-01" );
	return en;
} 


#if 0
//Parse an HTTP response
zhttp_t * http_parse_response ( zhttp_t *en, char *err, int errlen ) {
	//Prepare the rest of the request
	int hdLen = memstrat( en->msg, "\r\n\r\n", en->mlen );

	//Initialize the remainder of variables 
	en->headers = en->body = en->url = NULL;
	en->hlen = hdLen; 
	en->host = zhttp_msg_get_value( "Host: ", "\r", en->msg, hdLen );
	return NULL;
} 
#endif

#if 0
static int http_get_expected_header_length ( zhttp_t *en ) {
	return -1;
}

static int http_get_expected_content_length ( zhttp_t *en, HttpContentType rtype ) {
	int length = 0;
	zhttpr_t **body = en->body;

	//Check for the proper method
	for ( const char **method = zhttp_supported_methods2; *method; method++ ) {
		if ( strcmp( *method, m ) == 0 ) {
			for ( const char **imethod = zhttp_idempotent_methods2; *imethod; imethod++ ) {
				if ( strcmp( *imethod, m ) == 0 ) {
					en->idempotent = 1;	
					break; 
				}
			}
		}
	}

	//Return a negative content length in this case
	if ( !en->idempotent ) {
		return -1;
	}

	if ( !body || !( *body ) ) {
		return -1;
	}

	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	if ( rtype == ZHTTP_OTHER )
		length = (*body)->size;
	else if ( rtype == ZHTTP_URL_ENCODED ) {
		for ( ; body && *body; body++ ) {
			( *en->body != *body ) ? length ++ : 0;
			length += strlen( (*body)->field ) + 1 + (*body)->size;
		}
	}
	else {
		//Handle multipart requests
		int blen = strlen( en->boundary );
		//Each block will contain a combination of the following characters
		int clen = sizeof( "\r\n\"\r\n\r\n\r\n" ) + sizeof( cdisph ) + sizeof( cdispt ) + sizeof( nameh ) - 1;
		for ( ; body && *body; body++ ) {
			length += blen + clen + strlen( r->field ) + r->size;
		}
		//Account for the trailing final boundary which includes 2 dashes
		length += blen + 2;
	}
	return length;
}
#endif


//Finalize an HTTP request (really just returns a unsigned char, but this can handle it)
zhttp_t * http_finalize_request ( zhttp_t *en, char *err, int errlen ) {
	unsigned char *msg = NULL, *hmsg = NULL;
	HttpContentType rtype = ZHTTP_OTHER;
	en->atype = ZHTTP_MESSAGE_MALLOC;
	en->type = ZHTTP_IS_CLIENT;

	char clen[ 32 ] = {0};
	en->clen = 0, en->mlen = 0, en->hlen = 0;

	if ( !en->protocol ) {
		en->protocol = "HTTP/1.1";
	}

	if ( !en->path ) {
		snprintf( err, errlen, "%s", "No path specified with request." );
		return NULL;
	}

	if ( !en->method ) {
		snprintf( err, errlen, "%s", "No method specified with request." );
		return NULL;
	}

	if ( !strcmp( en->method, "POST" ) || !strcmp( en->method, "PUT" ) ) {
		if ( !en->body && !en->ctype ) {
			snprintf( err, errlen, "Content-Type not specified for %s request.", en->method );
			return NULL;
		}

		if ( !strcmp( en->ctype, zhttp_url_encoded ) ) 
			rtype = ZHTTP_URL_ENCODED;	
		else if ( !strcmp( en->ctype, zhttp_multipart ) ) {
			rtype = ZHTTP_MULTIPART;	
			char *b = zhttp_rand_chars( 32 );
			memcpy( en->boundary, b, strlen( b ) );
			memset( en->boundary, '-', 6 );
			free( b );
		}
	}

	//You can get the content-length without generating first
	//#1 - Loop through and just add sizes
	//#2 - Modify the content-length as you add body parts
#if 0
	int hlen = http_get_expected_header_length( en, rtype );
	int clen = http_get_expected_content_length( en, rtype );

	if ( en->idempotent && clen == -1 ) {
		snprintf( err, errlen, "Invalid length for idempotent request." );
		return NULL;
	}

	if ( ( clen + hlen + 4 ) < ZHTTP_PREAMBLE_SIZE )
		en->msg = en->preamble, en->atype = ZHTTP_MESSAGE_STATIC;
	else {
		en->atype = ZHTTP_MESSAGE_STATIC;
		if ( !( en->msg = malloc( clen + hlen + 4 ) ) || !memset( en->msg, 0, clen + hlen + 4 ) ) {
			snprintf( err, errlen, "Couldn't allocate space for request message." );
			return NULL;
		}
	}

	//Do all the message creation stuff here...	
	
#else

	//TODO: Catch each of these or use a static buffer and append ONE time per struct...
	if ( !strcmp( en->method, "POST" ) || !strcmp( en->method, "PUT" ) ) {
		//app/xwww is % encoded
		if ( rtype == ZHTTP_OTHER ) {
			if ( !en->body ) 
				en->clen = 0;	
			else {
				//Assumes JSON or a single file or something
				zhttpr_t **body = en->body;
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)(*body)->value, (*body)->size ); 
			}
		}
		else if ( rtype == ZHTTP_URL_ENCODED ) {
			for ( zhttpr_t **body = en->body; body && *body; body++ ) {
				zhttpr_t *r = *body;
				if ( *en->body != *body ) {
					zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"&", 1 );
				}
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)r->field, strlen( r->field ) ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"=", 1 ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)r->value, r->size ); 
			}
		}
		else {
			//Handle multipart requests
			for ( zhttpr_t **body = en->body; body && *body; body++ ) {
				zhttpr_t *r = *body;
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)en->boundary, strlen( en->boundary ) ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"\r\n", 2 ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)cdisph, sizeof( cdisph ) ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)cdispt, sizeof( cdispt ) ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)nameh, sizeof( nameh ) ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"\"", 1 );
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)r->field, strlen( r->field ) ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"\"", 1 );
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"\r\n\r\n", 4 ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)r->value, r->size ); 
				zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"\r\n", 2 ); 
			}
			zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)en->boundary, strlen( en->boundary ) ); 
			zhttp_append_to_uint8t( &msg, &en->clen, (unsigned char *)"--", 2 ); 
		}
	}

	//TODO: Do this in a better way
	snprintf( clen, sizeof( clen ), "%d", en->clen );
	int expbody = !strcmp( en->method, "POST" ) || !strcmp( en->method, "PUT" );
	struct t { const char *value, *fmt, reqd, add; } m[] = {
		{ en->method, "%s ", 1, 1 },
		{ en->path, "%s ", 1, 1 },
		{ en->protocol, "%s\r\n", 1, 1 },
		{ clen, "Content-Length: %s\r\n", expbody, en->clen > 0 ? 1 : 0  },
		{ en->ctype, "Content-Type: %s", expbody, 1 },
		{ ( rtype == ZHTTP_MULTIPART ) ? en->boundary : "", 
			( rtype == ZHTTP_MULTIPART ) ? ";boundary=\"%s\"\r\n" : "%s\r\n", 
			expbody, 1 },
		{ en->host, "Host: %s\r\n", 1, 1 },
	};
	
	//Now build the request header(s)
	for ( int i = 0; i < sizeof(m)/sizeof(struct t); i++ ) {
		if ( !m[i].reqd && !m[i].value )
			continue;
		else if ( m[i].reqd && !m[i].value ) {
			snprintf( err, errlen, "%s", "Failed to add HTTP metadata to request." );
			return NULL;
		}
		else if ( m[i].add ) {
			//Add whatever value
			unsigned char buf[ 1024 ] = { 0 };
			int len = snprintf( (char *)buf, sizeof(buf), m[i].fmt, m[i].value ); 
			zhttp_append_to_uint8t( &hmsg, &en->hlen, buf, len );
		}
	}

	//Add any other headers
	for ( zhttpr_t **headers = en->headers; headers && *headers; headers++ ) {
		zhttpr_t *r = *headers;
		zhttp_append_to_uint8t( &hmsg, &en->hlen, (unsigned char *)r->field, strlen( r->field ) ); 
		zhttp_append_to_uint8t( &hmsg, &en->hlen, (unsigned char *)": ", 2 ); 
		zhttp_append_to_uint8t( &hmsg, &en->hlen, (unsigned char *)r->value, r->size ); 
		zhttp_append_to_uint8t( &hmsg, &en->hlen, (unsigned char *)"\r\n", 2 ); 
	}

	//Terminate the header
	zhttp_append_to_uint8t( &hmsg, &en->hlen, (unsigned char *)"\r\n", 2 );

	if ( !( en->msg = malloc( en->clen + en->hlen ) ) ) {
		snprintf( err, errlen, "%s", "Failed to reallocate message buffer." );
		return NULL;
	}

	memcpy( &en->msg[0], hmsg, en->hlen ), en->mlen += en->hlen; 
	memcpy( &en->msg[en->mlen], msg, en->clen ), en->mlen += en->clen; 
	free( msg ), free( hmsg );
#endif
	return en;
}


//Finalize an HTTP response (really just returns a unsigned char, but this can handle it)
zhttp_t * http_finalize_response ( zhttp_t *en, char *err, int errlen ) {
	unsigned char *msg = NULL;
	int msglen = 0;
	int http_header_len = 0;
	zhttpr_t **headers = en->headers;
	zhttpr_t **body = en->body;
	char http_header_buf[ 2048 ] = { 0 };
	char http_header_fmt[] = "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n";

	if ( !en->headers && !en->body && !en->fd ) {
		snprintf( err, errlen, "%s", "No headers or body specified with response." );
		return NULL;
	}

	if ( !en->ctype ) {
		snprintf( err, errlen, "%s", "No Content-Type specified with response." );
		return NULL;
	}

	if ( !en->status ) {
		snprintf( err, errlen, "%s", "No status specified with response." );
		return NULL;
	}

	if ( body && *body && ( !(*body)->value || !(*body)->size ) ) {
		snprintf( err, errlen, "%s", "No body length specified with response." );
		return NULL;
	}

	//This assumes (perhaps wrongly) that ctype is already set.
	en->clen = !en->clen ? (*en->body)->size : en->clen;
	//en->clen = (*en->body)->size;
	http_header_len = snprintf( http_header_buf, sizeof(http_header_buf) - 1, http_header_fmt,
		en->status, http_get_status_text( en->status ), en->ctype, en->clen ); //((*en->body)->size );

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
#if 0
	if ( !msg ) {
		snprintf( err, errlen, "Failed to append all headers" );
		return NULL;
	}
#endif

	if ( !zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)"\r\n", 2 ) ) {
		snprintf( err, errlen, "%s", "Could not add header terminator to message." );
		return NULL;
	}

	if ( !en->fd && !zhttp_append_to_uint8t( &msg, &msglen, (*en->body)->value, (*en->body)->size ) ) {
		snprintf( err, errlen, "%s", "Could not add content to message." );
		return NULL;
	}

	en->msg = msg, en->mlen = msglen;
	return en;
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
 ( zhttp_t *en, zhttpr_t ***list, int type, const char *k, unsigned char *v, int vlen, int free ) {
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
	len = en->lengths[ type ];
	r->field = zhttp_dupstr( k ), r->size = vlen, r->value = v, r->free = free;
	zhttp_add_item( list, r, zhttpr_t *, &len );
	en->lengths[ type ] = len; //en->size = vlen;
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
void http_free_body ( zhttp_t *en ) {
	if ( en->type == ZHTTP_IS_SERVER ) {
		en->path ? free( en->path ) : 0;
		en->ctype ? free( en->ctype ) : 0;
		en->host ? free( en->host ) : 0;
		en->method ? free( en->method ) : 0;
		en->protocol ? free( en->protocol ) : 0;
	}

	http_free_records( en->headers );
	http_free_records( en->url );
	http_free_records( en->body );

	//Free big message buffer
	if ( en->atype == ZHTTP_MESSAGE_MALLOC )
		free( en->msg );
	else if ( en->atype == ZHTTP_MESSAGE_SENDFILE ) {
		// TODO: Switching to a union for all of this...
		( en->fd > 2 ) ? close( en->fd ) : 0;
	}
}


//...
int http_set_error ( zhttp_t *en, int status, char *message ) {
	char err[ 2048 ];
	memset( err, 0, sizeof( err ) );

	http_set_status( en, status );
	http_set_ctype( en, text_html );
	http_copy_content( en, (unsigned char *)message, strlen( message ) );

	if ( !http_finalize_response( en, err, sizeof(err) ) ) {
		ZHTTP_PRINTF( stderr, "FINALIZE FAILED!: %s", err );
		return 0;
	}

#if 0
	fprintf(stderr, "msg: " );
	ZHTTP_WRITE( 2, en->msg, en->mlen );
#endif
	return 0;
}


#ifdef DEBUG_H
//list out all rows in an HTTPRecord array
void print_httprecords ( zhttpr_t **r ) {
	if ( *r == NULL ) return;
	while ( *r ) {
		ZHTTP_PRINTF( stderr, "'%s' -> ", (*r)->field );
		//ZHTTP_PRINTF( "%s\n", (*r)->field );
		ZHTTP_WRITE( 2, "'", 1 );
		ZHTTP_WRITE( 2, (*r)->value, (*r)->size );
		ZHTTP_WRITE( 2, "'\n", 2 );
		r++;
	}
}


//list out everything in an HTTPBody
void print_httpbody_to_file ( zhttp_t *r, const char *path ) {
	FILE *fb = NULL;
	int fd = 0;

	if ( r == NULL || !path ) {
		return;
	}

	if ( strcmp( path, "/dev/stdout" ) )
		fb = stdout, fd = 1;
	else if ( strcmp( path, "/dev/stderr" ) )
		fb = stderr, fd = 2;
#ifndef _WIN32
	else {
		if ( ( fd = open( path, O_RDWR | O_CREAT | O_TRUNC, 0655 ) ) == -1 ) {
			fprintf( stderr, "[%s:%d] %s\n", __func__, __LINE__, strerror( errno ) );
			return;
		}

		if ( ( fb = fdopen( fd, "w" ) ) == NULL ) {
			fprintf( stderr, "[%s:%d] %s\n", __func__, __LINE__, strerror( errno ) );
			return;
		}
	}
#else
	else {
		//TODO: Handle Windows properly
		fb = stdout, fd = 1;
	}
#endif

	ZHTTP_PRINTF( fb, "r->mlen: '%d'\n", r->mlen );
	ZHTTP_PRINTF( fb, "r->clen: '%d'\n", r->clen );
	ZHTTP_PRINTF( fb, "r->hlen: '%d'\n", r->hlen );
	ZHTTP_PRINTF( fb, "r->status: '%d'\n", r->status );
	ZHTTP_PRINTF( fb, "r->ctype: '%s'\n", r->ctype );
	ZHTTP_PRINTF( fb, "r->method: '%s'\n", r->method );
	ZHTTP_PRINTF( fb, "r->path: '%s'\n", r->path );
	ZHTTP_PRINTF( fb, "r->protocol: '%s'\n", r->protocol );
	ZHTTP_PRINTF( fb, "r->host: '%s'\n", r->host );
	ZHTTP_PRINTF( fb, "r->boundary: '%s'\n", r->boundary );
	ZHTTP_PRINTF( fb, "r->port: %d\n", r->port );

	ZHTTP_PRINTF( fb, "r->idempotent: '%d'\n", r->idempotent );
	ZHTTP_PRINTF( fb, "r->chunked: '%d'\n", r->chunked );
	ZHTTP_PRINTF( fb, "r->svctype: '%s'\n", !r->type ? "client" : "server" );
	ZHTTP_PRINTF( fb, "r->formtype: '%s'\n", print_formtype( r->formtype ) );

	switch ( r->atype ) {
		case ZHTTP_MESSAGE_STATIC:
			ZHTTP_PRINTF( fb, "Message allocation type: '%s'\n", "static" );
			break;	
		case ZHTTP_MESSAGE_MALLOC:
			ZHTTP_PRINTF( fb, "Message allocation type: '%s'\n", "mallocd" );
			break;	
		case ZHTTP_MESSAGE_SENDFILE:
			ZHTTP_PRINTF( fb, "Message allocation type: '%s'\n", "sendfile" );
			break;	
		case ZHTTP_MESSAGE_OTHER:
			ZHTTP_PRINTF( fb, "Message allocation type: '%s'\n", "other" );
			break;	
	}

#if 0
	switch ( r->formtype ) {
		case ZHTTP_NO_CONTENT:
			ZHTTP_PRINTF( fb, "Form type: '%s'\n", "other" );
			break;	
		case ZHTTP_URL_ENCODED:
			ZHTTP_PRINTF( fb, "Form type: '%s'\n", "other" );
			break;	
		case ZHTTP_MULTIPART:
			ZHTTP_PRINTF( fb, "Form type: '%s'\n", "other" );
			break;	
		case ZHTTP_OTHER:
			ZHTTP_PRINTF( fb, "Form type: '%s'\n", "other" );
			break;	
	}
#endif

	//Print out headers and more
	const char *names[] = { "r->headers", "r->url", "r->body" };
	zhttpr_t **rr[] = { r->headers, r->url, r->body };
	for ( int i=0; i<sizeof(rr)/sizeof(zhttpr_t **); i++ ) {
		ZHTTP_PRINTF( fb, "%s: %p\n", names[i], rr[i] );
		if ( rr[i] ) {
			zhttpr_t **w = rr[i];
			while ( *w ) {
				ZHTTP_WRITE( fd, " '", 2 ); 
				ZHTTP_WRITE( fd, (*w)->field, strlen( (*w)->field ) );
				ZHTTP_WRITE( fd, "' -> '", 6 );
				ZHTTP_WRITE( fd, (*w)->value, (*w)->size );
				ZHTTP_WRITE( fd, "'\n", 2 );
				if ( (*w)->type == ZHTTP_MULTIPART ) {
					ZHTTP_PRINTF( fb, "  Content-Type: %s\n", (*w)->ctype );
					ZHTTP_PRINTF( fb, "  Filename: %s\n", (*w)->filename );
					ZHTTP_PRINTF( fb, "  Content-Disposition: %s\n", (*w)->disposition );
				}
				w++;
			}
		}
	}

#ifndef _WIN32
	if ( fd > 2 ) {
		fclose( fb );
		close( fd );
	}
#endif
}

#endif
