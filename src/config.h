#ifndef CONFIG_H
#define CONFIG_H
#include "../vendor/single.h"
#include "luabind.h"
#include "util.h"
#endif

void *get_values ( Table *t, const char *key, void *userdata, int (*fp)(LiteKv *, int, void *) );


struct route ** build_routes ( Table *t );
struct host ** build_hosts ( Table *t );
int get_int_value ( Table *t, const char *key, int notFound );
char * get_char_value ( Table *t, const char *key );
