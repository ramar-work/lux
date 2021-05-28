/* ----------------------------------------------------------------
 * ztable.c
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
#include "ztable.h"

static const unsigned int lt_hash = 31;

zhInner __ltComplex = { LT_DUMP_LONG, LT_VERBOSE, NULL, 0 };
 
zhInner __ltHistoric = { LT_DUMP_SHORT, LT_VERBOSE, NULL, 0 };

zhInner __ltSimple = { LT_DUMP_SHORT, LT_CONDENSED, NULL, 0 };

static const char __lt_fmt[] =
	"[%-5d] (%d) %s";

#ifndef LT_TABDUMP
static const char __lt_ws[] = 
	"                                                  "
	"                                                  "
;
#else
static const char __lt_ws[] = 
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
;
#endif 
 
static const char *lt_errors[] = {
	[ZTABLE_ERR_LT_ALLOCATE]         = "Failed to allocate space for zTable",
	[ZTABLE_ERR_LT_OUT_OF_SPACE]     = "Out of space",
	[ZTABLE_ERR_LT_INVALID_VALUE]    = "Attempted to add invalid value.",
	[ZTABLE_ERR_LT_INVALID_TYPE]     = "Invalid type requested.",
	[ZTABLE_ERR_LT_INVALID_INDEX]    = "Attempted to access uninitialized index.",
	[ZTABLE_ERR_LT_OUT_OF_SLICE]     = "Value is out of requested range",	
	[ZTABLE_ERR_LT_INDEX_MAX]        = "No errors",	
};

static const char *lt_polymorph_type_names[] = {
	[ZTABLE_NON] = "uninitialized", 
	[ZTABLE_INT] = "integer",     
	[ZTABLE_FLT] = "float",     
	[ZTABLE_TXT] = "text",     
	[ZTABLE_BLB] = "blob",     
#ifdef ZTABLE_NUL
	[ZTABLE_NUL] = "null",     
#endif
	[ZTABLE_USR] = "userdata",     
	[ZTABLE_TBL] = "table",     
	[ZTABLE_TRM] = "terminator",     
	[ZTABLE_NOD] = "node",     
};

static const zhRecord nul = { 0 };

static const zhRecord *zt_nul = &nul;

static const int lt_maxbuf = 64;

static const int lt_buflen = 4096;

static const int lt_max_slots = LT_MAX_HASH;

#ifdef DEBUG_H
 static const char *fmt = "%-4s\t%-10s\t%-5s\t%-10s\t%-30s\t%-6s\t%-20s\n";
#endif


struct zh_iterator { 
	int len, depth;
	void *userdata; 
	zTable *source;
};



static int lt_hashu (unsigned char *ustr, int len, int size) {
	unsigned int hash = lt_hash;
	for ( int i = 0; i < len; i++ ) hash += ( ( hash * 31 ) + hash ) + ustr[i];
	return hash % size;
}


//Build a string or some other index in reverse
static int build_backwards (zKeyval *t, unsigned char *buf, int bs) { 
	//This should return if there is no value...
	int size = 0, mm = bs;
	zKeyval *p =  t;

	while ( p ) {
		//This should only run if there is a blob or pKey	
		if ( p->key.type == ZTABLE_INT || p->key.type == ZTABLE_FLT ) {
			char b[128] = {0};
			double f = (t->key.type == ZTABLE_FLT) ? p->key.v.vfloat : (double)p->key.v.vint;
			int a =	snprintf( b, 127, (t->key.type == ZTABLE_FLT) ? "%f" : "%.0f", f);
			mm -= strlen(b);
			memcpy( &buf[ mm ], b, a );
			buf[ --mm ] = '.';
		}
		else if ( p->key.type == ZTABLE_BLB ) {
			zhBlob *b = &p->key.v.vblob;
			mm -= b->size;
			memcpy( &buf[ mm ], b->blob, b->size );
			buf[ --mm ] = '.';
		}
		else if ( p->key.type == ZTABLE_TXT ) {
			char *b = p->key.v.vchar;
			mm -= strlen(b);
			memcpy( &buf[ mm ], b, strlen(b));
			buf[ --mm ] = '.';
		}
		else {
			return -1;
		}
		p = p->parent;
	}

	//Copy and clean
	size = bs - (++mm);
	memmove( &buf[ 0 ], &buf[ mm ], size );
	memset( &buf[size], 0, bs - size );
	buf[size] = '\0';
	return size;
}


//Get full string (using build_backwards) 
unsigned char *lt_get_full_key ( zTable *t, int hash, unsigned char *buf, int bs ) {
	zKeyval *kv = lt_retkv( t, hash );	
	//unsigned char tmp[ 2048 ] = { 0 };
	if ( kv ) {
		build_backwards( kv, buf, bs - 1 );
		return buf;
	}
	return NULL;
}


//Trim things
unsigned char *lt_trim ( unsigned char *msg, char *trim, int len, int *nlen ) {
	//Define stuff
	unsigned char *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


//Count indices in a table. If index is greater than 1 and the item is a "table", then will return the number of elements in said table
int lt_count_at_index ( zTable *t, int index, int type ) {
	//Define
	zhTable *tt = NULL;

	//Return count of all elements
	if ( index == -1 )
		return t->count;
	else {
		//Return one for elements that exist, but aren't tables
		if ( lt_vta( t, index ) != ZTABLE_TBL )
			return 1;
		else {
			return ( tt = &lt_table_at( t, index ) ) ? (tt->count - type) : 0;
		}
	}
	return 0;
}


int lt_countall( zTable *t ) {
	return t->count + 1;
}


//Initiailizes a table data structure
zTable *lt_init ( zTable *t, zKeyval *k, int size ) {
	//Define
	int actual_size = size;

	//Calculate optimal modulus for hashing
	if ( size <= 63 )
		t->modulo = 63; 
	else if ( size <= 127 )
		t->modulo = 127; 
	else if ( size <= 511 )
		t->modulo = 511; 
	else if ( size <= 1027 )
		t->modulo = 1027;
	else if ( size <= 2047 )
		t->modulo = 2047; 
	else if ( size <= 4091 )
		t->modulo = 4091; 
	else if ( size <= 8191 )
		t->modulo = 8191; 
	else if ( size <= 16383)
		t->modulo = 16383; 
	else if ( size <= 32767)
		t->modulo = 32767; 
	else {
		t->modulo = 65535; 
	}

	t->mallocd = (!k) ? 1 : 0;

	//Allocate space for users that don't pass in their own structure 
	if ( !k ) {
		actual_size = t->modulo;
		if ( !( k = malloc( t->modulo * sizeof(zKeyval) ) ) ) {
			t->error = ZTABLE_ERR_LT_ALLOCATE;
			return 0;
		}
	}

	if ( !memset( (void *)k, 0, sizeof(zKeyval) * actual_size ) ) {
		t->error = ZTABLE_ERR_LT_ALLOCATE;
		return 0;
	}

	//Initialize all hash entries to -1
	for ( int i=0; i < actual_size; i++ ) {
		memset( k[i].hash, -1, sizeof(int) * lt_max_slots );
	}

	//Set this
	t->current = NULL, t->src = NULL;
	t->srcmallocd = 0;
	t->error = 0;
	t->cptr = -1;
	t->total = actual_size;
	t->count = 0, t->index = 0;
	t->head = k;
	t->start = 0;
	t->end = 0;
	t->rCount = &t->count;
	return t;
}



//Adds a value to a table data structure
zhType lt_add ( zTable *t, int side, zhType lt, int vi, float vf,
	char *vc, unsigned char *vb, unsigned int vblen, void *vn, zTable *vt, char *trim )
{

	if ( t->index >= t->total ) {
		t->error = ZTABLE_ERR_LT_OUT_OF_SPACE;
		return 0;
	}

	//...		
	zhValue *v = (!side) ? &(t->head + t->index)->key : &(t->head + t->index)->value;
	zhRecord *r = &v->v;
	v->type = lt;

	//Check for zero length blobs or text
	if ( ( lt == ZTABLE_BLB || lt == ZTABLE_TXT ) && !vblen ) {
		return 0;
	}

	//Set each value to its matching type
	if ( lt == ZTABLE_INT )
		r->vint = vi;
	else if ( lt == ZTABLE_FLT )
		r->vfloat = vf;
	else if ( lt == ZTABLE_USR )
		r->vusrdata = vn;
	else if ( lt == ZTABLE_TBL )
		return ( t->error = ZTABLE_ERR_LT_INVALID_VALUE ) ? -1 : -1;
	else if ( lt == ZTABLE_BLB )
		r->vblob.blob = vb, r->vblob.size = vblen;
#ifdef ZTABLE_NUL
	else if ( lt == ZTABLE_NUL )
		r->vnull = NULL;
#endif
	else if ( lt == ZTABLE_TXT ) {
		if ( !( r->vchar = malloc( vblen + 1 ) ) )
			return 0;
		else {
			memset( r->vchar, 0, vblen + 1 );
			memcpy( r->vchar, vb, vblen );
			r->vchar[ vblen ] = '\0';
		}
	}
	else {
		return 0;
	}
	return lt;
}



//Return types
zhType lt_rettype( zTable *t, int side, int index ) {
	if ( index < 0 || index > t->count ) {
		return ( t->error = ZTABLE_ERR_LT_INVALID_INDEX ) ? 0 : 0;
	}
	return (!side) ? (t->head + index)->key.type : (t->head + index)->value.type; 
}



//Return typenames
const char *lt_rettypename( zTable *t, int side, int index ) {
	if ( index < 0 || index > t->count ) {
		t->error = ZTABLE_ERR_LT_INVALID_INDEX; 
		return lt_polymorph_type_names[ 0 ];
	}
	zhType i = (!side) ? (t->head + index)->key.type : (t->head + index)->value.type;
	return lt_polymorph_type_names[ (int) i ];
}


const char *lt_typename ( int type ) {
	return ( type > -1 && type <= ZTABLE_NOD ) ? lt_polymorph_type_names[ type ] : NULL;
}


//Move left or right within the hierarchy of tables
int lt_move ( zTable *t, int dir ) {
	//Out of space
	if ( t->index > t->total ) {
		t->error = ZTABLE_ERR_LT_OUT_OF_SPACE;
		return -1;
	}

	zKeyval *curr = (t->head + t->index);
	zhValue *value = &curr->value;

	//Left or right?	
	if ( !dir ) {
		//Set count of elements in this new table to actual count
		zhTable *T = &value->v.vtable;
		value->type = ZTABLE_TBL;
		t->rCount = &T->count;
		T->parent = ( !t->current ) ? NULL : t->current;
		T->ptr = *(long *)&T; 
		t->current = T;
	}
	else {
		//Set references
		zhValue *key = &curr->key; 
		key->type = ZTABLE_TRM;
		value->type = ZTABLE_NUL;
		zhRecord *r = &key->v;	

		//....
		if ( !t->current->parent ) {
			t->rCount = &t->count;
			r->vptr = (long)t->current->ptr;
			t->current = NULL;
		}
		else if ( t->current->parent ) {
			r->vptr = (long)t->current->ptr;
			zhTable *T = t->current->parent;
			t->rCount = &T->count;
			t->current = T;
		}
	}
	lt_finalize( t );
	return 1;
}



//Finalize adding to both sides of a table data structure
void lt_finalize ( zTable *t ) {
	//if these are equal, don't increment both *t->rCount and t->count
	( t->rCount == &t->count ) ? 0 : ( *t->rCount )++ ; 
	t->count ++;
	t->index ++;
}



//Hash each key
void lt_lock ( zTable *t ) {
	zKeyval *parent = NULL;

	for ( int i=0; i <= t->index; i++ ) {
		//Get reference
		zKeyval *tt = t->head + i;
		zKeyval *slot = NULL;
		int pp = 0;
    int h = 0;

		//Make a new buffer
		unsigned char buf[ LT_POLYMORPH_BUFLEN ];
		memset( buf, 0, LT_POLYMORPH_BUFLEN );

		//Check keys and values...
		if (tt->value.type == ZTABLE_NUL ) {
			if ( parent ) {
				parent = parent->parent;
			}
			continue;
		}

		//Set parent of an item.
		if ( parent )	{
			tt->parent = parent; 
		}
	
		//Do parents here
		if ( tt->value.type == ZTABLE_TBL ) {
			parent = tt;
		}

		//Build a string to hash, save the hash somewhere 
		pp = build_backwards( tt, buf, LT_POLYMORPH_BUFLEN );
		h = lt_hashu( buf, pp, t->modulo );
		slot = t->head + h;

		//Find an available slot
		for ( int m=0, j=0; !j && m < lt_max_slots; m++ ) {
			slot->hash[m] == -1 ?	slot->hash[ m ] = i, j=1 : 0;
		}
	}
}




//Return index in table where key was found
int lt_get_long_i ( zTable *t, unsigned char *find, int len ) {
	zKeyval *hv = NULL;
	int hash = 0, hh = 0;
	unsigned char *f = NULL, gb[ LT_POLYMORPH_BUFLEN ] = { 0 };

	if ( len > LT_POLYMORPH_BUFLEN || len < 0 ) { 
		return -1;
	}

	if ( !t->start && !t->end ) 
		hash = lt_hashu( f = find, len, t->modulo );
	else {
		//Ewww..
		int a = 0;
		memcpy( &gb[0], t->buf, t->buflen );
		if ( *find != '.' ) {
			a = 1;
			memcpy( &gb[ t->buflen ], ".", 1 ); 
		}	
		memcpy( &gb[ t->buflen + a ], find, len );
		hash = lt_hashu( gb, t->buflen + a + len, t->modulo );
		f = gb;
	}

	//Find the key
	for ( int i=0 ; !hv && i < 5; i++ ) {
		unsigned char buf[LT_POLYMORPH_BUFLEN] = {0};

		if ( (hh = (t->head + hash)->hash[i]) == -1 || i == lt_max_slots ) {
			return -1;
		}

		if ( !(hv = t->head + hh) ) {
			return -1;
		}
	
		//?	
		if ( build_backwards( hv, buf, LT_POLYMORPH_BUFLEN ) == -1 ) {
			return -1;
		}

		hv = ( memcmp(f, buf, len) == 0) ? hv : 0;
	}

	//Pull the value if it's in the acceptable range
	if ( !t->start && !t->end )
		return hh;
	else {
		return ( hh > t->start && hh < t->end ) ? hh : -1;	
	}
}


//Return zKeyval at certain index
zhValue *lt_retany ( zTable *t, int index ) {
	return ( index <= -1 || index > t->count ) ? NULL : &(t->head + index)->value; 
}


int lt_exists (zTable *t, int index) {
	return ( index <= -1 || index > t->count );
}


//Return a zKeyval at a certain index
zKeyval *lt_retkv ( zTable *t, int index ) {
	if ( index <= -1 || index > t->count ) {
		t->error = ZTABLE_ERR_LT_INVALID_INDEX;
		return NULL;
	}

	return (t->head + index); 
}


//Return a zhRecord matching a certain type at a certain index
zhRecord *lt_ret ( zTable *t, zhType type, int index ) {
	if ( index <= -1 || index > t->count ) {
		t->error = ZTABLE_ERR_LT_INVALID_INDEX;
		return (zhRecord *)zt_nul; 
	}

	if ( (t->head + index)->value.type != type ) { 
		return (zhRecord *)zt_nul; 
	}

	return &(t->head + index)->value.v; 
}


//Set the index to another one (absolutely)
int lt_absset( zTable *t, int index ) {
	//Can't return less than 0
	if ( index < 0 || index > t->count ) {
		return 0;
	}

	//Rewind by the current index then increment by the new index	
	t->current -= t->index;
	t->index = index;
	t->current += t->index ;	
	t->cptr = (t->head + t->index)->value.v.vtable.ptr;
	return 1;
}


//Set the current index to another one
int lt_set ( zTable *t, int index ) {
	int j = 0;	
	if ( index < 0 )
		j = (( t->index + index ) < 0 ) ? -1 : (t->index += index ); 
	else {
		j = (( t->index + index ) > t->count ) ? -1 : (t->index += index ); 
	}

	if ( j == -1 )
		return 0;
	else {
		t->current += index;	
		return 1;
	}
}


//Reset a table index
void lt_reset ( zTable *t ) {
	t->start = 0, t->end = 0, t->index = 0;
}


//Iterate through the indices of a table
zKeyval *lt_next ( zTable *t ) {
	zKeyval *curr = (t->index > t->count) ? NULL : t->head + t->index;
	t->index++;
	return curr;
}


zKeyval *lt_current ( zTable *t ) {
	return ( t->index > t->count ) ? NULL : t->head + t->index;
}


//Loop from another point...
zKeyval *lt_items_by_index ( zTable *t, int ind ) {
	//Find a hash, and if it's a table... set some stuff
	zKeyval *curr = NULL;

	if ( t->cptr == -1 ) {
		if ( (t->head + ind)->value.type != ZTABLE_TBL ) {
			return NULL;
		}
		t->index = ind;
		t->cptr = (t->head + ind)->value.v.vtable.ptr;
	}

	//
	if ( t->index > t->count ) {
		return NULL;
	}

	//Set reference
	curr = t->head + t->index;

	//Check the key name and see if it matches t->cptr, return null if so
	if ( curr->key.type == ZTABLE_TRM && curr->key.v.vptr == t->cptr ) {
		t->index = 0, t->cptr = -1;
		return NULL;
	}

	//Increment and move on
	t->index++;
	return curr;
}



//Find a table by hash and return until it has no more keys.
zKeyval *lt_items_i ( zTable *t, unsigned char *src, int len ) {
	//Find a hash, and if it's a table... set some stuff
	zKeyval *curr = NULL;
	int in;

	//Check for a hash table
	if ( t->cptr == -1 ) {
		if ( (in = lt_get_long_i ( t, src, len )) == -1 ) {
			return NULL;
		}

		if ( (t->head + in)->value.type != ZTABLE_TBL ) {
			return NULL;
		}
		
		t->index = in;
		t->cptr = (t->head + in)->value.v.vtable.ptr;
	}

	if (t->index > t->count) {
		return NULL;
	}

	//Set reference
	curr = t->head + t->index;

	//Check the key name and see if it matches t->cptr, return null if so
	if ( curr->key.type == ZTABLE_TRM && curr->key.v.vptr == t->cptr ) {
		t->index = 0, t->cptr = -1;
		return NULL;
	}

	//Increment and move on
	t->index++;
	return curr;
} 



//Set a data source
void lt_setsrc ( zTable *t, void *src ) {
	t->src = src;
}


//Will set boundaries on a new table
zTable *lt_within_long( zTable *t, unsigned char *src, int len ) {
	//Whenever we look for a string, we copy til the end
	int a = 0;
	t->buf = src;
	t->buflen = len;

	//Search for a table	
	if ( (a = lt_get_long_i(t, src, len)) == -1 || lt_vta(t, a) != ZTABLE_TBL ) {
		return NULL;
	}

	//Set start and end, then return the table
	t->start = a, t->end = a + (&lt_table_at( t, a ))->count;
	return t;
}


void lt_unset ( zTable *t ) {
	if ( t->buf ) {
		free( t->buf );
		t->buf = NULL;
	}
}



//Get a key or value somewhere
void lt_free ( zTable *t ) {	
	//Free any text keys
	for ( int ii=0; ii < t->count; ii++ ) {
		zKeyval *k = t->head + ii;
		( k->key.type == ZTABLE_TXT ) ? free( k->key.v.vchar ), k->key.v.vchar = NULL : 0;
		( k->value.type == ZTABLE_TXT ) ? free( k->value.v.vchar ), k->value.v.vchar = NULL : 0;
	}

	if ( t->mallocd ) {
		//TODO: Why not just memset to zero?
		free( t->head );
		t->head = NULL, t->rCount = NULL;
		t->error = 0, t->total = 0, t->count = 0, t->index = 0;
		t->mallocd= 0;
	}

	if ( t->srcmallocd /*t->src*/ ) {
		free( t->src );
		t->src = NULL;
	}
}



