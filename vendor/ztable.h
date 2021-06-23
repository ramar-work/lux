/* ----------------------------------------------------------------
 * ztable.h
 * ========
 * 
 * Summary
 * =======
 * ztable is a library for handling hash tables in C.  It is intended 
 * as a drop-in two-file library that takes little work to setup and teardown, 
 * and even less work to integrate into a project of your own.
 * 
 * ztable can be built with: 
 * 	`gcc -Wall -Werror -std=c99 ztable.c main.c`
 * 
 * 
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * ---------------------------------------------------------------- */

#ifndef _WIN32
 #define _POSIX_C_SOURCE 200809L
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef ZTABLE_H
#define ZTABLE_H

#ifndef LT_DEVICE
 #define LT_DEVICE 2
#endif


#define ZTABLE_ERRV_LENGTH 127 

#define LT_BUFLEN 2047 

#ifndef LT_MAX_COLLISIONS
 #define LT_MAX_COLLISIONS 10
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
	!( __ltHistoric.level = 0 ) && fprintf( stderr, "LEVEL IS %d\n", __ltHistoric.level ) && lt_exec( t, &__ltHistoric, __lt_dump )

#define lt_fdump(t, i) \
	!( __ltHistoric.level = 0 ) && ( __ltHistoric.fd = i ) && lt_exec( t, &__ltHistoric, __lt_dump ) && fflush( stdout )

#define lt_kfdump(t, i) \
	!( __ltComplex.level = 0 ) && ( __ltComplex.fd = i ) && lt_exec( t, &__ltComplex, __lt_dump ) && fflush( stdout )

#define lt_kdump(t) \
	!( __ltComplex.level = 0 ) && lt_exec( t, &__ltComplex, __lt_dump )

#define lt_sdump(t) \
	lt_exec( t, &__ltSimple, __lt_dump )

#define lt_blob_at( t, i ) \
	lt_ret( t, ZTABLE_BLB, i )->vblob

#define lt_blobdata_at( t, i ) \
	lt_ret( t, ZTABLE_BLB, i )->vblob.blob

#define lt_blobsize_at( t, i ) \
	lt_ret( t, ZTABLE_BLB, i )->vblob.size

#define lt_int_at( t, i ) \
	lt_ret( t, ZTABLE_INT, i )->vint

#define lt_float_at( t, i ) \
	lt_ret( t, ZTABLE_FLT, i )->vfloat

#define lt_text_at( t, i ) \
	lt_ret( t, ZTABLE_TXT, i )->vchar

#define lt_userdata_at( t, i ) \
	lt_ret( t, ZTABLE_USR, i )->vusrdata

#define lt_table_at( t, i ) \
	lt_ret( t, ZTABLE_TBL, i )->vtable

