/* ------------------------------------------- * 
 * zhttp.h
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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <zwalker.h>

#ifdef DEBUG_H
 #include <stdio.h>
 #define ZHTTP_PRINTF(...) \
	fprintf( stderr, __VA_ARGS__ )
 #define ZHTTP_WRITE(a,b) \
	write( 2, a, b )
#else
 #define ZHTTP_PRINTF(...)
 #define ZHTTP_WRITE(...)
#endif

#ifndef ZHTTP_H

#define ZHTTP_H

#define zhttp_add_item(LIST,ELEMENT,SIZE,LEN) \
	zhttp_add_item_to_list( (void ***)LIST, ELEMENT, sizeof( SIZE ), LEN )

#define http_free_request( entity ) \
	http_free_body( entity )

#define http_free_response( entity ) \
	http_free_body( entity )

#define http_set_status(ENTITY,VAL) \
	http_set_int( &(ENTITY)->status, VAL )

#define http_set_ctype(ENTITY,VAL) \
	http_set_char( &(ENTITY)->ctype, VAL )
	
#define http_set_method(ENTITY,VAL) \
	http_set_char( &(ENTITY)->method, VAL )
	
#define http_set_protocol(ENTITY,VAL) \
	http_set_char( &(ENTITY)->protocol, VAL )
	
#define http_set_path(ENTITY,VAL) \
	http_set_char( &(ENTITY)->path, VAL )
	
#define http_set_host(ENTITY,VAL) \
	http_set_char( &(ENTITY)->host, VAL )
	
#define http_set_content(ENTITY,VAL,VLEN) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, ".", VAL, VLEN, 1 )

#define http_copy_content(ENTITY,VAL,VLEN) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, ".", zhttp_dupblk((unsigned char *)VAL, VLEN), VLEN, 1 )

#define http_copy_tcontent(ENTITY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, ".", zhttp_dupstr(VAL), strlen(VAL), 1 )

#define http_set_header(ENTITY,KEY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->headers, 0, KEY, (unsigned char *)VAL, strlen(VAL), 0 )

#define http_copy_header(ENTITY,KEY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->headers, 0, KEY, (unsigned char *)zhttp_dupstr(VAL), strlen(VAL), 1 )

#define http_copy_theader(ENTITY,KEY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->headers, 0, KEY, zhttp_dupstr(VAL), strlen(VAL), 1 )

#define zhttp_dupstr(V) \
	(char *)zhttp_dupblk( (unsigned char *)V, strlen(V) + 1 )

extern const char http_200_fixed[];

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


typedef enum {
	ZHTTP_NONFATAL = 0x0000
, ZHTTP_FATAL = 0xff00
} HTTP_ErrorType;


typedef enum {
	ZHTTP_NONE = 0
,	ZHTTP_INCOMPLETE_METHOD
, ZHTTP_INCOMPLETE_PATH
, ZHTTP_INCOMPLETE_PROTOCOL
, ZHTTP_INCOMPLETE_HEADER
, ZHTTP_UNSUPPORTED_METHOD
, ZHTTP_UNSUPPORTED_PROTOCOL
, ZHTTP_MALFORMED_FIRSTLINE
, ZHTTP_INCOMPLETE_FIRSTLINE
, ZHTTP_OUT_OF_MEMORY
} HTTP_Error;


typedef enum {
	ZHTTP_URL_ENCODED = 0
, ZHTTP_MULTIPART
} HttpContentType;


struct HTTPRecord {
	HttpContentType type; //multipart or not?
	const char *field; 
	int size; 
#if 1
	unsigned char *value;
	const char *disposition;
	const char *filename;
	const char *ctype;
	char free;
#else
	union {
		unsigned char *uchars; 
		struct HTTPMultipartContent {
			const char *disposition;
			const char *filename;
			const char *type;
			unsigned char *data;
		} * mpc;
	} value;
#endif
};


struct HTTPBody {
	char *path;
	char *ctype; 
	char *host;
	char *method;
	char *protocol;
	char *boundary;
	int clen;  //content length
	int mlen;  //message length (length of the entire received message)
	int	hlen;  //header length
	int status; //what was this?
	int error;
 	unsigned char *msg;
	struct HTTPRecord **headers;
	struct HTTPRecord **url;
	struct HTTPRecord **body;
	//int rstatus;  //HEADER_PARSED, URL_PARSED, ...
};

unsigned char *httpvtrim (unsigned char *, int , int *) ;

unsigned char *httptrim (unsigned char *, const char *, int , int *) ;

void http_free_body( struct HTTPBody * );

struct HTTPBody * http_finalize_response (struct HTTPBody *, char *, int );

struct HTTPBody * http_finalize_request (struct HTTPBody *, char *, int );

struct HTTPBody * http_parse_request (struct HTTPBody *, char *, int );

struct HTTPBody * http_parse_response (struct HTTPBody *, char *, int );

int http_set_int( int *, int );

char *http_set_char( char **, const char * );

void *http_set_record( struct HTTPBody *, struct HTTPRecord ***, int, const char *, unsigned char *, int, int );

int http_set_error ( struct HTTPBody *entity, int status, char *message );

unsigned char * zhttp_dupblk( const unsigned char *v, int vlen ) ;

#ifdef DEBUG_H
 void print_httprecords ( struct HTTPRecord ** );
 void print_httpbody ( struct HTTPBody * );
#else
 #define print_httprecords(...)
 #define print_httpbody(...)
#endif
#endif
