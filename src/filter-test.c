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

struct Test cases[] = {
	TESTCASE(root, "GET", "/", NULL, NULL, TEXTHTML),
	TESTCASE(appxwww_post, "POST", "/post", headers, bodies, X_APP_WWW),
	TESTCASE(multipart_post, "POST", "/post", headers, bodies, MULTIPART),
#if 0
	TESTCASE(404_never_find_me, "GET", "/you-will-never-find-me", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_missing, "GET", "/static/not_found.jpg", NULL, NULL, TEXTHTML),
	TESTCASE(static_file_present, "GET", "/static/present.gif", NULL, NULL, TEXTHTML),
	TESTCASE(level1url, "GET", "/turkey", NULL, NULL, TEXTHTML),
	TESTCASE(level2url, "GET", "/recipe/2", NULL, NULL, TEXTHTML),
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
}; 


struct filter_test filter_tests[] = {
#if 0
	FILTER( filter_lua, TEST_LUA_DIR "bad-config", "/index.lua" ),
	FILTER( filter_lua, TEST_LUA_DIR "blank-config", "/index.lua" ),
	//FILTER( filter_lua, TEST_LUA_DIR "split-config", "/index.lua" ),
#endif
	FILTER( filter_lua, TEST_LUA_DIR "echo.local", "/index.lua" ),
#if 0
	FILTER( filter_lua, TEST_LUA_DIR "error-model", "/index.lua" ),
	FILTER( filter_lua, TEST_LUA_DIR "db-model", "/index.lua" ),
	FILTER( filter_lua, TEST_LUA_DIR "very-large-config", "/index.lua" ),
	FILTER( filter_lua, TEST_LUA_DIR "xl-model", "/index.lua" ),
	FILTER( filter_lua, TEST_LUA_DIR "imggal", "/index.lua" ),
#endif
	{ NULL }
};

#if 0
struct filter_test filter_tests[] = {
	//Static filters can go here
	FILTER( filter_static, TEST_STATIC_DIR "static/text", "/index.html" ),
	FILTER( filter_static, TEST_STATIC_DIR "static/binary", "/index.html" ),
	{ NULL }
};
#endif

#if 0
struct filter_test filter_tests[] = {
	//zWalkerory filters can go here
	{ "memory", filter_memory },
	{ NULL }
};
#endif

#if 0
struct filter_test filter_tests[] = {
	//Echo filters can go here
	{ "echo", "/", "/", filter_echo },
	{ NULL }
};
#endif


//Write a formatted message to some kind of buffer
void log_buf ( struct HTTPBody *res, char *log, int loglen ) {
	const char logfmt[] = 
		"%d, %d, "; 
	memset( log, 0, loglen );
	int len = snprintf( log, loglen, logfmt, res->status, res->mlen );
	memcpy( &log[ len ], res->msg, res->mlen );
}


//Run the tests
int main ( int argc, char *argv[] ) {

	FILE *file = stderr;
	struct filter_test *f = filter_tests;

	while ( f->name ) {
		fprintf( stderr, "Running tests against filter '%s'\n", f->name );
		struct Test *test = cases;
		while ( test->name ) {
			char err[2048] = { 0 };
			struct HTTPBody response = { 0 };
			struct config config = { .root_default = f->root };
			struct host host = { .dir = (char *)f->path, .root_default = (char *)f->root };

			fprintf( file, "\n[ Test name: %-13s ]: ", test->name );

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
			
			//Writing the message out itself is easier
			char log[ 2048 ];
			log_buf( &response, log, sizeof(log) );
			fprintf( file, log );

			http_free_response( &response );
			test++;
		}
		f++;
	}

	return 0;
}