//An iterator providing more control
int lt_exec_complex (zTable *t, int start, int end, void *p, int (*fp)( zKeyval *kv, int i, void *p ) ) {
	//Bounds violations should stop.
	if ( start < 0 || start > t->count || end < 0 || end > t->count ) {
		return 0;
	}

	for ( int i = start; i < end; ++i ) {
		if ( !fp( (t->head + i ), i, p ) ) return 0;
	}
	return 1;
}



//Copy iterator
static int copy_iterator( zKeyval *kv, int i, void *p ) {
	struct zh_iterator *f = (struct zh_iterator *)p; 
	zTable **t = (zTable **)f->userdata;

	//decrease depth
	if ( kv->key.type == ZTABLE_TRM ) {
		if ( --f->depth == 0 ) {
			lt_ascend( *t );
			//lt_finalize( *t );
			return 0;
		}
	}

	//increase depth
	if ( kv->value.type == ZTABLE_TBL ) {
		f->depth ++;
	}

	if ( kv->key.type == ZTABLE_INT )
		lt_addintkey( *t, kv->key.v.vint );
	else if ( kv->key.type == ZTABLE_TXT )
		lt_addtextkey( *t, kv->key.v.vchar );
	else if ( kv->key.type == ZTABLE_BLB ) 
		lt_addblobkey( *t, kv->key.v.vblob.blob, kv->key.v.vblob.size );
	else if ( kv->key.type == ZTABLE_TRM ) {
		lt_ascend( *t );
		//lt_finalize( *t );
		return 1;
	}

	if ( kv->value.type == ZTABLE_INT )
		lt_addintvalue( *t, kv->value.v.vint );
	else if ( kv->value.type == ZTABLE_BLB ) 
		lt_addblobvalue( *t, kv->value.v.vblob.blob, kv->value.v.vblob.size );
	else if ( kv->value.type == ZTABLE_FLT )
		lt_addfloatvalue( *t, kv->value.v.vfloat );	
	else if ( kv->value.type == ZTABLE_USR )
		lt_addudvalue( *t, kv->value.v.vusrdata );
	else if ( kv->value.type == ZTABLE_TXT ) {
		char * v = !kv->value.v.vchar ? "" : kv->value.v.vchar;
		lt_addtextvalue( *t, v );
	}
	else if ( kv->value.type == ZTABLE_TBL ) {
		lt_descend( *t );
		return 1;
	}

	lt_finalize( *t );
	return 1;
}



