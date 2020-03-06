/*all data needed for filter testing*/
#include "http.h"
#include "config.h"

#define TPRINTF(...) \
	fprintf( stderr, __VA_ARGS__ )

#define REQUEST( NAME, METHOD, PATH, HEADERS, BODIES, TEXTHTML ) \
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
}; 


typedef int (*Filter)(struct HTTPBody *, struct HTTPBody *, void *);


int filter_test ( const char *fname, Filter filter, struct config *c, struct Test *tt ) {
	TPRINTF( "Running tests against filter '%s'\n", fname );
	while ( c->path ) {
		struct Test *t = tt;
		while ( t->name ) {
			const char fmt[] = "[ Test: %-13s, path: %s, URL: %s ]: ";
			char err[2048] = { 0 };
			struct HTTPBody response = { 0 };
			int f = filter( &t->request, &response, c );
			//TODO: Compare against expected response for a good test
			TPRINTF( fmt, t->name, c->path, t->request.path );
			TPRINTF( "%s (%d)", f ? "SUCCEEDED" : "FAILED", response.status );
		#if 0
			fprintf( stderr, "\nMessage contents:\n" );
			write( 2, response.msg, response.mlen );
		#endif
			http_free_response( &response );
			TPRINTF( "\n" );
			t++;
		}
		c++;
	}
	return 0;
}
