// http.h
#ifndef HTTP_H
#define HTTP_H
#include "single.h"
#include "nw.h"

// Definable settings
#ifndef HTTP_ERROR_BUFFER_LENGTH
 #define HTTP_ERROR_BUFFER_LENGTH 2048
#endif
#ifndef HTTP_HEADER_MAX
 #define HTTP_HEADER_MAX 25
#endif
#ifndef HTTP_BODY_MAX
 #define HTTP_BODY_MAX 25
#endif
#ifndef HTTP_URL_MAX
 #define HTTP_URL_MAX 2048
#endif
#ifndef HTTP_METHOD_MAX
 #define HTTP_METHOD_MAX 10
#endif
#ifndef HTTP_PROTO_MAX
 #define HTTP_PROTO_MAX 10
#endif
#ifndef HTTP_MAX_ELEMENTS
 #define HTTP_MAX_ELEMENTS 63 
#endif

//General error handler
#define http_err(h, st, ...) \
	((memset(h->error, 0, 2048) ? 1 : 1) && (snprintf(h->error, 2047, __VA_ARGS__) ? 1 : 1) && http_error_h(h, st))

#define http_append( h, blob, bs ) \
	bf_append( h->resb, (uint8_t *)blob, bs )

#define http_appends( h, str ) \
	bf_append( h->resb, (uint8_t *)str, strlen(str))

//Errors
//Data
typedef enum 
{
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



typedef struct HTTP_Request HTTP_Request;
typedef struct HTTP_Response HTTP_Response;
typedef struct HTTP HTTP;


struct HTTP_Request 
{
	int       clen,  //content length
					  mlen,  //message length (length of the entire received message)
					  hlen;  //header length
	char      method[HTTP_METHOD_MAX];  //one of 7 methods
	char      protocol[HTTP_PROTO_MAX]; //
	char      path[HTTP_URL_MAX];       //The requested path
	char      host[1024];               //safe bet for host length
	char      boundary[128];            //The boundary
#if 0
	char      *method;
	char      *protocol;
	char      *path;
	char      *host;
	char      *boundary;
#endif
 	uint8_t  *msg     ;
	Table     table;	
};



struct HTTP_Response 
{
	int 				 clen,         //Mirrors the first three in HTTP_Request
               mlen,  
               hlen;
	int          bypass;       //Choose whether or not to bypass packing
	float        version;
	HTTP_Status  status;
	const char  *sttext;
	const char  *ctype;
	uint8_t     *msg;
	//LiteKv       headers[HTTP_MAX_ELEMENTS];
  Table        headers;
}; 



struct HTTP 
{
	HTTP_Request   request;
	HTTP_Response  response;
	Buffer        *reqb;
	Buffer        *resb;
	void          *userdata;
	const char    *hashname;
	char          *hostname;
	char           hostname_buf[ 256 ];
	char           error[HTTP_ERROR_BUFFER_LENGTH];
};



//Print functions
void print_request (HTTP *) ;
void print_response (HTTP *) ;


//Stuff to process a message (eventually, this will all go to one function)
_Bool http_get_content_length (HTTP *, uint8_t *, int32_t) ;
_Bool http_get_header_length  (HTTP *, uint8_t *, int32_t) ;
_Bool http_get_message_length (HTTP *, uint8_t *, int32_t) ;
_Bool http_parse_first_line (HTTP *, uint8_t *, int) ;
_Bool http_get_boundary (HTTP *h, uint8_t *msg, int32_t len);
int http_get_remaining (HTTP *h, uint8_t *msg, int len) ;


_Bool http_set_content_type (HTTP *, const char *);
_Bool http_set_status (HTTP *, HTTP_Status);
_Bool http_set_header (HTTP *h, const char *key, const char *value);
_Bool http_set_version (HTTP *h, float version); 
_Bool http_set_content_length (HTTP *, int ); 
_Bool http_set_content (HTTP *, const char * /*mimetype*/, uint8_t *, int); 
#define http_set_content_str( h, mime, str ) \
	http_set_content (h, mime, (uint8_t *)str, strlen( str )); 

int http_pack_response (HTTP *);
int http_pack_request (HTTP *);

//Basic HTTP data message digestor functions
_Bool http_read (Recvr *r, void *p, char *e);
_Bool http_proc (Recvr *r, void *p, char *e);
_Bool http_test (Recvr *r, void *p, char *e);
_Bool http_fin (Recvr *r, void *p, char *e);
_Bool http_error_h (HTTP *h, HTTP_Status st) ;

void http_print_request (HTTP *h);
void http_print_response (HTTP *h);

#endif
