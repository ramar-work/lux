#include "http.h"
#include "config.h"
#include "filter-static.h"
#include "filter-echo.h"
#include "filter-lua.h"

#define TESTCASE( NAME, METHOD, PATH, HEADERS, BODIES, TEXTHTML ) \
	{ #NAME, { TEXTHTML, METHOD, PATH, HTTP_11, .headers=HEADERS, .body=BODIES } }

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

char X_APP_WWW[] = "application/x-www-form-urlencoded";
char MULTIPART[] = "multipart/form-data";
char TEXTHTML[] = "text/html";
char HTTP_11[] = "HTTP/1.1";


struct Test {
	const char *name;
	struct HTTPBody request;
} testsp[] = {
#if 0
	TESTCASE(root, "GET", "/", NULL, NULL, TEXTHTML),
	TESTCASE(level1url, "GET", "/ashera", NULL, NULL, TEXTHTML),
	TESTCASE(level2url, "GET", "/ashera/two", NULL, NULL, TEXTHTML),
#endif
	TESTCASE(404_never_find_me, "GET", "/you-will-never-find-me", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_missing, "GET", "/static/not_found.jpg", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_present, "GET", "/static/handtinywhite.gif", NULL, NULL, TEXTHTML),
	TESTCASE(multipart_post, "POST", "/beef", headers, bodies, MULTIPART),
#if 0
	TESTCASE(appxwww_post, "POST", "/nope", headers, bodies, X_APP_WWW),
#endif
	{ NULL }
};


struct filter_test {
	const char *name, *path, *root;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, void * ); 
	uint8_t *expected;
	int len;
} fp[] = {
	{ "lua", "tests/filters/lua/dafoodsnob-bad-config", "/index.lua", filter_lua },
#if 0
	{ "static", "tests/filters/static/text", "/index.html", filter_static },
	{ "static", "tests/filters/static/binary", "/aeon.jpg", filter_static },
	{ "echo", NULL, NULL, filter_echo },
	{ "memory", filter_memory },
#endif
	{ NULL }
};


int main ( int argc, char *argv[] ) {

	struct filter_test *f = fp;
	while ( f->name ) {
		fprintf( stderr, "Running tests against filter '%s'\n", f->name );
		struct Test *test = testsp;
		while ( test->name ) {
			char err[2048] = { 0 };
			struct HTTPBody response = { 0 };
			struct config config = { .path = f->path, .root_default = f->root };
			fprintf( stderr, "============\n" );
			fprintf( stderr, "[ Test name: %-13s ]: ", test->name );

			//Run the filter on it...
			//TODO: The messages here make no sense unless you create 
			//expected responses and compare them.
			if ( f->filter( &test->request, &response, &config ) )
				;//fprintf( stderr, "SUCCESS:\n" );
			else {
				;//fprintf( stderr, "FAILED" );
				//fprintf( stderr, " %d %d\n", response.status, response.mlen );
			}

			//Optionally show the finished message?
			if ( 1 ) {	
				fprintf( stderr, "\nMessage contents:\n" );
				write( 2, response.msg, response.mlen );
			}
			fprintf( stderr, "\n===========\n" );
			http_free_response( &response );
			test++;
		}
		f++;
	}

	return 0;
}
