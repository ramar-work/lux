#include "http.h"
#include "config.h"
#include "filter-lua.h"
#if 0
#include "filter-static.h"
#include "filter-echo.h"
#include "filter-c.h"
#endif

#define TEST_C_DIR "tests/filter-c/"
#define TEST_LUA_DIR "tests/filter-lua/"
#define TEST_STATIC_DIR "tests/filter-static/"

#define TESTCASE( NAME, METHOD, PATH, HEADERS, BODIES, TEXTHTML ) \
	{ #NAME, { TEXTHTML, METHOD, PATH, HTTP_11, .headers=HEADERS, .body=BODIES } }

#define FILTER( FUNCT, PATH, DFILE ) \
	{ FUNCT, #FUNCT, PATH, DFILE }

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
}; 


struct Test static_cases[] = {
	TESTCASE(root, "GET", "/", NULL, NULL, TEXTHTML),
#if 0
	TESTCASE(level1url, "GET", "/ashera", NULL, NULL, TEXTHTML),
	TESTCASE(level2url, "GET", "/ashera/two", NULL, NULL, TEXTHTML),
	TESTCASE(404_never_find_me, "GET", "/you-will-never-find-me", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_missing, "GET", "/static/not_found.jpg", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_present, "GET", "/static/handtinywhite.gif", NULL, NULL, TEXTHTML),
	TESTCASE(multipart_post, "POST", "/beef", headers, bodies, MULTIPART),
	TESTCASE(appxwww_post, "POST", "/nope", headers, bodies, X_APP_WWW),
#endif
	{ NULL }
};

struct Test lua_cases[] = {
	TESTCASE(root, "GET", "/", NULL, NULL, TEXTHTML),
#if 0
	TESTCASE(level1url, "GET", "/ashera", NULL, NULL, TEXTHTML),
	TESTCASE(level2url, "GET", "/ashera/two", NULL, NULL, TEXTHTML),
	TESTCASE(404_never_find_me, "GET", "/you-will-never-find-me", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_missing, "GET", "/static/not_found.jpg", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_present, "GET", "/static/handtinywhite.gif", NULL, NULL, TEXTHTML),
	TESTCASE(multipart_post, "POST", "/beef", headers, bodies, MULTIPART),
#endif
#if 0
	TESTCASE(appxwww_post, "POST", "/nope", headers, bodies, X_APP_WWW),
#endif
	{ NULL }
};

struct filter_test {
	int (*filter)( 
		struct HTTPBody *, 
		struct HTTPBody *, 
		struct config *, 
		struct host * 
	); 
	const char *name; 
	const char *path;
	const char *root;
	uint8_t *expected;
	int len;
} fp[] = {
	FILTER( filter_lua, TEST_LUA_DIR "lil-model", "/index.lua" ),
#if 0
	{ filter_c  , TEST_C_DIR "submarine.local", "/index.html", filter_c },

	{ filter_lua, TEST_LUA_DIR "error-model", "/index.lua", filter_lua },
	{ filter_lua, TEST_LUA_DIR "big-model", "/index.lua", filter_lua },
	{ filter_lua, TEST_LUA_DIR "tests/filters/lua/dafoodsnob-bad-config", "/index.lua", filter_lua },

	{ "static", "tests/filters/static/text", "/index.html", filter_static },
	{ "static", "tests/filters/static/binary", "/aeon.jpg", filter_static },
	{ "memory", filter_memory },
	{ "echo", "/", "/", filter_echo },
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
			struct config config = { .root_default = f->root };
			struct host host = { .dir = (char *)f->path, .root_default = (char *)f->root };

			fprintf( stderr, "[ Test name: %-13s ]: ", test->name );

			//Run the filter on it...
			if ( f->filter( &test->request, &response, &config, &host ) )
				;//fprintf( stderr, "SUCCESS:\n" );
			else {
				;//fprintf( stderr, "FAILED" );
				//fprintf( stderr, " %d %d\n", response.status, response.mlen );
			}

#if 0
			//Optionally show the finished message?
			if ( 1 ) {	
				fprintf( stderr, "\nMessage contents:\n" );
				write( 2, response.msg, response.mlen );
			}
			fprintf( stderr, "\n===========\n" );
#endif
			//http_free_request( &request );
			http_free_response( &response );
			test++;
		}
		f++;
	}

	return 0;
}
