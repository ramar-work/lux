//router.h
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "util.h"
#include "loader.h"
#include "mvc.h"

#ifndef ROUTER_H
#define ROUTER_H

#define DUMPACTION( NUM ) \
	( NUM == ACT_ID    ) ? "ACT_ID" : \
	( NUM == ACT_WILDCARD ) ? "ACT_WILDCARD" : \
	( NUM == ACT_SINGLE   ) ? "ACT_SINGLE" : \
	( NUM == ACT_EITHER   ) ? "ACT_EITHER" : \
	( NUM == ACT_RAW  ) ? "ACT_RAW" : \
	( NUM == ACT_NONE ) ? "ACT_NONE" : "UNKNOWN" 

#define DUMPMATCH( NUM ) \
	( NUM == RE_NUMBER    ) ? "RE_NUMBER" : \
	( NUM == RE_STRING ) ? "RE_STRING" : \
	( NUM == RE_ANY   ) ? "RE_ANY" : \
	( NUM == RE_NONE ) ? "RE_NONE" : "UNKNOWN" 

#if 1
typedef enum {
	RE_NONE = 0,
	RE_NUMBER = 31,
	RE_STRING,
	RE_ANY,
} RouterAction;

typedef enum {
	ACT_NONE = 0,
	ACT_ID = 34,
	ACT_WILDCARD,
	ACT_SINGLE,
	ACT_EITHER,
	ACT_RAW,
} RouterStatus;
#endif

struct routeh { 
	char *name; 
	struct mvc *mvc;
};

struct element {
	int len;
#if 1
	RouterStatus type;
	RouterAction mustbe;
#else
	int type;
	int mustbe;
#endif
	char **string;
};

struct urimap {
	const char *name, *routeset;
	int listlen;
	struct element **list;
	//Mem r;
};

#if 0
void dump_routeh ( struct routeh ** );
void free_routeh ( struct routeh ** );
#endif
void build_routeh ( struct routeh ** );
struct routeh * resolve_routeh ( struct routeh **, const char * );
void dump_routehs ( struct routeh **set ) ;
void free_urimap ( struct urimap * ) ;

#endif
