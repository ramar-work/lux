#include "http.h"
#include "mime.h"
#include "util.h"
#include "config.h"

#ifndef FILTER_STATIC_H
#define FILTER_STATIC_H

int filter_static( struct HTTPBody *, struct HTTPBody *, void * );
int check_static_prefix( const char *, const char * );

#endif
