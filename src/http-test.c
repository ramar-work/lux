#include "http.h"
#include "http-test-data.c"

const char default_protocol[] = "HTTP/1.1";

struct HTTPRecord *text_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Ok</h2>", 11 } 
};

struct HTTPRecord *uint8_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, aeon_thumb_favicon_jpg, 3848 } 
};

struct HTTPRecord *text_body_no_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, NULL, 11 } 
};

struct HTTPRecord *text_body_no_len[] = { 
	&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Ok</h2>" } 
};

struct HTTPRecord *headers[] = {
	&(struct HTTPRecord){ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
	&(struct HTTPRecord){ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
	&(struct HTTPRecord){ "Accept", NULL, (uint8_t *)"*/*", 3 },
	&(struct HTTPRecord){ NULL }
};

struct HTTPRecord xheaders[] = {
	{ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
	{ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
	{ "Accept", NULL, (uint8_t *)"*/*", 3 },
	{ NULL }
};

struct HTTPBody response_missing_body = {
	.clen = 12,
	.mlen = 16,
	.hlen = 16,
	.status = 200,
	.ctype = "text/html",
	.method = NULL,
	.protocol = "HTTP/1.1",
};

struct HTTPBody response_small_body = {
	.status = 200,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)text_body,
	.headers = NULL
};

struct HTTPBody response_small_body_missing_params_1 = {
	.status = 200,
	.ctype = "image/jpeg",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)text_body_no_body
};

struct HTTPBody response_small_body_missing_params_2 = {
	.status = 200,
	.ctype = "image/jpeg",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)text_body_no_len
};

struct HTTPBody response_small_body_with_headers = {
	.status = 200,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.headers = (struct HTTPRecord **)&headers,
	.body = (struct HTTPRecord **)&text_body,
};

struct HTTPBody response_with_error_not_found = {
	.status = 404,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = &( struct HTTPRecord * ){
		&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Resource not found</h2>", 28 }
	}
};

struct HTTPBody response_with_error_internal_server_error = {
	.status = 500,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = &( struct HTTPRecord * ){
		&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Internal Server Error</h2>", 31 }
	}
};

struct HTTPBody response_with_invalid_error_code = {
	.status = 909,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = &( struct HTTPRecord * ){
		&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Never</h2>", 14 }
	}
};

struct HTTPBody response_binary_body = {
	.status = 200,
	.ctype = "image/jpeg",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)uint8_body 
};

#if 0
//Build a response body like it would be in a real environment
struct HTTPBody *build_test_object ( int status, char *ctype, uint8_t *body, int bodylen, struct HTTPRecord **headers ) {
	
	//Allocate
	struct HTTPBody *object = NULL;
	if ( !( object = malloc( sizeof( struct HTTPRecord ) ) ) ) {
		fprintf( stderr, "Failed to allocate space for test object.\n" );
		exit( 0 );
		return NULL;
	}

	if ( body ) {
		struct HTTPRecord *b = NULL;
		if ( !( *object->body = b = malloc( bodylen ) ) || !memset( b, 0, bodylen ) ) {
			fprintf( stderr, "Failed to allocate space for test body object.\n" );
			exit( 0 );
			return NULL;
		}

		if ( !memcpy( b, body, bodylen ) ) {
			fprintf( stderr, "Failed to allocate space for test body object.\n" );
			exit( 0 );
			return NULL;
		}
	}

	if ( headers ) {
		int headerlen = 0;
		while ( *headers ) {
			//Allocate each new header item...
			//ADDITEM( 
			struct HTTPRecord *r = malloc( sizeof( struct HTTPRecord ) );
			//memcpy( r, *headers, sizeof( struct HTTPRecord ) );
			fprintf( stderr, "field   : %s\n", (*headers)->field ); 
			fprintf( stderr, "metadata: %s\n", (*headers)->metadata ); 
			fprintf( stderr, "value   : " );
			write( 2, (*headers)->value, (*headers)->size ); 
			fprintf( stderr, "\n" );
			headers++;
		}
	}

