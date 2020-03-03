#include "http.h"
#include "util.h"
#include "luabind.h"
#include "config.h"
//#include "mime.h"
//#include "render.h"
//#include "router.h"

#ifndef FILTER_LUADUMP_H
#define FILTER_LUADUMP_H

int filter_lua ( struct HTTPBody *, struct HTTPBody *, void * );

#endif
