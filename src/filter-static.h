#include "http.h"
#include "mime.h"
#include "util.h"

#ifndef FILTER_STATIC_H
#define FILTER_STATIC_H

int h_proc ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx );

#endif
