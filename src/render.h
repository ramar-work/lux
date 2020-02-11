#ifndef RENDER_H
#define RENDER_H
#include "../vendor/single.h"
#include "luabind.h"
#include "util.h"
#endif

uint8_t *table_to_template ( Table *t, const uint8_t *src, int srclen, int * newlen );
