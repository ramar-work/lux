#include "../filter-test.h"
#include "../filter-static.h"
#define TESTDIR "tests/filter-static/"

struct config configs[] = {
	{ .path = TESTDIR "text", .root_default = "index.html" },
	{ .path = TESTDIR "text"  },
	{ .path = TESTDIR "binary", .root_default = "index.html"  },
	{ .path = TESTDIR "binary", .root_default = "aeon.jpg" },
	{ NULL }
};

struct Test tests[] = {
	REQUEST(root, "GET", "/", NULL, NULL, TEXTHTML),
	REQUEST(level2url, "GET", "/ashera/two", NULL, NULL, TEXTHTML),
	REQUEST(404_never_find_me, "GET", "/you-will-never-find-me", NULL, NULL, TEXTHTML),
#if 0
	REQUEST(static_file_missing, "GET", "/static/not_found.jpg", NULL, NULL, TEXTHTML),
	REQUEST(static_file_present, "GET", "/static/handtinywhite.gif", NULL, NULL, TEXTHTML),
	REQUEST(multipart_post, "POST", "/beef", headers, bodies, MULTIPART),
#endif
	{ NULL }
};

int main ( int argc, char *argv[] ) {
	return filter_test( "static", filter_static, configs, tests );
}
