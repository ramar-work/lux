//zhasher.h

#ifndef _WIN32
 #define _POSIX_C_SOURCE 200809L
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#ifndef ZHASHER_H
#define ZHASHER_H

#ifndef LT_DEVICE
 #define LT_DEVICE 2
#endif

#define ZHASHER_ERRV_LENGTH 127 
#define LT_POLYMORPH_BUFLEN 2048
#define LT_MAX_HASH 7 
#if 0
#define lt_advance(t, pos) \
	lt_set( t, t->index + pos, 0 ) 
#define lt_rewind(t, pos) \
	lt_set( t, t->index - pos, 0 )
#endif
#define lt_counti(t, i) \
	lt_count_at_index( t, i, 1 ) 
#define lt_counta(t, i) \
	lt_count_at_index( t, i, 0 ) 
#define lt_advance(t, pos) \
	lt_set( t, pos ) 
#define lt_rewind(t, pos) \
	lt_set( t, pos )
#define lt_exec(t, a, b) \
	lt_exec_complex( t, 0, t->index, a, b )
#define lt_dump(t) \
	lt_exec( t, &__ltHistoric, __lt_dump )
#define lt_kdump(t) \
	lt_exec( t, &__ltComplex, __lt_dump )
#define lt_sdump(t) \
	lt_exec( t, &__ltSimple, __lt_dump )
#define lt_blob_at( t, i ) \
	lt_ret( t, LITE_BLB, i )->vblob
#define lt_blobdata_at( t, i ) \
	lt_ret( t, LITE_BLB, i )->vblob.blob
#define lt_blobsize_at( t, i ) \
	lt_ret( t, LITE_BLB, i )->vblob.size
#define lt_int_at( t, i ) \
	lt_ret( t, LITE_INT, i )->vint
#define lt_float_at( t, i ) \
	lt_ret( t, LITE_FLT, i )->vfloat
#define lt_text_at( t, i ) \
	lt_ret( t, LITE_TXT, i )->vchar
#define lt_userdata_at( t, i ) \
	lt_ret( t, LITE_USR, i )->vusrdata
#define lt_table_at( t, i ) \
	lt_ret( t, LITE_TBL, i )->vtable
#define lt_blob( t, key ) \
	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vblob
#define lt_blobdata( t, key ) \
	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vblob.blob
#define lt_blobsize( t, key ) \
	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vblob.size
#define lt_int( t, key ) \
	lt_ret( t, LITE_INT, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vint
#define lt_float( t, key ) \
	lt_ret( t, LITE_FLT, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vfloat
#define lt_text( t, key ) \
	lt_ret( t, LITE_TXT, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vchar
#define lt_userdata( t, key ) \
	lt_ret( t, LITE_USR, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vusrdata
#define lt_table( t, key ) \
	lt_ret( t, LITE_TBL, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vtable
#define lt_lblob( t, key, len ) \
	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, len) )->vblob
#define lt_lblobdata( t, key, len ) \
	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, len) )->vblob.blob
#define lt_lblobsize( t, key, len ) \
	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, len) )->vblob.size
#define lt_lint( t, key, len ) \
	lt_ret( t, LITE_INT, lt_get_long_i(t, (uint8_t *)key, len) )->vint
#define lt_lfloat( t, key, len ) \
	lt_ret( t, LITE_FLT, lt_get_long_i(t, (uint8_t *)key, len) )->vfloat
#define lt_ltext( t, key, len ) \
	lt_ret( t, LITE_TXT, lt_get_long_i(t, (uint8_t *)key, len) )->vchar
#define lt_luserdata( t, key, len ) \
	lt_ret( t, LITE_USR, lt_get_long_i(t, (uint8_t *)key, len) )->vusrdata
#define lt_ltable( t, key, len ) \
	lt_ret( t, LITE_TBL, lt_get_long_i(t, (uint8_t *)key, len) )->vtable
#define lt_ascend( t ) \
	lt_move( t, 1 )
#define lt_descend( t ) \
	lt_move( t, 0 )
#define lt_geti( t, key ) \
	lt_get_long_i( t, (unsigned char *)key, strlen((char *)key))
#define lt_keytype( t ) \
	lt_rettype( t, 0, (t)->index )
#define lt_valuetype( t ) \
	lt_rettype( t, 1, (t)->index )
#define lt_keytypename( t ) \
	lt_rettypename( t, 0, (t)->index )
#define lt_valuetypename( t ) \
	lt_rettypename( t, 1, (t)->index )
#define lt_keytypeat( t, i ) \
	lt_rettype( t, 0, i )
#define lt_valuetypeat( t, i ) \
	lt_rettype( t, 1, i )
#define lt_keytypenameat( t, i ) \
	lt_rettypename( t, 0, i )
