#include "http.h"
#include "config.h"
#include "filter-static.h"
#include "filter-echo.h"
#include "filter-lua.h"

struct HTTPRecord *headers[] = {
	&(struct HTTPRecord){ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
	&(struct HTTPRecord){ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
	&(struct HTTPRecord){ "Accept", NULL, (uint8_t *)"*/*", 3 },
	&(struct HTTPRecord){ NULL }
};

struct HTTPRecord *bodies[] = {
	&(struct HTTPRecord){ "field1", "text/html", (uint8_t *)"Lots of text", 12 },
	&(struct HTTPRecord){ "another_field", "text/plain", (uint8_t *)"dd323d9asdf", 11 },
	&(struct HTTPRecord){ "gunz", "text/plain", (uint8_t *)"Hello, world!", 13 },
	&(struct HTTPRecord){ NULL }
};


//I'd rather use a parse, but building directly and moving it is better
struct HTTPBody request_body_get_nopath = {
	.protocol = "HTTP/1.1",
	.method = "GET",
	.path = "/",
	.ctype = "text/html"
};

struct HTTPBody request_body_with_path = {
	.protocol = "HTTP/1.1",
	.method = "GET",
	.path = "/ashera",
	.ctype = "text/html"
};

struct HTTPBody request_application_x_www_url_formencoded = {
	.protocol = "HTTP/1.1",
	.method = "POST",
	.path = "/useless-post",
	.ctype = "application/x-www-form-urlencoded",
	.headers = headers, 
	.body = bodies
};

struct HTTPBody request_multipart_text = { 
	.protocol = "HTTP/1.1",
	.method = "POST",
	.ctype = "multipart/form-data",
	.path = "/post",
	.headers = headers,
	.body = bodies
};

struct HTTPBody *tests[] = {
	(struct HTTPBody *)&request_body_get_nopath,
	(struct HTTPBody *)&request_body_with_path,
	(struct HTTPBody *)&request_application_x_www_url_formencoded,
	(struct HTTPBody *)&request_multipart_text,
	NULL
};

struct filter_test {
	const char *name, *path, *root;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, void * ); 
	uint8_t *expected;
	int len;
};

struct filter_test fp[] = {
	{ "static", "tests/filters/static/text", "/index.html", filter_static },
	{ "static", "tests/filters/static/binary", "/aeon.jpg", filter_static },
	{ "echo", NULL, NULL, filter_echo },
#if 0
	{ "lua", filter_lua },
	{ "memory", filter_memory },
#endif
	{ NULL }
};


int main ( int argc, char *argv[] ) {

	struct filter_test *f = fp;
	while ( f->name ) {
		fprintf( stderr, "Running test on filter '%s'\n", f->name );
		//Build some possible requests
		struct HTTPBody **tb = tests;
		while ( *tb ) {
			fprintf( stderr, "%s\n", (*tb)->method );
			char err[2048] = { 0 };
			struct HTTPBody response = { 0 };
			struct config config = { 0 };
			config.path = f->path;
			config.root_default = f->root;

			//Run the filter on it...
			int status = f->filter( *tb, &response, &config );
			fprintf( stderr, !status ? "FAILED:" : "SUCCESS:\n" );
			if ( !status ) {
				//If it failed, just write the message to the thing
				int p = memstrat( response.msg, "\r\n\r\n", response.mlen );
				fprintf( stderr, " %d %d\n", response.status, response.mlen );
			}
			write( 2, response.msg, response.mlen );
			tb++;
		}
		f++;
	}

	return 0;
}
