#include "mime.h"
#include "util.h"
#include "luabind.h"
#include "config.h"
#include "loader.h"
//#include "render.h"
#include "router.h"
#include "mvc.h"
#include "../vendor/zrender.h"
#include "../vendor/zhttp.h"

#ifndef FILTER_LUA_H
#define FILTER_LUA_H

int filter_lua ( struct HTTPBody *, struct HTTPBody *, struct config *, struct host * );

struct luaconf {
	char *db;	
	char *fqdn;
	char *title;
	char *root_default;
	char *spath;
	char *dir;
	struct mvc *mvc;
	struct routeh **routes;
};
#endif
