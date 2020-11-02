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
} zWalker;

#define strwalk(a,b,c) \
 memwalk(a, (uint8_t *)b, (uint8_t *)c, strlen(b), strlen((char *)c))

#define meminit(mems, p, m) \
 Mem mems; \
 memset(&mems, 0, sizeof(Mem)); \
 mems.pos = p; \
 mems.it = m; 

int memstr (const void *, const void *, int);
int32_t memchrocc (const void *, const char, int);
int32_t memstrocc (const void *, const void *, int);
int32_t memstrat (const void *, const void *, int);
int32_t memchrat (const void *, const char, int);
int32_t memtok (const void *, const uint8_t *, int32_t, int32_t );
int32_t memmatch (const void *, const char *, int32_t, char ); 
char *memstrcpy (char *, const uint8_t *, int32_t );
int memwalk (zWalker *mm, const uint8_t *, const uint8_t *, int, int ) ;

#endif
