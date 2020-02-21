#ifndef CONFIG_H
#define CONFIG_H
#include "../vendor/single.h"
#include "luabind.h"
#include "util.h"
#endif

void *get_values ( Table *t, const char *key, void *userdata, int (*fp)(LiteKv *, int, void *) );
void * build_routes ( Table *t );
void * build_hosts ( Table *t );