#define lt_blob( t, key ) \
	lt_ret( t, ZTABLE_BLB, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vblob

#define lt_blobdata( t, key ) \
	lt_ret( t, ZTABLE_BLB, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vblob.blob

#define lt_blobsize( t, key ) \
	lt_ret( t, ZTABLE_BLB, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vblob.size

#define lt_int( t, key ) \
	lt_ret( t, ZTABLE_INT, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vint

#define lt_float( t, key ) \
	lt_ret( t, ZTABLE_FLT, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vfloat

#define lt_text( t, key ) \
	lt_ret( t, ZTABLE_TXT, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vchar

#define lt_userdata( t, key ) \
	lt_ret( t, ZTABLE_USR, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vusrdata

#define lt_table( t, key ) \
	lt_ret( t, ZTABLE_TBL, lt_get_long_i(t, (unsigned char *)key, strlen(key)) )->vtable

#define lt_lblob( t, key, len ) \
	lt_ret( t, ZTABLE_BLB, lt_get_long_i(t, (unsigned char *)key, len) )->vblob

#define lt_lblobdata( t, key, len ) \
	lt_ret( t, ZTABLE_BLB, lt_get_long_i(t, (unsigned char *)key, len) )->vblob.blob

#define lt_lblobsize( t, key, len ) \
	lt_ret( t, ZTABLE_BLB, lt_get_long_i(t, (unsigned char *)key, len) )->vblob.size

#define lt_lint( t, key, len ) \
	lt_ret( t, ZTABLE_INT, lt_get_long_i(t, (unsigned char *)key, len) )->vint

#define lt_lfloat( t, key, len ) \
	lt_ret( t, ZTABLE_FLT, lt_get_long_i(t, (unsigned char *)key, len) )->vfloat

#define lt_ltext( t, key, len ) \
	lt_ret( t, ZTABLE_TXT, lt_get_long_i(t, (unsigned char *)key, len) )->vchar

#define lt_luserdata( t, key, len ) \
	lt_ret( t, ZTABLE_USR, lt_get_long_i(t, (unsigned char *)key, len) )->vusrdata

#define lt_ltable( t, key, len ) \
	lt_ret( t, ZTABLE_TBL, lt_get_long_i(t, (unsigned char *)key, len) )->vtable

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
	lt_add(t, 0, ZTABLE_INT, v, 0, 0, 0, 0, 0, 0, NULL)

#define lt_addintvalue(t, v) \
	lt_add(t, 1, ZTABLE_INT, v, 0, 0, 0, 0, 0, 0, NULL)

#define lt_addtextkey(t, v) \
	lt_add(t, 0, ZTABLE_TXT, 0, 0, 0, (unsigned char *)v, strlen(v), 0, 0, NULL)

#define lt_addtextvalue(t, v) \
	lt_add(t, 1, ZTABLE_TXT, 0, 0, 0, (unsigned char *)v, strlen(v), 0, 0, NULL)

#define lt_addblobdkey(t, v, vlen) \
	lt_add(t, 0, ZTABLE_TXT, 0, 0, 0, (unsigned char *)v, vlen, 0, 0, NULL)

#define lt_addblobdvalue(t, v, vlen) \
	lt_add(t, 1, ZTABLE_TXT, 0, 0, 0, (unsigned char *)v, vlen, 0, 0, NULL)

#define lt_addblobkey(t, vblob, vlen) \
	lt_add(t, 0, ZTABLE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)

#define lt_addblobvalue(t, vblob, vlen) \
	lt_add(t, 1, ZTABLE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)

#define lt_addfloatvalue(t, v) \
	lt_add(t, 1, ZTABLE_FLT, 0, v, 0, 0, 0, 0, 0, NULL)

#ifdef ZTABLE_NUL
 #define lt_addnullvalue(t) \
	lt_add(t, 1, ZTABLE_NUL, 0, 0, 0, 0, 0, 0, 0, NULL)
#endif

#define lt_addudvalue(t, v) \
	lt_add(t, 1, ZTABLE_USR, 0, 0, 0, 0, 0, v, 0, NULL)

#define lt_addik(t, v) \
	lt_add(t, 0, ZTABLE_INT, v, 0, 0, 0, 0, 0, 0, NULL)

#define lt_addiv(t, v) \
	lt_add(t, 1, ZTABLE_INT, v, 0, 0, 0, 0, 0, 0, NULL)

#define lt_addtk(t, v) \
	lt_add(t, 0, ZTABLE_TXT, 0, 0, 0, (unsigned char *)v, strlen(v), 0, 0, NULL)

#define lt_addtv(t, v) \
	lt_add(t, 1, ZTABLE_TXT, 0, 0, 0, (unsigned char *)v, strlen(v), 0, 0, NULL)

#define lt_addbk(t, vblob, vlen) \
	lt_add(t, 0, ZTABLE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)

#define lt_addbv(t, vblob, vlen) \
	lt_add(t, 1, ZTABLE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)

#define lt_addfv(t, v) \
	lt_add(t, 1, ZTABLE_FLT, 0, v, 0, 0, 0, 0, 0, NULL)

#ifdef ZTABLE_NUL
 #define lt_addnv(t) \
	lt_add(t, 1, ZTABLE_NUL, 0, 0, 0, 0, 0, 0, 0, NULL)
#endif

#define lt_adduv(t, v) \
	lt_add(t, 1, ZTABLE_USR, 0, 0, 0, 0, 0, v, 0, NULL)

#define lt_addtbk(t, str, vblob, vlen) \
	lt_add(t, 0, ZTABLE_BLB, 0, 0, 0, vblob, vlen, 0, 0, str)

#define lt_addtbv(t, str, vblob, vlen) \
	lt_add(t, 1, ZTABLE_BLB, 0, 0, 0, vblob, vlen, 0, 0, str)

#define lt_items(t, str) \
	lt_items_i(t, (unsigned char*)str, strlen((char *)str))

#define lt_iitems(t, ind) \
	lt_items_by_index(t, ind )

#define lt_within( t, str ) \
 	lt_within_long( t, (unsigned char *)str, strlen(str))

#define lt_copy_by_key(t, start) \
	lt_deep_copy (t, lt_geti( t, start ), t->count )

#define lt_copy_by_index(t, start) \
	lt_deep_copy (t, start, t->count )

enum {
	ZTABLE_ERR_NONE = 0,
	ZTABLE_ERR_LT_ALLOCATE,
	ZTABLE_ERR_LT_OUT_OF_SPACE,
	ZTABLE_ERR_LT_INVALID_VALUE,
	ZTABLE_ERR_LT_INVALID_TYPE,
	ZTABLE_ERR_LT_INVALID_INDEX,
	ZTABLE_ERR_LT_OUT_OF_SLICE,
	ZTABLE_ERR_LT_MAX_COLLISIONS,
	ZTABLE_ERR_LT_INVALID_KEY,
	ZTABLE_ERR_LT_INDEX_MAX,
};

//Define a type polymorph with a bunch of things to help type inference
typedef struct zhValue zhValue;

typedef struct zhBlob zhBlob;

typedef struct zhTable zhTable;

typedef struct zKeyval zKeyval;

typedef union zhRecord zhRecord;

typedef struct { 
	enum { LT_DUMP_SHORT, LT_DUMP_LONG } dumptype;
	char fd, level, *buffer; 
} zhInner;

//Table for table values
typedef enum {
  ZTABLE_NON = 0, //Uninitialized values
  ZTABLE_INT,     //Integer
  ZTABLE_FLT,     //FLoat
  ZTABLE_TXT,     //Text
  ZTABLE_BLB,     //Blobs (strings that don't terminate come back as blobs too)
  ZTABLE_NUL,     //Null
  ZTABLE_USR,     //Userdata
  ZTABLE_TBL,     //A "table"
  ZTABLE_TRM,     //Table terminator (NULL alone can't be described)
  ZTABLE_NOD,     //A node
} zhType;

typedef struct {
  unsigned int total  ;     //Size allocated (the bound)
	unsigned int index  ;     //Index to current element
	unsigned int count  ;     //Elements in table
	unsigned int *rCount;     //Elements in current table 
	unsigned short modulo ;     //Optimal modulus value for hashing
  int mallocd;     //An error occurred, read it...
	int srcmallocd;  //An error occurred, read it...
	int size   ;     //Size of newly trimmed key or pointer
	int cptr   ;     //Table will stop here
	int start  ;     //Table bounds are here if "lt_within" is used
	int end    ;
	int buflen ;
#ifdef DEBUG_H
	int collisions;
#endif
  unsigned char *src; //Source for when you need it
  unsigned char *buf; //Pointer for trimmed keys and values
  zKeyval *head; //Pointer to the first element
  zhTable *current; //Pointer to the current element
	void *ptr; //A random void pointer...
  int error;
#ifdef ZTABLE_ERR_EXP 
	char errmsg[ ZTABLE_ERRV_LENGTH ];
#endif
} zTable;

struct zhTable {
  unsigned int count;
  long ptr;
  zhTable *parent;
};

struct zhBlob {
  int size;
  unsigned char *blob;
};

union zhRecord {
  int vint;
  float vfloat;
  char *vchar;
#ifdef ZTABLE_NUL
  void *vnull;
#endif
  void *vusrdata;
  zhBlob vblob;
  zhTable vtable;
  long vptr;
};

struct zhValue {
  zhType type;
  zhRecord v;
};

struct zKeyval {
  zhValue key; 
  zhValue value;
  zKeyval *parent;  
	//This works b/c we can hash at lt_lock... 
	//Can't tell if there's a way to use allocation to get this done
	int index[ LT_MAX_COLLISIONS ];
#ifdef DEBUG_H
	//int collisions;
	//int ???;
#endif

  //zKeyval *next[ LT_MAX_COLLISIONS ];
  //int hash[LT_MAX_COLLISIONS];
};

extern zhInner __ltComplex; 

extern zhInner __ltHistoric; 

extern zhInner __ltSimple; 

zhType lt_add (zTable *, int, zhType, 
	int, float, char *, unsigned char *, unsigned int , void *, zTable *, char *);

zTable *lt_init (zTable *, zKeyval *, int) ;

zTable *lt_make ( int ) ;

void lt_printall (zTable *);

void lt_finalize (zTable *) ;

int __lt_dump (zKeyval *, int, void *);

int lt_exec_complex (zTable *, int, int, void *, int (*fp)(zKeyval *, int, void *) );

int lt_move(zTable *, int) ;

zKeyval *lt_retkv (zTable *, int);

zhType lt_rettype (zTable *, int, int);

const char *lt_rettypename (zTable *, int, int);

int lt_lock (zTable *); 

int lt_get_long_i (zTable *, unsigned char *, int);

unsigned char *lt_get_full_key (zTable *, int, unsigned char *, int);

zKeyval *lt_next (zTable *);

zKeyval *lt_current (zTable *);

void lt_reset (zTable *);

int lt_set (zTable *, int);

int lt_absset (zTable *, int);

int lt_get_raw (zTable *, int);

zhValue *lt_retany (zTable *, int );

zhRecord *lt_ret (zTable *, zhType, int );

void lt_setsrc (zTable *, void *);

void lt_free (zTable *);

unsigned char *lt_trim (unsigned char *, char *, int, int *);

zKeyval *lt_items_i (zTable *, unsigned char *, int);

zKeyval *lt_items_by_index (zTable *, int);

int lt_count_elements ( zTable *, int);

int lt_exists (zTable *, int);

int lt_count_at_index ( zTable *, int, int);

int lt_countall ( zTable * );

zTable *lt_within_long( zTable *, unsigned char *, int);

const char *lt_typename (int);

zTable *lt_deep_copy (zTable *t, int from, int to); 

const char *lt_strerror (zTable *);

void lt_clearerror (zTable *);

int build_backwards (zKeyval *, unsigned char *, int );

#ifdef DEBUG_H
void lt_printt (zTable *);

void lt_free_keys ( const char ** );

const char ** lt_get_keys ( zTable * );
#endif

#endif
