#include "../server/server.h"

#if 0
#ifndef SFILTER_H
#define SFILTER_H

//Filter for each request interpreter 
struct filter {
	const char *name;
	//const int (*filter)( int, zhttp_t *, zhttp_t *, struct cdata * );
	const int (*filter)( server_t *, conn_t * );
};

#endif
#endif

