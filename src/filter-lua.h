#include "http.h"
#include "mime.h"
#include "util.h"
#include "luabind.h"
#include "config.h"
#include "render.h"
#include "routes.h"

#ifndef FILTER_LUA_H
#define FILTER_LUA_H

int filter_lua ( struct HTTPBody *, struct HTTPBody *, struct config *, struct host * );

#endif