#define lt_valuetypenameat( t, i ) \
	lt_rettypename( t, 1, i )
#define lt_kt( t ) \
	lt_rettype( t, 0, (t)->index )
#define lt_vt( t ) \
	lt_rettype( t, 1, (t)->index )
#define lt_ktn( t ) \
	lt_rettypename( t, 0, (t)->index )
#define lt_vtn( t ) \
	lt_rettypename( t, 1, (t)->index )
#define lt_kta( t, i ) \
	lt_rettype( t, 0, i )
#define lt_vta( t, i ) \
	lt_rettype( t, 1, i )
#define lt_ktna( t, i ) \
	lt_rettypename( t, 0, i )
#define lt_vtna( t, i ) \
	lt_rettypename( t, 1, i )
#define lt_addintkey(t, v) \
	lt_add(t, 0, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
#define lt_addintvalue(t, v) \
	lt_add(t, 1, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
#define lt_addtextkey(t, v) \
	lt_add(t, 0, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
#define lt_addtextvalue(t, v) \
	lt_add(t, 1, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
#define lt_addblobdkey(t, v, vlen) \
	lt_add(t, 0, LITE_TXT, 0, 0, 0, (uint8_t *)v, vlen, 0, 0, NULL)
#define lt_addblobdvalue(t, v, vlen) \
	lt_add(t, 1, LITE_TXT, 0, 0, 0, (uint8_t *)v, vlen, 0, 0, NULL)
#define lt_addblobkey(t, vblob, vlen) \
	lt_add(t, 0, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
#define lt_addblobvalue(t, vblob, vlen) \
	lt_add(t, 1, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
#define lt_addfloatvalue(t, v) \
	lt_add(t, 1, LITE_FLT, 0, v, 0, 0, 0, 0, 0, NULL)
#ifdef LITE_NUL
#define lt_addnullvalue(t) \
	lt_add(t, 1, LITE_NUL, 0, 0, 0, 0, 0, 0, 0, NULL)
#endif
#define lt_addudvalue(t, v) \
	lt_add(t, 1, LITE_USR, 0, 0, 0, 0, 0, v, 0, NULL)
#define lt_addik(t, v) \
	lt_add(t, 0, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
#define lt_addiv(t, v) \
	lt_add(t, 1, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
#define lt_addtk(t, v) \
	lt_add(t, 0, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
#define lt_addtv(t, v) \
	lt_add(t, 1, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
#define lt_addbk(t, vblob, vlen) \
	lt_add(t, 0, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
#define lt_addbv(t, vblob, vlen) \
	lt_add(t, 1, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
#define lt_addfv(t, v) \
	lt_add(t, 1, LITE_FLT, 0, v, 0, 0, 0, 0, 0, NULL)
#define lt_addnv(t) \
	lt_add(t, 1, LITE_NUL, 0, 0, 0, 0, 0, 0, 0, NULL)
#define lt_adduv(t, v) \
	lt_add(t, 1, LITE_USR, 0, 0, 0, 0, 0, v, 0, NULL)
#define lt_addtbk(t, str, vblob, vlen) \
	lt_add(t, 0, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, str)
#define lt_addtbv(t, str, vblob, vlen) \
	lt_add(t, 1, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, str)
#define lt_items(t, str) \
	lt_items_i(t, (uint8_t*)str, strlen((char *)str))
#define lt_iitems(t, ind) \
	lt_items_by_index(t, ind )
#define lt_within( t, str ) \
 	lt_within_long( t, (uint8_t *)str, strlen(str))

#if 1
#define SHOWDATA(...)
#else
#define SHOWDATA(...) do { \
 fprintf(stderr, "%-30s [ %s %d ] ( %s ) ", __func__, __FILE__, __LINE__, __CURRENT_TIME__ ); \
 fprintf( stderr, __VA_ARGS__ ); \
 fprintf( stderr, "\n"); } while (0)
#endif

enum {
	ZHASHER_ERR_NONE,
	ZHASHER_ERR_LT_ALLOCATE,
	ZHASHER_ERR_LT_OUT_OF_SPACE,
	ZHASHER_ERR_LT_INVALID_VALUE,
	ZHASHER_ERR_LT_INVALID_TYPE,
	ZHASHER_ERR_LT_INVALID_INDEX,
	ZHASHER_ERR_LT_OUT_OF_SLICE,
	ZHASHER_ERR_LT_INDEX_MAX,
};

//Define a type polymorph with a bunch of things to help type inference
typedef struct LiteValue LiteValue;
typedef struct LiteBlob LiteBlob;
typedef struct LiteTable LiteTable;
typedef struct LiteKv LiteKv;
typedef union  LiteRecord LiteRecord;
//typedef struct LiteNode LiteNode;

typedef struct { 
	enum { LT_DUMP_SHORT, LT_DUMP_LONG } dumptype; 
	enum { LT_CONDENSED, LT_VERBOSE } indextype;
	const char *customfmt;
	int level; 
} LtInner;

//Table for table values
typedef enum {
  LITE_NON = 0, //Uninitialized values
  LITE_INT,     //Integer
  LITE_FLT,     //FLoat
  LITE_TXT,     //Text
  LITE_BLB,     //Blobs (strings that don't terminate come back as blobs too)
  LITE_NUL,     //Null
  LITE_USR,     //Userdata
  LITE_TBL,     //A "table"
  LITE_TRM,     //Table terminator (NULL alone can't be described)
  LITE_NOD,     //A node
} LiteType;

typedef struct {
  unsigned int  total  ,     //Size allocated (the bound)
                modulo ,     //Optimal modulus value for hashing
                index  ,     //Index to current element
                count  ,     //Elements in table
                *rCount;     //Elements in current table 
  int           mallocd,     //An error occurred, read it...
                srcmallocd,  //An error occurred, read it...
                size   ,     //Size of newly trimmed key or pointer
                cptr   ,     //Table will stop here
                start  ,     //Table bounds are here if "lt_within" is used
                end    ,
                buflen ;
  unsigned char *src   ;     //Source for when you need it
  unsigned char *buf   ;     //Pointer for trimmed keys and values
  LiteKv        *head  ;     //Pointer to the first element
  LiteTable     *current;    //Pointer to the first element
 #ifndef ERR_H
  int error;
	#ifndef ERRV_H
	char  errmsg[ ZHASHER_ERRV_LENGTH ];
	#endif 
 #endif
  
} Table;

struct LiteTable {
  uint32_t  count;
  long      ptr;
  LiteTable *parent;
};

struct LiteBlob {
  int size;
  uint8_t *blob;
};

union LiteRecord {
  int         vint;
  float       vfloat;
  char       *vchar;
#ifdef LITE_NUL
  void       *vnull;
#endif
  void       *vusrdata;
  LiteBlob    vblob;
  LiteTable   vtable;
  long        vptr;
#if 0
  LiteNode    vnode;
#endif
};

struct LiteValue {
  LiteType    type;
  LiteRecord  v;
};

struct LiteKv {
  int hash[LT_MAX_HASH];
  LiteKv *parent;  
  LiteValue key; 
  LiteValue value;
};

LiteType lt_add (Table *, int, LiteType, int, float, char *, unsigned char *, unsigned int , void *, Table *, char *);
Table *lt_init (Table *, LiteKv *, int) ;
void lt_printall ( Table *t );
void lt_finalize (Table *t) ;
int __lt_dump ( LiteKv *kv, int i, void *p );
extern LtInner __ltComplex; 
extern LtInner __ltHistoric; 
extern LtInner __ltSimple; 
int lt_exec_complex (Table *t, int start, int end, void *p, int (*fp)( LiteKv *kv, int i, void *p ) );
int lt_move(Table *t, int dir) ;
LiteKv *lt_retkv (Table *t, int index);
LiteType lt_rettype( Table *t, int side, int index );
const char *lt_rettypename( Table *t, int side, int index );
void lt_lock (Table *t); 
int lt_get_long_i (Table *t, unsigned char *find, int len);
unsigned char *lt_get_full_key ( Table *t, int hash, unsigned char *buf, int bs );
LiteKv *lt_next (Table *t);
LiteKv *lt_current (Table *t);
void lt_reset (Table *t);
int lt_set (Table *t, int index);
int lt_absset( Table *t, int index ) ;
int lt_get_raw (Table *t, int index);
LiteValue *lt_retany (Table *t, int index);
LiteRecord *lt_ret (Table *t, LiteType type, int index);
const char *lt_strerror (Table *t);
void lt_clearerror (Table *t);
void lt_setsrc (Table *t, void *src);
void lt_free (Table *t);
unsigned char *lt_trim (uint8_t *msg, char *trim, int len, int *nlen);
LiteKv *lt_items_i (Table *t, uint8_t *src, int len);
LiteKv *lt_items_by_index (Table *t, int ind);
int lt_count_elements ( Table *t, int index );
//Table *lt_copy (Table *t, int from, int to); 
int lt_exists (Table *t, int index);
int lt_count_at_index ( Table *t, int index, int type );
int lt_countall ( Table *t );
//Table *lt_within_long ( Table *t, uint8_t *src, int len );
Table *lt_within_long( Table *st, uint8_t *src, int len );
const char *lt_typename (int type);
#ifdef DEBUG_H
 /*This is only enabled when debugging*/
 void lt_printt (Table *t);
#endif

#endif