//Deep copy
zTable *lt_deep_copy ( zTable *t, int start, int end ) {
	zTable *nt = NULL;
	struct zh_iterator zd = { 0 };

	if ( start == -1 ) {
		t->error = 0; // INVALID INDEX
		return NULL;
	}

	//Finally, fp->depth should be zero when done, but starting at one may save time
	if ( !( nt = malloc( sizeof ( zTable ) ) ) || !lt_init( nt, NULL, t->modulo ) ) {
		t->error = 0; // MEMORY ALLOCATION ERROR
		return NULL;
	}

	//Save data
	zd.len = end - start, zd.depth = 0;
	zd.userdata = &nt, zd.source = t;	

	if ( !lt_exec_complex( t, start, t->count, &zd, copy_iterator ) ) {
		lt_reset( t );
		lt_lock( nt );
		return nt; 
	}

	lt_reset( t );
	lt_lock( nt );
	return nt;
}


#ifdef ZTABLE_ERR_EXP 
//Clear error
void lt_clearerror ( zTable *t ) {
	t->error = 0;
}


//Return errors as strings
const char *lt_strerror ( zTable *t ) {
	return ( t->error > -1 && t->error < ZTABLE_ERR_LT_INDEX_MAX)
		? lt_errors[ (int)t->error ] : NULL; 
}
#endif


