#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "util.h"
#ifndef HTTP_H

#define HTTP_H

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
	
#define http_set_body(ENTITY,KEY,VAL,VLEN) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, KEY, VAL, VLEN )
	
#define http_set_content(ENTITY,VAL,VLEN) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, ".", VAL, VLEN )

#define http_set_content_text(ENTITY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, ".", (uint8_t *)VAL, strlen(VAL) )

#define http_set_textbody(ENTITY,KEY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->body, 1, KEY, (uint8_t *)VAL, strlen((char *)VAL) )
	
#define http_set_header(ENTITY,KEY,VAL) \
	http_set_record( ENTITY, &(ENTITY)->headers, 0, KEY, (uint8_t *)VAL, strlen(VAL) )

extern const char http_200_fixed[];

struct HTTPRecord {
	const char *field; 
	const char *metadata; 
	uint8_t *value; 
	int size; 
};

struct HTTPBody {
	char *ctype; //content type ptr
	char *method;
	char *path;
	char *protocol;
	char *host;
	char *boundary;
 	uint8_t *msg;
	int clen;  //content length
	int mlen;  //message length (length of the entire received message)
	int	hlen;  //header length
	int status; //what was this?
#if 0
	//This may make things more efficient
	struct HTTPRecord **values;
#else
	struct HTTPRecord **headers;
	struct HTTPRecord **url;
	struct HTTPRecord **body;
#endif
};

unsigned char *httpvtrim (uint8_t *, int , int *) ;
unsigned char *httptrim (uint8_t *, const char *, int , int *) ;
void print_httprecords ( struct HTTPRecord ** ) ;
void print_httpbody ( struct HTTPBody * ) ;
void http_free_body( struct HTTPBody * );
struct HTTPBody * http_finalize_response (struct HTTPBody *, char *, int );
struct HTTPBody * http_finalize_request (struct HTTPBody *, char *, int );
struct HTTPBody * http_parse_request (struct HTTPBody *, char *, int );
struct HTTPBody * http_parse_response (struct HTTPBody *, char *, int );
int http_set_int( int *, int );
char *http_set_char( char **, const char * );
void *http_set_record( struct HTTPBody *, struct HTTPRecord ***, int, const char *, uint8_t *, int );
int http_set_error ( struct HTTPBody *entity, int status, char *message );

#endif
