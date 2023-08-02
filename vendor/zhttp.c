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



//Generate random characters
static char *zhttp_rand_chars ( int len ) {
	const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	char * a = malloc( len + 1 );
	memset( a, 0, len + 1 );
	for ( int i = 0; i < len; i ++ ) {
		a[ i ] = chars[ rand() % sizeof(chars) ];
	}
	return a;	
}



//Add to series
static void * zhttp_add_item_to_list
	( void ***list, void *element, int size, int * len ) {
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



//Duplicate a block 
unsigned char * zhttp_dupblk( const unsigned char *v, int vlen ) {
	unsigned char * vv = malloc( vlen );
	memset( vv, 0, vlen );
	memcpy( vv, v, vlen );
	return vv;
}


static const char zhttp_safe[] =
	"0123456789"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"-:_~"
;

// Encode a string
char *zhttp_url_encode ( unsigned char *p, int len ) {
	int total = 0;
	char *m = NULL;
	static const char rbase[] = "0123456789ABCDEF";

	// Return null if p is blank or len is 0
	if ( !p || !len ) {
		return NULL;
	}

	// Get a count of how long the string SHOULD be
	int i = len;
	for ( unsigned char *d = p ; i; d++, i-- ) {
		total += ( !memchr( zhttp_safe, *d, sizeof( zhttp_safe ) ) ) ? 3 : 1;
	}

	// TODO: Catch this cleanly.
	if ( !( m = malloc( total + 1 ) ) || memset( m, 0, total + 1 ) ) {
		// snprintf( err, "%s\n", strerror( errno ) );
		return NULL;
	}

	// Copy to the new string buffer
	for ( unsigned char *d = p, *x = (unsigned char *)m; len; d++, len-- ) {
		// Only encode these characters
		if ( !memchr( zhttp_safe, *d, sizeof( zhttp_safe ) ) ) {
			char c = rbase[ ( *d - ( *d % 16 ) ) / 16 ];
			char e = rbase[ *d % 16 ];
			// can print to string buffer that makes sense...
			*x = '%', *( x + 1 ) = c, *( x + 2 ) = e;
			x += 3;
		}
		else {
			*x = *d;
			x++;
		}
	}

	return m;
}



// Decode a string
unsigned char *zhttp_url_decode ( char *p, int len, int *nlen ) {
	size_t nl = 0;
	unsigned char *m = NULL;
	static const unsigned char base[] = {
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
	,	['A'] = 10
	,	['B'] = 11
	,	['C'] = 12
	,	['D'] = 13
	,	['E'] = 14
	,	['F'] = 15
	};

	// Catch failures
	if ( !( m = malloc( len ) ) || !memset( m, 0, len ) ) {
		// snprintf( err, "%s\n", strerror( errno ) );
		return NULL;
	}

	for ( unsigned char *x = m; len ; ) {
		if ( *p == '%' && len >= 3 ) {
			unsigned char b = 0, c = *(p + 1), d = *(p + 2);

			// Do some quick base16 encoding
			if ( ( c == '0' || base[ (int)c ] ) && ( d == '0' || base[ (int)d ] ) ) {
				b += base[ (int)c ] * 16;
				b += base[ (int)d ];
			}
			*x = b;
			p += 3, len -= 3;
		}
		else {
			// Any other character (w/ the exception of '+')
			*x = ( *p == '+' ) ? ' ' : *p;
			p++, len--;
		}

		x++, nl++;
	}

	// TODO: Reallocate because the new string will likely be smaller
	if ( nl < len ) {
		m = realloc( m, nl );
	}
	*nlen = nl;
	return m;
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



// Return the status text associated with a particular code.
const char *http_get_status_text ( HTTP_Status status ) {
	//TODO: This should error out if HTTP_Status is not received...
	if ( status < 100 || status > sizeof( http_status ) / sizeof( char * ) ) {
		return http_status[ 200 ];	
	}
	return http_status[ status ];	
}


 
// Trim characters in [trim] from both sides of a unsigned char array
static unsigned char *httptrim (unsigned char *msg, const char *trim, int len, int *nlen) {
	//Define stuff
	unsigned char *m = msg;
	int nl = len;
	int tl = strlen( trim );

	//Move forwards and backwards to find whitespace...
	while ( memchr( trim, *(m + ( nl - 1 )), 4 ) && nl-- ) ; 
	while ( memchr( trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}



// Initialize a new ZHTTP record
static zhttpr_t * init_record() {
	zhttpr_t *record = NULL;
	int size = sizeof( zhttpr_t );
	if ( !( record = malloc( size ) ) || !memset( record, 0, size ) ) {
		return NULL;
	}
	record->type = ZHTTP_NO_CONTENT;
	return record;
}



// Return true if the header is fully received
int http_header_received ( unsigned char *p, int size ) {
	return memblkat( p, "\r\n\r\n", size, 4  ); 
}



// Return the method in use by either the request or response
static char * http_get_method ( zhttp_t *en, unsigned char **p, int *plen ) {
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



// Return the requested path
static char * http_get_path ( zhttp_t *en, unsigned char **p, int *plen ) {
	char *m = (char *)*p;
	unsigned char *w = *p;
	//Mark with a \0
	for ( ; ( *w != ' ' && *plen > 1 ) || ( *w = '\0' ) ; w++, (*p)++, (*plen)-- ) {
		( *w == '?' ) ? en->expectsURL = 1 : 0;
	}
	(*p)++;
	
	return en->path = m;
}



// Return the protocol in use
static char * http_get_protocol ( zhttp_t *en, unsigned char **p, int *plen ) {
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



// Return the content length of the response or request
static int http_get_content_length ( zhttpr_t **list ) {
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


#if 0
char a[] =
	"multipart/form-data; boundary=----WebKitFormBoundaryAvA33EhHy5kGaWO\r\n\r\n";
char b[] =
	"application/json; charset=utf-8\r\n\r\n";
char c[] = 
	"application/octet-stream\r\n\r\n";
char d[] = 
	"something/weird; boundary=----WebKitFormBoundaryAvA33EhHy5kGaWO; charset=utf-8\r\n\r\n";
#endif

// Return the content type of the response or request
static char * http_get_content_type ( zhttp_t *en, zhttpr_t **list, HttpContentType *type ) {

	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		for ( const char **id = zhttp_content_type_id; *id; id++ ) { 
			if ( strcmp( (*slist)->field, *id ) == 0 ) {
				int a = 0; 
				int s = (*slist)->size;
				char *ctype = (char *)(*slist)->value;
				unsigned char *v = (*slist)->value;

fprintf( stderr, "Original:\n" );
write( 2, (*slist)->value, (*slist)->size );

				// Set and initialize the most important structures
				en->ctype = ctype;
				en->boundary = NULL;
				en->charset = NULL;

				// Nul-terminate the content type header
				// TODO: Works, but is a bit gnarly for most to look at...
				for ( ; ( *v != '\r' && *v != ';' ) || ( *v = '\0' ); v++, a++, s-- );

				if ( memcmp( ctype, zhttp_multipart, strlen( zhttp_multipart ) ) == 0 )
					*type = ZHTTP_MULTIPART;
				else if ( memcmp( ctype, zhttp_url_encoded, strlen( zhttp_url_encoded ) ) == 0 )
					*type = ZHTTP_URL_ENCODED;
				else {
					*type = ZHTTP_OTHER;
				}

				// Trim any white space 
				for ( ; ( *v == ' ' ); v++, a++, s-- );

				// Set boundary and charset if any
				for ( ; ( *v != '\r' ) || ( *v = '\0' ) ; v++, s-- ) {
					//If you see a '=', check for either boundary or the other
					if ( s > 9 && memcmp( v, "boundary=", 9 ) == 0 )
						en->boundary = (char *)v + 9;
					else if ( s > 8 && memcmp( v, "charset=", 8 ) == 0 ) {
						en->charset = (char *)v + 8;
					}

					//Check for these things
					if ( *v == ';' ) {
						*v = '\0';
					}
				}

			#if 1
				fprintf( stderr, "CTYPE: %s\n", en->ctype );
				fprintf( stderr, "BOUNDARY: %s\n", en->boundary );
				fprintf( stderr, "CHARSET: %s\n", en->charset );
			#endif

				return ctype;
			}
		}
	}

	//This technically can be any content type...
	return (char *)default_content_type;
}




// Get the host specified in the request
static char * http_get_host ( zhttp_t *en, zhttpr_t **list, int *p ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		const char *f = (*slist)->field;
		if ( strcmp( f, "Host" ) == 0 || strcmp( f, "host" ) == 0 ) {
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



// Get and store all of the query strings
static zhttpr_t ** http_get_query_strings ( char *p, int plen, short *err ) {
	zhttpr_t **list = NULL;
	int len = 0;
	int at = 0;
	int start = 0;
	zWalker z = {0};

	//Process the query string if there is one...
	if ( strlen( p ) == 1 || ( at = memchrat( p, '?', plen ) ) == -1 ) {
		return NULL;
	}

	// Move the string forward since I only expect one ?
	p += ( at + 1 ), plen -= ( at + 1 );

#if 0
fprintf( stderr, "QUERY STRING RECVD: " );
write( 2, "'", 1 );write( 2, p, plen );write( 2, "'\n", 2 );
#endif

	// Walk through and serialize
	for ( zhttpr_t *b = NULL; memwalk( &z, (unsigned char *)p, (unsigned char *)"=&", plen, 2 ); ) {
		unsigned char *m = (unsigned char *)&p[ z.pos ];

		if ( z.chr == '=' ) {
			// TODO: Add a memory check
			b = init_record();
			b->field = zhttp_copystr( m, z.size - 1 );
			// TODO: I left this out for now.  Need to check if it automatically frees...
			//b->type = ZHTTP_URL_ENCODED;
		}
		else {
			if ( !b ) {
				*err = ZHTTP_INCOMPLETE_QUERYSTRING;
				return NULL;
			}
			b->value = zhttp_url_decode( (char *)m, z.size - 1, &b->size );
			b->free = 1;
			zhttp_add_item( &list, b, zhttpr_t *, &len );
			b = NULL;
		}
	}

	return list;
}



// Set the chunked "bit" if this is that kind of message...
static int http_check_for_chunked_encoding ( zhttp_t *en, zhttpr_t **list ) {
	for ( zhttpr_t **slist = list; slist && *slist; slist++ ) {
		if ( strcmp( (*slist)->field, "Transfer-Encoding" ) == 0 ) {
			return en->chunked = 1;
		}
	}
	return 1;
}



// Get and store each of the headers
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
			pos++;
			t += pos;
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

	return list;
}



// Parse the response or request headers
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

	if ( !( en->ctype = http_get_content_type( en, en->headers, &en->formtype ) ) )
		return fatal_error( en, ZHTTP_UNSPECIFIED_CONTENT_TYPE );

	if ( en->idempotent && ( en->clen = http_get_content_length( en->headers ) ) < 0 )
		return fatal_error( en, ZHTTP_INVALID_CONTENT_LENGTH );

	if ( en->formtype == ZHTTP_MULTIPART && !en->boundary )
		return fatal_error( en, ZHTTP_UNSPECIFIED_MULTIPART_BOUNDARY );

	return en;
}



//Parse standard application/x-www-url-form data
zhttpr_t ** http_parse_standard_form ( unsigned char *p, int clen ) {

	zhttpr_t **list = NULL;
	zWalker z = {0};
	int len = 0;
	const char reject[] = "&=+[]{}*";

	//Block messages that are not very likely to be actual forms...
	if ( memchr( reject, *p, strlen( reject ) ) ) {
		return NULL;
	}

	//Walk through and serialize what you can
	//TODO: Handle de-encoding from here?	
	for ( zhttpr_t *b = NULL; memwalk( &z, p, (unsigned char *)"=&", clen, 2 ); ) {
		unsigned char *m = &p[ z.pos ];
		//TODO: Should be checking that allocation was successful
		if ( z.chr == '=' ) {
			// TODO: Add a memory check
			b = init_record();
			b->field = zhttp_copystr( m, z.size - 1 );
			b->type = ZHTTP_URL_ENCODED;
		}
	#if 0
		else if ( z.chr == '+' )
			*m = ' ';
		//Percent encoding will drop
		else if ( z.chr == '%' ) {

		}
	#endif
		else { 
			if ( b ) {
				//You'll have blank spaces, but this is probably ok...
				b->value = zhttp_url_decode( (char *)m, z.size - 1, &b->size );
				b->free = 1;
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



// Store reference to a serializable bodies (e.g. XML, JSON and MsgPack)
zhttpr_t ** http_parse_freeform_body ( unsigned char *p, int clen ) {
	zhttpr_t *b, **list = NULL;
	int len = 0;

	if ( ( b = init_record() ) ) {
		b->field = zhttp_dupstr( "body" );
		b->value = p;
		b->size = clen;
		b->type = ZHTTP_OTHER;
		zhttp_add_item( &list, b, zhttpr_t *, &len ); 
		return list;
	}

	return list;
}



// Parses different types of content
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



// Finalize an HTTP request
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



// Finalize an HTTP response
zhttp_t * http_finalize_response ( zhttp_t *en, char *err, int errlen ) {
	unsigned char *msg = NULL;
	int msglen = 0;
	int http_header_len = 0;
	zhttpr_t **headers = en->headers;
	zhttpr_t **body = en->body;
	char http_header_buf[ 2048 ] = { 0 };
	const char http1_1close[] = "Connection: close\r\n";
	char http_header_fmt[] = 
		"HTTP/1.1 %d %s\r\n"
		"Content-Type: %s\r\n"
		"Content-Length: %d\r\n";
		"Connection: close\r\n";

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
	http_header_len = snprintf( http_header_buf, sizeof( http_header_buf ) - 1, http_header_fmt,
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
	//TODO: As other protocols are supported, this will change.  
	//For now, however, this has got to be it
	const char http1_1close[] = "Connection: close\r\n";
	if ( !zhttp_append_to_uint8t( &msg, &msglen, (unsigned char *)http1_1close, strlen( http1_1close ) ) ) {
		snprintf( err, errlen, "%s", "Could not add 'Connection: close' to message." );
		return NULL;
	}

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

	en->msg = msg;
	en->mlen = msglen;
	return en;
}



// Set any integer value in a zhttp_t structure
int http_set_int( int *k, int v ) {
	return ( *k = v );
}



// Set any character value in a zhttp_t structure
char * http_set_char( char **k, const char *v ) {
	return ( *k = zhttp_dupstr( v ) );	
}



// Allocate and setup a new zhttp record
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
	r->field = zhttp_dupstr( k );
	r->size = vlen;
	r->value = v;
	r->free = free;

	// NOTE: By default, use this when setting up a response... it's really not a useful
	// field outside of the context of a request.  Maybe come up with another way to do.
	r->type = ZHTTP_NO_CONTENT;
	zhttp_add_item( list, r, zhttpr_t *, &len );
	en->lengths[ type ] = len; //en->size = vlen;
	return r;
}



// Tear down list of records
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


// Tear down fully received request or created response
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
		( en->fd > 2 ) ? close( en->fd ) : 0;
		free( en->msg );
	}
}



// Set the error code and message in an HTTP response
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

	return 0;
}



#ifdef DEBUG_H
// Return the body form type 
static const char * print_formtype ( int x ) {
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



//list out all rows in an HTTPRecord array
void print_httprecords ( zhttpr_t **r ) {
	if ( !r || !( *r ) ) {
		return;
	}

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
void print_httpbody_to_file ( zhttp_t *rb, const char *path ) {
	FILE *fb = NULL;
	int fd = 0;

	if ( rb == NULL || !path ) {
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

	ZHTTP_PRINTF( fb, "rb->mlen: '%d'\n", rb->mlen );
	ZHTTP_PRINTF( fb, "rb->clen: '%d'\n", rb->clen );
	ZHTTP_PRINTF( fb, "rb->hlen: '%d'\n", rb->hlen );
	ZHTTP_PRINTF( fb, "rb->status: '%d'\n", rb->status );
	ZHTTP_PRINTF( fb, "rb->ctype: '%s'\n", rb->ctype );
	ZHTTP_PRINTF( fb, "rb->method: '%s'\n", rb->method );
	ZHTTP_PRINTF( fb, "rb->path: '%s'\n", rb->path );
	ZHTTP_PRINTF( fb, "rb->protocol: '%s'\n", rb->protocol );
	ZHTTP_PRINTF( fb, "rb->host: '%s'\n", rb->host );
	ZHTTP_PRINTF( fb, "rb->boundary: '%s'\n", rb->boundary );
	ZHTTP_PRINTF( fb, "rb->port: %d\n", rb->port );

	ZHTTP_PRINTF( fb, "rb->idempotent: '%d'\n", rb->idempotent );
	ZHTTP_PRINTF( fb, "rb->chunked: '%d'\n", rb->chunked );
	ZHTTP_PRINTF( fb, "rb->svctype: '%s'\n", !rb->type ? "client" : "server" );
	ZHTTP_PRINTF( fb, "rb->formtype: '%s'\n", print_formtype( rb->formtype ) );

	switch ( rb->atype ) {
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
	const char *names[] = { "rb->headers", "rb->url", "rb->body" };
	zhttpr_t **rr[] = { rb->headers, rb->url, rb->body };
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


#if 0
int main ( int argc, char *argv[] ) {
	const char *encoded[] = {
		"Ladies%20%2B%20Gentlemen",
		"An%20encoded%20string%21",
		"Dogs%2C%20Cats%20%26%20Mice",
		"%E2%98%83",
		NULL
	};

	const char *decoded[] = {
		"Ladies + Gentlemen",
		"An encoded string!",
		"Dogs, Cats & Mice",
		"☃" ,
		NULL
	};


	//
	struct test {
		const char *enc, *dec;
	} tests[] = {
		{ "Ladies%20%2B%20Gentlemen", "Ladies + Gentlemen" },
		{ "An%20encoded%20string%21", "An encoded string!" },
		{ "Dogs%2C%20Cats%20%26%20Mice", "Dogs, Cats & Mice" },
		{ "%E2%98%83", "☃" },
		{ NULL, NULL }
	};


	// Check decoding
	for ( struct test *t = tests; t->enc; t++ ) {
		int xlen = 0;
		unsigned char *x = zhttp_url_decode( (char *)t->enc, strlen( t->enc ), &xlen );
		write( 2, "'", 1 ); write( 2, x, xlen ); write( 2, "' -> ", 5 );
		fprintf( stderr, "%s\n", ( !memcmp( x, t->dec, xlen ) ) ? "PASS" : "FAIL" );
	}

	// Check encoding
	for ( struct test *t = tests; t->dec; t++ ) {
		char *x = zhttp_url_encode( (unsigned char *)t->dec, strlen( t->dec ) );
		fprintf( stderr, "'%s' -> ", x );
		fprintf( stderr, "%s\n", ( !memcmp( t->enc, x, strlen( t->enc ) ) ) ? "PASS" : "FAIL" );
	}

	return 0;
}
#endif