#ifdef DEBUG_H 
//Print out an initialized table
void lt_printt ( zTable *t ) {
	fprintf( stderr, "t->total:      %d\n", t->total );
	fprintf( stderr, "t->modulo:     %d\n", t->modulo );
	fprintf( stderr, "t->index:      %d\n", t->index );
	fprintf( stderr, "t->count:      %d\n", t->count );
	fprintf( stderr, "t->rCount:     %p\n", (void *)t->rCount );
	fprintf( stderr, "t->head:       %p\n", (void *)t->head );
}



//Get a key or value somewhere
void lt_printall ( zTable *t ) {
	//Header
	fprintf( stderr, fmt, "Index", "KType", "VType", "Value", "CombinedValue", "HashOf", "Hashes" );

	for ( int ii=0; ii < t->index; ii++ ) {
		zKeyval *k = t->head + ii;
		zhType kt;
		int hash;	
		const char *kk=NULL, *vv=NULL;
		char bkbuf[1024] = {0}, 
         strbuf[512] = {0},
				 inbuf[11] = {0},
				 habuf[11] = {0},
         nmbuf[125] = {0};

		//Index
		sprintf(inbuf, "%d", ii);
		
		//Hashes
		for ( int i=0, j=0; i < lt_max_slots ; i++ )
			j += sprintf( &nmbuf[ j ], "%3d,",  k->hash[ i ] );

		//Key and value types
		kk = lt_rettypename( t, 0, ii );
		vv = lt_rettypename( t, 1, ii );

		//Finally, the key itself (the whole thing, I suppose)
		if ((kt = lt_rettype( t, 0, ii )) == ZTABLE_INT )
			sprintf( strbuf, "%d, ", (t->head + ii )->key.v.vint );
		else if ( kt == ZTABLE_TXT )
			sprintf( strbuf, "%s, ", (t->head + ii )->key.v.vchar );
		else if ( kt == ZTABLE_BLB ) 
		{
			int size = (t->head + ii )->key.v.vblob.size;
			if ( size > 1024 ) 
				sprintf( strbuf, "%s, ", "Blob too large" );
			else {
				memcpy( strbuf, (t->head + ii )->key.v.vblob.blob, size );
				strbuf[ size ] = '\0';
			}
		}
	
		//Build a string backwards
		build_backwards( t->head + ii, (unsigned char *)bkbuf, 1024 );
		hash = lt_hashu( (unsigned char *)bkbuf, strlen(bkbuf), t->modulo );
		sprintf( habuf, "%d", ( kt == ZTABLE_NON ) ? -1 : hash );
		fprintf( stderr, fmt, inbuf, kk, vv, strbuf, bkbuf, habuf, nmbuf );
	}
}


