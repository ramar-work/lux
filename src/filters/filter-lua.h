/* ------------------------------------------- * 
 * filter-lua.h
 * ===========
 * 
 * Summary 
 * -------
 * Header file for functions comprising the Lua 
 * filter for interpreting HTTP messages.
 *
 * Usage
 * -----
 * filter-lua.c allows hypno to use Lua as a 
 * server-side evaluation langauge.
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 * 
 * ------------------------------------------- */
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <zhttp.h>
#include <ztable.h>
#include <zrender.h>
#include <zmime.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <router.h>
#include <dirent.h>
#include "../util.h"
#include "../server.h"
#include "../lua.h"
#include "../lua/lib.h"
#include <zjson.h>
#include "../mime.h"

#ifndef FILTER_LUA_H
#define FILTER_LUA_H

#define LD_LEN 128
#define LD_PATH_LEN 1024
#define LD_ROUTE_LEN LD_LEN 
#define LD_DBNAME_LEN LD_LEN 
#define LD_FQDN_LEN 256 
#define LD_ERRBUF_LEN 1024

struct route_t { 
	int iroute_tlen;
	struct iroute_t { char *route; int index; } **iroute_tlist;
	zTable *src;
};


struct mvc_t {
	struct mvcmeta_t *mset;
	int flen, type; 
	int depth;   //track the depth, so you know when to stop iterating
	//int inherit; //a keyword

	int model;
	int view;
	int query;
	
	const char ctype[ 128 ];
	struct imvc_t {
		const char file[ 2048 ], base[ 128 ], ext[ 16 ];
		//leave some space here...
		//const char *dir;
	} **imvc_tlist;
};


struct luadata_t {
	zhttp_t *req, *res;
	lua_State *state;
	//TODO: This might be able to be pointers again...
	const char aroute[ LD_LEN ];
	const char rroute[ LD_ROUTE_LEN ];
	const char apath[ LD_PATH_LEN ];
	const char db[ LD_DBNAME_LEN ];
	const char fqdn[ LD_FQDN_LEN ];
	const char root[ LD_LEN ];
	int status; //can return a different status
	ztable_t *zconfig;
//ztable_t *zroutes;
	ztable_t *zroute;
	ztable_t *zmodel;
//ztable_t *zhttp; //TODO: you might not need this anymore...
	struct mvc_t pp; 
	char err[ LD_ERRBUF_LEN ];
};

const int filter_lua
( int , zhttp_t *, zhttp_t *, struct cdata * );
#endif
