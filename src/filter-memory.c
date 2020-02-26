#include "filter-memory.h"

struct memresponse { 
	const char *path;
	uint8_t *content;
	int clen;
} responses[] = {
	NULL
};

int filter_memory( struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//Serve some file from memory depending on the path name...
	return 0;
}
