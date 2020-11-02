#include "vendor/zwalker.h"
#include "vendor/zhasher.h"
#include "vendor/util.h"


#ifndef RENDER_H
#define RENDER_H

#define RPRINTF(a,b,blen) \
	memset( rprintchar, 0, sizeof(rprintchar) ); \
	memcpy( rprintchar, b, (blen >= sizeof(rprintchar)) ? sizeof(rprintchar) - 1 : blen ); \
	FPRINTF( "%s > %s(...): '%s'\n", a, __func__, replace_chars( rprintchar, strlen(rprintchar) ) );

#define EXTRACTOR(name) \
	void name ( struct map ***row, uint8_t **dst, int *dlen, uint8_t *src, int len, struct ptr **ptr, void *t )

#define MAPPER(name) \
	void name ( struct map *row, struct map ***parent, int *plen, uint8_t *ptr, int len, void *t )


enum {
	RAW = 0,
	LOOP_START = 30,
	LOOP_END = 31,
	SIMPLE_EXTRACT = 32,
	COMPLEX_EXTRACT = 33,
	EACH_KEY = 34,
	EXECUTE = 35,
	BOOLEAN = 36,
	BLOCK_START = 100,
	BLOCK_END = 101,
	TERM = -2,
	UNINIT = 127,
};

struct map { 
	char action; 
	int **hashList; 
	int len; 
	int pos; 
	void *ptr; 
	int children; 
	struct map *parent;
};

//You can alternately just get rid of this by allocating enough space for the new things
struct ptr {
	struct map **index;
	int current;
	int children;	
};

typedef void (*Mapper)
	( struct map *, struct map ***, int *, uint8_t *, int, void * );

typedef void (*Extractor)
	( struct map ***, uint8_t **, int *, uint8_t *, int, struct ptr **, void * );

struct zrSet {
	Mapper mapper;
	Extractor extractor;
};

typedef struct zRender {
	const char *zStart, *zEnd;
	void *userdata;
	int zrlen;
	struct zrSet *mapset[128]; //takes up more space
} zRender;

//static?
void extract_table_value( zKeyval *, uint8_t **, int *, uint8_t *, int );
//struct map **table_to_map( void *, const uint8_t *, int );
//uint8_t *map_to_uint8t( void *, struct map **, int * );
uint8_t *zrender_render ( zRender *, const uint8_t *, int, int * );

//public?
int zrender_check_balance ( zRender *, const uint8_t *, int );
void zrender_free_table( struct map **map );
int * zrender_copy_int ( int i ) ;
uint8_t *zrender_trim ( uint8_t *msg, const char *trim, int len, int *nlen ) ;

zRender * zrender_init();
void zrender_set_boundaries ( zRender *, const char *, const char *);
void zrender_set_fetchdata( zRender *, void * );
void zrender_set( zRender *, const char, Mapper, Extractor );
void zrender_set_default_dialect( zRender *rz ) ;

#ifdef DEBUG_H
void zrender_print_table ( struct map ** );

#define DUMPACTION( NUM ) \
	( NUM == LOOP_START ) ? "LOOP_START" : \
	( NUM == LOOP_END ) ? "LOOP_END" : \
	( NUM == COMPLEX_EXTRACT ) ? "COMPLEX_EXTRACT" : \
	( NUM == SIMPLE_EXTRACT ) ? "SIMPLE_EXTRACT" : \
	( NUM == EACH_KEY ) ? "EACH_KEY" : \
	( NUM == EXECUTE ) ? "EXECUTE" : \
	( NUM == BOOLEAN ) ? "BOOLEAN" : \
	( NUM == RAW ) ? "RAW" : "UNKNOWN" 
#else
#define zrender_print_table(a)
#endif

#endif




