#include "../vendor/single.h"
#ifndef HTTP_H

#define HTTP_H

struct HTTPRecord {
	const char *field; 
	const char *metadata; 
	uint8_t *value; 
	int size; 
};


struct HTTPBody {
	int clen;  //content length
	int mlen;  //message length (length of the entire received message)
	int	hlen;  //header length
	int status; //what was this?
	char *ctype; //content type ptr
	char *method;
	char *protocol;
	char *path;
	char *host;
	char *boundary;
 	uint8_t *msg;
#if 1
	struct HTTPRecord **url;
	struct HTTPRecord **headers;
	struct HTTPRecord **body;
#endif
};


#define WRITE_HTTP_500( cstr, estr ) \
	char hbuf[ 2048 ]; \
	char cbuf[ 4096 ]; \
	memset( &hbuf, 0, sizeof( hbuf ) ); \
	memset( &cbuf, 0, sizeof( cbuf ) ); \
	int clen = snprintf( cbuf, sizeof( cbuf ) - 1, "%s: %s", cstr, estr ); \
	int hlen = snprintf( hbuf, sizeof( hbuf ) - 1, http_500_custom, clen ); \
	write( 2, hbuf, hlen ); \
	write( 2, cbuf, clen ); \
	write( fd, hbuf, hlen ); \
	write( fd, cbuf, clen ); \
	close( fd );


unsigned char *httpvtrim (uint8_t *msg, int len, int *nlen) ;
unsigned char *httptrim (uint8_t *msg, const char *trim, int len, int *nlen) ;
void print_httprecords ( struct HTTPRecord **r ) ;
void print_httpbody ( struct HTTPBody *r ) ;
#endif
