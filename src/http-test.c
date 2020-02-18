#include "http.h"
#include "http-test-data.c"

const char default_protocol[] = "HTTP/1.1";

/*data*/
struct HTTPRecord *text_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Ok</h2>", 11 } 
};

struct HTTPRecord *uint8_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, (uint8_t []){ AEON_THUMB }, AEON_THUMB_LEN } 
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

struct HTTPRecord *bodies[] = {
	&(struct HTTPRecord){ "field1", "text/html", (uint8_t *)"Lots+of+text", 12 },
	&(struct HTTPRecord){ "another_field", "text/plain", (uint8_t *)"dd323d9asdf", 11 },
	&(struct HTTPRecord){ "gunz", "text/plain", (uint8_t *)"Hello,+world!", 13 },
	&(struct HTTPRecord){ NULL }
};

struct HTTPRecord *bodies_bin[] = {
	&(struct HTTPRecord){ "field1", "text/html", (uint8_t *)"Lots+of+text", 12 },
	&(struct HTTPRecord){ "another_field", "text/plain", (uint8_t *)"dd323d9asdf", 11 },
	&(struct HTTPRecord){ "binary", "image/jpeg", (uint8_t *)aeon_thumb_favicon_jpg, AEON_THUMB_LEN },
	&(struct HTTPRecord){ NULL }
};