//
void print_key( zKeyval *kv ) {
	if ( kv->key.type == ZTABLE_INT )
		fprintf( stderr, "%d\n", kv->key.v.vint );
	else if ( kv->key.type == ZTABLE_TXT )
		fprintf( stderr, "%s\n", kv->key.v.vchar );
	else if ( kv->key.type == ZTABLE_BLB ) 
		write( 2, kv->key.v.vblob.blob, kv->key.v.vblob.size );
	else if ( kv->key.type == ZTABLE_TRM ) {
		0;
	}
}

void print_value( zKeyval *kv ) {
	if ( kv->value.type == ZTABLE_INT )
		fprintf( stderr, "%d\n", kv->value.v.vint );
	else if ( kv->value.type == ZTABLE_TXT )
		fprintf( stderr, "%s\n", kv->value.v.vchar );
	else if ( kv->value.type == ZTABLE_BLB ) 
		write( 2, kv->value.v.vblob.blob, kv->value.v.vblob.size );
	else if ( kv->value.type == ZTABLE_FLT )
		fprintf( stderr, "%f\n", kv->value.v.vfloat );
	else if ( kv->value.type == ZTABLE_USR )
		fprintf( stderr, "%p\n", kv->value.v.vusrdata );
	else if ( kv->value.type == ZTABLE_TBL ) {
		0;	
	}
}