	object->status = status;
	object->ctype = ctype;
	object->protocol = (char *)default_protocol;
	return NULL;
}
#endif



//small HEAD 
//small GET
//big GET
//small POST
//big POST 
//binary POST 
//small PUT
//big PUT
//binary PUT 
//small DELETE 
//small OPTIONS 

const uint8_t head_body[] =
 "HEAD /gan HTTP/1.1\r\n"
 "Content-Type: text/html\r\n"
 "Text-trapper: Nannybot\r\n\r\n"
;

const uint8_t head_body_missing_protocol[] =
 "HEAD /gan\r\n"
 "Content-Type: text/html\r\n"
 "Text-trapper: Nannybot\r\n\r\n"
;

const uint8_t get_body_missing_path[] =
 "GET HTTP/1.1\r\n"
 "Content-Type: text/html\r\n"
 "Text-trapper: Nannybot\r\n\r\n"
;

const uint8_t get_body_missing_headers[] =
 "GET HTTP/1.1\r\n"
;

const uint8_t get_body_extra_long_path[] =
 "GET " HTTP_PATH_2048_CHARS " HTTP/1.1\r\n"
;

const uint8_t get_body_1[] =
 "GET / HTTP/1.1\r\n"
 "Content-Type: text/html\r\n"
 "Text-trapper: Nannybot\r\n\r\n"
;

const uint8_t get_body_2[] =
 "GET /j HTTP/1.1\r\n"
 "Content-Type: text/html\r\n"
 "Text-trapper: Nannybot\r\n\r\n"
;

const uint8_t get_body_3[] =
 "GET /etc/2001-06-07?index=full&get=all HTTP/1.1\r\n"
 "Content-Type:"" text/html\r\n"
 "Host: hellohellion.com:3496\r\n"
 "Upgrade:"     " HTTP/2.0, HTTPS/1.3, IRC/6.9, RTA/x11, websocket\r\n"
 "Text-trapper:"" Nannybot\r\n\r\n"
;

//Tests cookies and Hosts with colons
const uint8_t get_body_4[] =
 "GET /jax HTTP/1.1\r\n"
 "Content-Type: text/html\r\n"
 "Host: jimmycrackcorn.com:3496\r\n"
 "Set-Cookie: name=Nicholas; expires=Sat, 02 May 2009 23:38:25 GMT\r\n"
 "Text-trapper: Nannybot\r\n\r\n"
;

const uint8_t get_body_5[] =
 "GET /etc/2001-06-07?index=full&get=all&funder=3248274982j2kljjjoasdf HTTP/1.1\r\n"
 "Content-Type:"" text/html\r\n"
 "Upgrade:"     " HTTP/2.0, HTTPS/1.3, IRC/6.9, RTA/x11, websocket\r\n"
 "Text-trapper:"" Nannybot\r\n\r\n"
;

const uint8_t get_body_6[] =
 "GET /etc/2001-06-07?index=full&get=allxxxxxxxxxxxxxxxxx HTTP/1.1\r\n"
 "Content-Type:"" text/html\r\n"
 "Upgrade:"     " HTTP/2.0, HTTPS/1.3, IRC/6.9, RTA/x11, websocket\r\n"
 "Text-trapper:"" Nannybot\r\n\r\n"
;

const uint8_t post_body_1[] =
 "POST /chinchinchinny/washhouse/2011-01-20/index?get=html&version=12 HTTP/1.1\r\n"
 "Content-Type"     ": application/x-www-form-urlencoded\r\n"
 "TE"               ": trailers,Â deflate\r\n"
 "Upgrade"          ": HTTP/2.0, HTTPS/1.3, IRC/6.9, RTA/x11, websocket\r\n"
 //"User-Agent"       ": Mozilla/5.0 (X11; Linux x86_64; rv:12.0) Gecko/20100101 Firefox/21.0\r\n"
 "Via"              ": 1.0 fred, 1.1 example.com (Apache/1.1)\r\n"
 "Warning"          ": 199 Miscellaneous warning\r\n"
 "X-Requested-With" ": XMLHttpRequest\r\n"
 "Content-Length"   ": 72\r\n\r\n" 
 "FirstName=Bacon&LastName=Gordon&Age=67" 
 "&Formula=a+%2B+b+%3D%3D+13%25%21\r\n"