struct HTTPRecord xheaders[] = {
	{ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
	{ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
	{ "Accept", NULL, (uint8_t *)"*/*", 3 },
	{ NULL }
};

/*responses*/
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

/*requests*/
struct HTTPBody request_head = {
	.protocol = "HTTP/1.1",
	.method = "HEAD",
	.path = "/juice",
	.ctype = "text/html"
};

struct HTTPBody request_text_html_no_path = {
	.protocol = "HTTP/1.1",
	.method = "GET",
	.ctype = "text/html"
};

struct HTTPBody request_text_html = {
	.protocol = "HTTP/1.1",
	.method = "GET",
	.path = "/",
	.ctype = "text/html"
};

struct HTTPBody request_application_x_www_url_formencoded_no_headers = {
	.protocol = "HTTP/1.1",
	.method = "POST",
	.path = "/",
	.ctype = "text/html",
	.body = bodies
};

struct HTTPBody request_application_x_www_url_formencoded = {
	.protocol = "HTTP/1.1",
	.method = "POST",
	.path = "/useless-post",
	.ctype = "application/x-www-form-urlencoded",
	.headers = headers, 
	.body = bodies
};

struct HTTPBody request_multipart_text_no_content = { 
	.protocol = "HTTP/1.1",
	.method = "POST",
	.path = "/post",
	.ctype = "text/html"
};

struct HTTPBody request_multipart_text = { 
	.protocol = "HTTP/1.1",
	.method = "POST",
	.ctype = "multipart/form-data",
	.path = "/post",
	.headers = headers,
	.body = bodies
};

struct HTTPBody request_multipart_binary = { 
	.protocol = "HTTP/1.1",
	.method = "POST",
	.ctype = "multipart/form-data",
	.path = "/post",
	.body = bodies,
};

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

const uint8_t text_html_body[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html\r\n"
	"Content-Length: 11\r\n\r\n"
	"<h2>Ok!</h2>"
;

const uint8_t text_plain_body[] =
	"HTTP/1.1 200 OK\r\n"
	"\r\n"
	"\r\n\r\n"
	""
;

const uint8_t zero_length_body[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Length: 0\r\n\r\n"
;

const uint8_t statusless_body[] =
	"HTTP/1.1 OK\r\n"
	"Content-Length: 11\r\n\r\n"
	"<h2>Ok!</h2>"
;

const uint8_t eww_body[] =
	"OK\r\n"
	"Content-Type: text/html\r\n"
	"Content-Length: 11\r\n\r\n"
	"<h2>Ok!</h2>"
;

const uint8_t headerless_body[] =
	"HTTP/1.1 200 OK\r\n"
;

const uint8_t missing_host_body[] =
	"HTTP/1.1 200 OK\r\n"
	"\r\n"
	"\r\n\r\n"
	""
;

const uint8_t missing_content_length_body[] =
	"HTTP/1.1 200 OK\r\n"
	"Host: geeneus.net\r\n\r\n"
;

#if 0
const uint8_t multipart_text_body[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Length: 0\r\n\r\n"
;

const uint8_t image_jpeg_body[] =
	"HTTP/1.1 200 OK\r\n"
	"Content-Type: image/jpeg\r\n"
	"Content-Length: 11\r\n\r\n"
	"\r\n\r\n"
	""
;
#endif

struct HTTPBody *requests_received[] = {
	&(struct HTTPBody){ .msg = (uint8_t *)head_body,  .mlen = sizeof( head_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_1, .mlen = sizeof( get_body_1 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_2, .mlen = sizeof( get_body_2 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_3, .mlen = sizeof( get_body_3 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_4, .mlen = sizeof( get_body_4 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_5, .mlen = sizeof( get_body_5 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)get_body_6, .mlen = sizeof( get_body_6 ) },
	&(struct HTTPBody){ .msg = (uint8_t *)post_body_1,.mlen = sizeof( post_body_1 ) },
	&(struct HTTPBody){ .msg = (uint8_t []){ POST_BODY },.mlen = POST_BODY_LEN },
	&(struct HTTPBody){ .msg = (uint8_t *)put_body,   .mlen = sizeof( put_body ) },
	NULL
};

struct HTTPBody *requests_to_send[] = {
#if 0
	&request_head,	
	&request_text_html_no_path,	
	&request_text_html,	
	&request_application_x_www_url_formencoded_no_length,
	&request_application_x_www_url_formencoded,
#endif
	&request_multipart_text,	
#if 0
	&request_multipart_binary,	
	&request_multipart_text_no_content,	
#endif
	NULL
};

struct HTTPBody *responses_received[] = {
	&(struct HTTPBody){ .msg = (uint8_t *)text_html_body, .mlen = sizeof( text_html_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)image_jpeg_body, .mlen = sizeof( image_jpeg_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)text_plain_body, .mlen = sizeof( text_plain_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)zero_length_body, .mlen = sizeof( zero_length_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)headerless_body, .mlen = sizeof( headerless_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)missing_host_body, .mlen = sizeof( missing_host_body ) },
	&(struct HTTPBody){ .msg = (uint8_t *)missing_content_length_body, .mlen = sizeof( missing_content_length_body ) },
	NULL
};

struct HTTPBody *responses_to_send[] = {
	&response_missing_body,
	&response_small_body,
	&response_small_body_missing_params_1,
	&response_small_body_missing_params_2,
	&response_small_body_with_headers,
	&response_with_error_not_found,
	&response_with_error_internal_server_error,
	&response_with_invalid_error_code,
	//&response_binary_body,
	NULL
};


void print_test_request ( struct HTTPBody *entity ) {
	//All of the members should be filled out...
	fprintf( stderr, "[ " );
	//fprintf( stderr, "proto: %5s",  entity->protocol );
	fprintf( stderr, ", method: %4s",  entity->method );
	fprintf( stderr, ", clen: %-5d", entity->clen );
	fprintf( stderr, ", hlen: %-3d", entity->hlen );
	fprintf( stderr, ", path: %s",   entity->path );
	fprintf( stderr, ", host: %s", entity->host );
	if ( entity->method && strcmp( entity->method, "POST" ) == 0 ) {
		fprintf( stderr, ", ctype: %s", entity->ctype );
		fprintf( stderr, ", boundary: %s", entity->boundary );
	}
	fprintf( stderr, " ]\n" );
}


void print_test_response ( struct HTTPBody *entity ) {
	write( 2, entity->msg, entity->mlen );
}


int main ( int argc, char *argv[] ) {
	//struct HTTPBody **r = NULL; 
	char err[ 2048 ] = { 0 };

	struct test { 
		const char *name;
		struct HTTPBody **list; 
		struct HTTPBody * (*transform)( struct HTTPBody *, char *, int );
		void (*print)( struct HTTPBody * );
		void (*free)( struct HTTPBody * );
	} tests[] = {
		//{ "REQUESTS - PARSE", requests_received, http_parse_request, print_test_request, http_free_body },
		{ "REQUESTS - PACK",  requests_to_send,  http_finalize_request, print_test_request, NULL },
#if 0
		{ "REPSONSES- PARSE", responses_received, http_parse_response, print_test_response, http_free_body },
		{ "RESPONSES- PACK", responses_to_send,  http_finalize_response, print_test_response, NULL },
#endif
	};
	

	//TODO: This wants to be a loop so bad...
	//This will have to check certain fields in the HTTPBody structure
	for ( int i = 0; i < sizeof(tests)/sizeof(struct test); i++ ) {
		struct HTTPBody **r = tests[i].list;
		fprintf( stderr, "%s\n===================\n", tests[i].name );
		while ( *r ) {
			memset( err, 0, sizeof(err) );	
			if ( !tests[i].transform( *r, err, sizeof(err) ) ) {
				fprintf( stderr, "FAILED - %s\n", err );
				r++;
				continue;
			}

			fprintf( stderr, "SUCCESS - " );
			if ( tests[i].print ) {
				tests[i].print( *r );
			}

#if 0
			if ( tests[i].free ) {
				tests[i].free( *r );
			}
#endif
			r++;
		}
	}

	//Now, just have to request pack and response parse... Could be done in same loop
	return 0;
}
