//zwalker.h
#ifndef _WIN32
 #define _POSIX_C_SOURCE 200809L
#endif 

#include <inttypes.h>
#include <string.h>

#ifndef ZWALKER_H
#define ZWALKER_H

typedef struct {
	int     pos,  //Position
         next,  //Next position
         size,  //Size of something
	         it;
	uint8_t chr;  //Character found
 #ifndef ERR_H
  int error;
	#ifndef ERRV_H
	char  errmsg[ 127 ];
	#endif 
 #endif
} Mem ;

#define strwalk(a,b,c) \
 memwalk(a, (uint8_t *)b, (uint8_t *)c, strlen(b), strlen((char *)c))

#define meminit(mems, p, m) \
 Mem mems; \
 memset(&mems, 0, sizeof(Mem)); \
 mems.pos = p; \
 mems.it = m; 

_Bool memstr (const void * a, const void *b, int size);
int32_t memchrocc (const void *a, const char b, int32_t size);
int32_t memstrocc (const void *a, const void *b, int32_t size);
int32_t memstrat (const void *a, const void *b, int32_t size);
int32_t memchrat (const void *a, const char b, int32_t size);
int32_t memtok (const void *a, const uint8_t *tokens, int32_t rng, int32_t tlen);
int32_t memmatch (const void *a, const char *tokens, int32_t sz, char delim); 
char *memstrcpy (char *dest, const uint8_t *src, int32_t len);
_Bool memwalk (Mem *mm, uint8_t *data, uint8_t *tokens, int datalen, int toklen) ;

#endif