;

const uint8_t put_body[] =
 "PUT /chinchinchinny HTTP/1.1\r\n"
 "Content-Type: application/x-www-form-urlencoded\r\n"
 "Content-Length: 72\r\n\r\n" 
 "FirstName=Bacon&LastName=Gordon&Age=67" 
 "&Formula=a+%2B+b+%3D%3D+13%25%21\r\n"
;


struct HTTPBody *requests[] = {
	&(struct HTTPBody){ .msg = (uint8_t *)head_body,  .mlen = sizeof( head_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_1, .mlen = sizeof( get_body_1 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_2, .mlen = sizeof( get_body_2 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_3, .mlen = sizeof( get_body_3 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_4, .mlen = sizeof( get_body_4 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_5, .mlen = sizeof( get_body_5 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_6, .mlen = sizeof( get_body_6 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)post_body_1,.mlen = sizeof( post_body_1 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)post_body_2,.mlen = sizeof( post_body_2 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)put_body,   .mlen = sizeof( put_body ) },
};


struct HTTPBody *responses[] = {
	&response_missing_body,
	&response_small_body,
	&response_small_body_missing_params_1,
	&response_small_body_missing_params_2,
	&response_small_body_with_headers,
	&response_with_error_not_found,
	&response_with_error_internal_server_error,
	&response_with_invalid_error_code,
	&response_binary_body,
	NULL
};

#if 0
struct HTTPRecord ** build_header_structure( struct HTTPRecord **h ) {
	struct HTTPRecord **hh = headers;
	struct HTTPRecord **new = NULL;
	int newlen = 0;
	while ( (*hh)->field ) {
		struct HTTPRecord *k = malloc( sizeof( struct HTTPRecord ) );
		k->field = (*hh)->field;
		k->metadata = (*hh)->metadata;
		k->value = (*hh)->value;
		k->size = (*hh)->size;
		ADDITEM( k, struct HTTPRecord, new, newlen, NULL );
		hh++;
	} 
	ADDITEM( NULL, struct HTTPRecord, new, newlen, NULL );
	return new;
}
#endif


int main ( int argc, char *argv[] ) {
	//All this does is output text strings
	fprintf( stderr, "Request parsing:\n" );
	struct HTTPBody **req = requests;
	//const uint8_t **req = requests;
	while ( *req ) {
		char err[ 2048 ] = { 0 };
		//write( 2, (*req)->msg, 7 );	
#if 1
		struct HTTPBody *body = http_parse_request( *req, err, sizeof(err) );
		if ( !body ) {
			fprintf( stderr, "FAILED - %s\n", err );
			req++;
			continue;
		}

		fprintf( stderr, "SUCCESS - " );
		//All of the members should be filled out...
		//write( 2, body->msg, body->mlen );
#endif
		req++;
	}

#if 0
	fprintf( stderr, "Response packing:\n" );
	struct HTTPBody **res = responses;
	while ( *res ) {
		//fprintf( stderr, "%p\n", *res );
		struct HTTPBody *body = NULL;
		char err[ 2048 ] = { 0 };
		int m = 0;
		fprintf( stderr, "\n" );

		if ( !( body = http_finalize_response( *res, err, sizeof(err) ) ) ) {
			fprintf( stderr, "FAILED - %s", err );
			res++;
			continue;
		}

		fprintf( stderr, "SUCCESS - " );
		write( 2, body->msg, body->mlen );
		if ( m ) {
			//free( (*res)->headers );
		}
		res++;
	}
#endif
	return 0;
}
