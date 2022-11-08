#include "filter-test.h"
#include "../filter-c.h"
#define TESTDIR "tests/filter-c/"

struct config configs[] = {
	{ .path = TESTDIR "submarine.local" },
	{ NULL }
};

struct Test tests[] = {
	REQUEST(root, "GET", "/", NULL, NULL, TEXTHTML),
	REQUEST(level1url, "GET", "/user", NULL, NULL, TEXTHTML),
	REQUEST(level2url, "GET", "/user/two", NULL, NULL, TEXTHTML),
	REQUEST(404_never_find_me, "GET", "/you-will-never-find-me", NULL, NULL, TEXTHTML),
	REQUEST(static_file_missing, "GET", "/static/not_found.jpg", NULL, NULL, TEXTHTML),
	REQUEST(static_file_present, "GET", "/static/handtinywhite.gif", NULL, NULL, TEXTHTML),
	REQUEST(multipart_post, "POST", "/beef", headers, bodies, MULTIPART),
	{ NULL }
};

int main ( int argc, char *argv[] ) {
	return filter_test( "c", filter_c, configs, tests );
}