//Print a set of values at a particular index
static void lt_printindex ( zKeyval *tt, int device, int showkey, int ind ) {
	int w = 0;
	int maxlen = (showkey) ? 24576 : lt_buflen;
  char b[maxlen]; 
	memset(b, 0, maxlen);
	struct { int t; zhRecord *r; } items[2] = {
		{ tt->key.type  , &tt->key.v    },
		{ tt->value.type, &tt->value.v  } 
	};

	for ( int i=0; i<2; i++ ) {
		zhRecord *r = items[i].r; 
		int t = items[i].t;
		if ( i ) {
			memcpy( &b[w], " -> ", 4 );
			w += 4;
			/*ZTABLE_NODE is handled in printall*/
			if (t == ZTABLE_NON)
				w += snprintf( &b[w], maxlen - w, "%s", "is uninitialized" );
		#ifdef ZTABLE_NUL
			else if (t == ZTABLE_NUL)
				w += snprintf( &b[w], maxlen - w, "is terminator" );
		#endif
			else if (t == ZTABLE_USR)
				w += snprintf( &b[w], maxlen - w, "userdata [address: %p]", r->vusrdata );
			else if (t == ZTABLE_TBL) {
				zhTable *rt = &r->vtable;
				w += snprintf( &b[w], maxlen - w, 
					"table [address: %p, ptr: %ld, elements: %d]", (void *)rt, rt->ptr, rt->count );
			}
		}

		//TODO: This just got ugly.  Combine the different situations better...
		if ( !i && showkey ) { 
			if ( t == ZTABLE_TRM )
				w += snprintf( &b[w], maxlen - w, "%ld", r->vptr );
			else if ( t == ZTABLE_NON || t == ZTABLE_NUL )
				w += snprintf( &b[w], maxlen - w, "(null)" );
			else {
				w += build_backwards( tt, (unsigned char *)b, maxlen );
			}
		}
		else {
			//I want to see the full key
			if (t == ZTABLE_FLT || t == ZTABLE_INT)
				w += snprintf( &b[w], maxlen - w, "%d", r->vint );
			else if (t == ZTABLE_FLT)
				w += snprintf( &b[w], maxlen - w, "%f", r->vfloat );
			else if (t == ZTABLE_TXT)
				w += snprintf( &b[w], maxlen - w, "%s", r->vchar );
			else if (t == ZTABLE_TRM)
				w += snprintf( &b[w], maxlen - w, "%ld", r->vptr );
			else if (t == ZTABLE_BLB) {
				zhBlob *bb = &r->vblob;
				if ( bb->size < 0 )
					return;	
				if ( bb->size > lt_maxbuf )
					w += snprintf( &b[w], maxlen - w, "is blob (%d bytes)", bb->size);
				else {
					memcpy( &b[w], bb->blob, bb->size ); 
					w += bb->size;
				}
			}
		}
	}

	write( device, b, w );
	write( device, "\n", 1 );
}	


//Dump a table (needs some flags for debugging) 
int __lt_dump ( zKeyval *kv, int i, void *p ) {
	zhType vt = kv->value.type;
	zhInner *pp = (zhInner *)p; 
	if ( pp->indextype ) {
		char buf[ 128 ] = { 0 };
		const char *space = &__lt_ws[ 100 - pp->level ];
		int l = snprintf( buf, sizeof(buf), __lt_fmt, i, pp->level, space ); 
		write( pp->fd, buf, l );
	}
	lt_printindex( kv, pp->fd, pp->dumptype, pp->level );
	pp->level += (vt == ZTABLE_NUL) ? -1 : (vt == ZTABLE_TBL) ? 1 : 0;
	return 1;
}
#endif

