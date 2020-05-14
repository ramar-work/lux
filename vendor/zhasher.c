/* ------------------------------------------- * 
 * zhasher.c
 * ---------
 * A hash table library written in C.
 *
 * Usage
 * -----
 *
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * TODO
 * ----
 * 
 * ------------------------------------------- */
#include "zhasher.h"

//unsigned int __lt_int = 0;
static const unsigned int lt_hash = 31;
LtInner __ltComplex = { LT_DUMP_LONG, LT_VERBOSE, NULL, 0 };
LtInner __ltHistoric = { LT_DUMP_SHORT, LT_VERBOSE, NULL, 0 };
LtInner __ltSimple = { LT_DUMP_SHORT, LT_CONDENSED, NULL, 0 };

static const char *errors[] = {
	[ZHASHER_ERR_LT_ALLOCATE]         = "Failed to allocate space for Table",
	[ZHASHER_ERR_LT_OUT_OF_SPACE]     = "Out of space",
	[ZHASHER_ERR_LT_INVALID_VALUE]    = "Attempted to add invalid value.",
	[ZHASHER_ERR_LT_INVALID_TYPE]     = "Invalid type requested.",
	[ZHASHER_ERR_LT_INVALID_INDEX]    = "Attempted to access uninitialized index.",
	[ZHASHER_ERR_LT_OUT_OF_SLICE]     = "Value is out of requested range",	
	[ZHASHER_ERR_LT_INDEX_MAX]        = "No errors",	
};

static const char *lt_polymorph_type_names[] = {
	[LITE_NON] = "uninitialized", 
	[LITE_INT] = "integer",     
	[LITE_FLT] = "float",     
	[LITE_TXT] = "text",     
	[LITE_BLB] = "blob",     
#ifdef LITE_NUL
	[LITE_NUL] = "null",     
#endif
	[LITE_USR] = "userdata",     
	[LITE_TBL] = "table",     
	[LITE_TRM] = "terminator",     
	[LITE_NOD] = "node",     
};

static const LiteRecord nul = { 0 };
static const LiteRecord *supernul = &nul;
static const int lt_maxbuf = 64;
static const int lt_buflen = 4096;
#ifdef LT_MAX_HASH
 static const int lt_max_slots     = LT_MAX_HASH;
#else
 static const int lt_max_slots     = 7;
#endif
#ifdef DEBUG_H
 static const char *fmt = "%-4s\t%-10s\t%-5s\t%-10s\t%-30s\t%-6s\t%-20s\n";
#endif


static int lt_hashu (unsigned char *ustr, int len, int size) {
	unsigned int hash=lt_hash;
	for (int i=0; i<len; i++) hash += ((hash*31) + hash) + ustr[i];
	return hash % size;
}


//Build a string or some other index in reverse
static int build_backwards (LiteKv *t, unsigned char *buf, int bs) { 
	//This should return if there is no value...
	int size =  0, 
      mm = bs;
	LiteKv *p =  t; 

	while (p) {
		//This should only run if there is a blob or pKey	
		if (p->key.type == LITE_INT || p->key.type == LITE_FLT) {
			char b[128] = {0};
			double f = (t->key.type == LITE_FLT) ? p->key.v.vfloat : (double)p->key.v.vint;
			int a =	snprintf( b, 127, (t->key.type == LITE_FLT) ? "%f" : "%.0f", f);
			mm -= strlen(b);
			memcpy( &buf[ mm ], b, a );
			buf[ --mm ] = '.';
		}
		else if (p->key.type == LITE_BLB) {
			LiteBlob *b = &p->key.v.vblob;
			mm -= b->size;
			memcpy( &buf[ mm ], b->blob, b->size );
			buf[ --mm ] = '.';
		}
		else if (p->key.type == LITE_TXT) {
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
unsigned char *lt_get_full_key ( Table *t, int hash, unsigned char *buf, int bs ) {
	LiteKv *kv = lt_retkv( t, hash );	
	//unsigned char tmp[ 2048 ] = { 0 };
	if ( kv ) {
		build_backwards( kv, buf, bs - 1 );
		return buf;
	}
	return NULL;
}


//Trim things
unsigned char *lt_trim (uint8_t *msg, char *trim, int len, int *nlen) {
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}


//Count indices in a table. If index is greater than 1 and the item is a "table", then will return the number of elements in said table
int lt_count_at_index ( Table *t, int index, int type ) {
	//Return count of all elements
	if ( index == -1 )
		return t->count;
	else if ( index == 0 ) {
		//NOTE: If the first index is not a table, there will always only be one value on the other side.
		if ( lt_vta( t, index ) != LITE_TBL )
			return 1; //t->count;
		else {
			LiteTable *tt = &lt_table_at( t, index );
			return ( tt ) ? (tt->count - type) : 0;
		}
	}
	else {
		//Return one for elements that exist, but aren't tables
		if ( lt_vta( t, index ) != LITE_TBL )
			return 1;
		else {
			LiteTable *tt = &lt_table_at( t, index );
			return ( tt ) ? (tt->count - type) : 0;
		}
	}
	return 0;
}


int lt_countall( Table *t ) {
	return t->count + 1;
}


//Clear error
void lt_clearerror (Table *t) 
{
	t->error = 0;
}


//Return errors as strings
const char *lt_strerror (Table *t) {
	//Paranoid bounds checking
	return ( t->error > -1 && t->error < ZHASHER_ERR_LT_INDEX_MAX) ? errors[ t->error ] : NULL; 
}


//Initiailizes a table data structure
Table *lt_init (Table *t, LiteKv *k, int size) {
	SHOWDATA( "Working with root table %p\n", t );

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
	else if ( size <= 65535)
		t->modulo = 65535; 
	else if ( size <= 65535)
		t->modulo = 131067; 
	else if ( size <= 131067)
		t->modulo = 199999; 
	else if ( size <= 199961 )
		t->modulo = 199999; 
	else {
		t->modulo = 511997; 
	}

	int actual_size = size;
	t->mallocd = (!k) ? 1 : 0;

	//Allocate space for users that don't pass in their own structure 
	if ( !k ) {
		actual_size = t->modulo;
		k = malloc(t->modulo * sizeof(LiteKv));
		if ( !k ) {	
			t->error = ZHASHER_ERR_LT_ALLOCATE;
			return 0;
		}
	}

	if ( memset((void *)k, 0, sizeof(LiteKv) * actual_size) == NULL ) {
		t->error = ZHASHER_ERR_LT_ALLOCATE;
		return 0;
	}

	//Initialize all hash entries to -1
	for ( int i=0; i < actual_size; i++ ) {
		memset( k[i].hash, -1, sizeof(int) * lt_max_slots );
	}

	//Set this
	t->current = NULL;
	t->src     = NULL;
	t->srcmallocd = 0;
	t->error   = 0;
	t->cptr    = -1;
	t->total   = actual_size;
	t->count   = 0;
	t->head    = k;
	t->index   = 0;
	t->start   = 0;
	t->end     = 0;
	t->rCount  = &t->count;
	//t->parent = NULL;

	//Lastly, increment tte pointer by one so we can always follow it
	return t;
}



//Adds a value to a table data structure
LiteType lt_add ( Table *t, int side, LiteType lt, int vi, float vf,
	char *vc, unsigned char *vb, unsigned int vblen, void *vn, Table *vt, char *trim )
{

	if ( t->index >= t->total ) {
		t->error = ZHASHER_ERR_LT_OUT_OF_SPACE;
		return 0;
	}

	//...		
	LiteValue  *v = (!side) ? &(t->head + t->index)->key : &(t->head + t->index)->value;
	LiteRecord *r = &v->v;
	v->type       = lt;

	//Check for zero length blobs or text
	if ( ( lt == LITE_BLB || lt == LITE_TXT ) && !vblen )
		return 0;

	//Set each value to its matching type
	if ( lt == LITE_INT ) {
		r->vint = vi;
		SHOWDATA( "Adding int %s %d to table at %p", ( !side ) ? "key" : "value", r->vint, ( void * )t );
	}
	else if ( lt == LITE_FLT ) {
		r->vfloat = vf;
		SHOWDATA( "Adding float %s %f to table at %p", ( !side ) ? "key" : "value", r->vfloat, ( void * )t );
	}
#ifdef LITE_NUL
	else if ( lt == LITE_NUL ) {
		r->vnull = NULL;
		SHOWDATA( "Adding null %s to table at %p", ( !side ) ? "key" : "value", ( void * )t );
	}
#endif
	else if ( lt == LITE_USR ) {
		r->vusrdata = vn;
		SHOWDATA( "Adding userdata %p to table at %p", ( void * )r->vusrdata, ( void * )t );
	}
	else if ( lt == LITE_TBL ) {
		SHOWDATA( "Adding invalid value table!" );
		return ( t->error = ZHASHER_ERR_LT_INVALID_VALUE ) ? -1 : -1;
	}
	else if ( lt == LITE_BLB ) {
		r->vblob.blob = vb, r->vblob.size = vblen;
		SHOWDATA( "Adding blob %s of length %d to table at %p", (!side) ? "key" : "value", r->vblob.size, ( void * )t );
	}
	else if ( lt == LITE_TXT ) {
		//Even though this says LITE_TXT, the assumption 
		//is that tab needs to duplicate the data. 
		r->vchar = malloc( vblen + 1 );
		if ( !r->vchar )
			return 0;
		else {
			memset( r->vchar, 0, vblen + 1 );
			memcpy( r->vchar, vb, vblen );
			r->vchar[ vblen ] = '\0';
			SHOWDATA( "Adding text %s '%s' to table at %p", ( !side ) ? "key" : "value", r->vchar, ( void * )t );
		}
	}
	else {
		SHOWDATA( "Attempted to add unknown %s type to table at %p", ( !side ) ? "key" : "value", ( void * )t );
		return 0;
	}
	return lt;
}



//Return types
LiteType lt_rettype( Table *t, int side, int index ) {
	if ( index < 0 || index > t->count ) {
		return ( t->error = ZHASHER_ERR_LT_INVALID_INDEX ) ? 0 : 0;
	}
	return (!side) ? (t->head + index)->key.type : (t->head + index)->value.type; 
}



//Return typenames
const char *lt_rettypename( Table *t, int side, int index ) {
	if ( index < 0 || index > t->count ) 
	{
		t->error = ZHASHER_ERR_LT_INVALID_INDEX; 
		return lt_polymorph_type_names[ 0 ];
	}
	LiteType i = (!side) ? (t->head + index)->key.type : (t->head + index)->value.type;
	return lt_polymorph_type_names[ (int) i ];
}


const char *lt_typename (int type) {
	return ( type > -1 && type <= LITE_NOD ) ? lt_polymorph_type_names[ type ] : NULL;
}


//Move left or right within the hierarchy of tables
int lt_move (Table *t, int dir) {
	//Out of space
	if ( t->index > t->total ) {
		t->error = ZHASHER_ERR_LT_OUT_OF_SPACE;
		return -1;
	}

	//LiteValue *curr  = &(t->head + t->index)->value;
	LiteKv *curr     = (t->head + t->index);
	LiteKv **cptr    = &curr;
	LiteValue *value = &curr->value;

	//Left or right?	
	if ( !dir ) {
		//Set count of elements in this new table to actual count
		LiteTable *T = &value->v.vtable;
		value->type  = LITE_TBL;
		t->rCount    = &T->count;

		/*Yay*/
		T->parent  = ( !t->current ) ? NULL : t->current;
		T->ptr     = *(long *)&T; 
		t->current = T;

		//Ascension shouldn't need pointer increment...
	#ifdef SUPEREXTRA
		t->count ++;
		t->index ++;
	#endif
	}
	else 
	{
		//Set references
		LiteValue *key = &curr->key; 
		key->type      = LITE_TRM;
		value->type    = LITE_NUL;
		LiteRecord *r  = &key->v;	

		//....
		if ( !t->current->parent )
		{
			t->rCount = &t->count;
			r->vptr = (long)t->current->ptr;
			t->current = NULL;
		}
		else if ( t->current->parent )
		{
			r->vptr = (long)t->current->ptr;
			LiteTable *T = t->current->parent;
			t->rCount = &T->count;
			t->current = T;
		}

	#ifdef SUPEREXTRA
		lt_finalize (t);
	#endif
	}

#ifndef SUPEREXTRA
	lt_finalize( t );		
#endif
	return 1;
}



//Finalize adding to both sides of a table data structure
void lt_finalize (Table *t) {
	//if these are equal, don't increment both *t->rCount and t->count
	( t->rCount == &t->count ) ? 0 : ( *t->rCount )++ ; 
	t->count ++;
	t->index ++;
}



//Hash each key
void lt_lock (Table *t) {
	//Might not be able to reuse this...	
	LiteKv *parent = NULL;
	LiteValue *v   = NULL;

	for (int i=0; i <= t->index; i++) 
	{
		//Get reference
		LiteKv   *tt = t->head + i,
					 *slot = NULL;
		int       pp = 0,
               h = 0;

		//Make a new buffer
		unsigned char buf[ LT_POLYMORPH_BUFLEN ];
		memset( buf, 0, LT_POLYMORPH_BUFLEN );

		//Check keys and values...
		if (tt->value.type == LITE_NUL )
		{
			v = NULL;
			if (parent)
				parent = parent->parent;
			continue;
		}

		//Set parent of an item.
		if ( parent )	
			tt->parent = parent, v = &tt->value;
	
		//Do parents here
		if (tt->value.type == LITE_TBL)
			parent = tt;

		//Build a string to hash, save the hash somewhere 
		pp   = build_backwards( tt, buf, LT_POLYMORPH_BUFLEN );
		h    = lt_hashu( buf, pp, t->modulo );
		slot = t->head + h;

		//Find an available slot
		for ( int m=0, j=0; !j && m < lt_max_slots; m++ )
		{
			slot->hash[m] == -1 ?	slot->hash[ m ] = i, j=1 : 0;
		}
	}
}




//Return index in table where key was found
int lt_get_long_i (Table *t, unsigned char *find, int len) 
{
	LiteKv *hv   = NULL;
	int     hash = 0,  
          hh   = 0;
	uint8_t *f   = NULL;
	uint8_t gb[ LT_POLYMORPH_BUFLEN ] = { 0 };

	if ( len > LT_POLYMORPH_BUFLEN )
		return -1;

	if ( len < 0 )
		return -1;

	if ( !t->start && !t->end ) 
		hash = lt_hashu( f = find, len, t->modulo );
	else 
	{
		//Ewww..
		int a = 0;
		memcpy( &gb[0], t->buf, t->buflen );
		if ( *find != '.' )
		{
			a = 1;
			memcpy( &gb[ t->buflen ], ".", 1 ); 
		}	
		memcpy( &gb[ t->buflen + a ], find, len );
		hash = lt_hashu( gb, t->buflen + a + len, t->modulo );
		f = gb;
	}

	//Find the key
	for ( int i=0 ; !hv && i < 5; i++ ) 
	{
		uint8_t buf[LT_POLYMORPH_BUFLEN] = {0};
		//fprintf( stderr, "%d, %d, %d\n", hh, hash, i );

		if ((hh = (t->head + hash)->hash[i]) == -1 || i == lt_max_slots )
		{
#if 0
			fprintf( stderr, "#1 error\n" );
#endif
			return -1;
		}

#if 0
		fprintf( stderr, "hash %d, hh %d, current pos in slot arr: %d\n", hash, hh, i );
#endif

		if ( !(hv = t->head + hh) )
		{
#if 0
			fprintf( stderr, "#2 error\n" );
#endif
			return -1;
		}
	
		//?	
		if (build_backwards( hv, buf, LT_POLYMORPH_BUFLEN ) == -1)
		{
#if 0
			fprintf( stderr, "#3 error\n" );
#endif
			return -1;
		}

#if 0
		fprintf( stderr, "looking for: '%s' (len: %d) => found: '%s' (len: %d)\n", 
			buf, (int)strlen((char *)buf), find, len );
#endif

		hv = ( memcmp(f, buf, len) == 0) ? hv : 0;
	}

	//Pull the value if it's in the acceptable range
	if ( !t->start && !t->end )
		return hh;
	else 
	{
		//fprintf( stderr, "nums[3] => %d, %d, %d", t->start, hh, t->end );
		return ( hh > t->start && hh < t->end ) ? hh : -1;	
	}

}



//Return LiteKv at certain index
LiteValue *lt_retany (Table *t, int index) {
	return ( index <= -1 || index > t->count ) ? NULL : &(t->head + index)->value; 
}


int lt_exists (Table *t, int index) {
	return ( index <= -1 || index > t->count );
}


//Return a LiteKv at a certain index
LiteKv *lt_retkv (Table *t, int index) {
	if ( index <= -1 || index > t->count ) {
		t->error = ZHASHER_ERR_LT_INVALID_INDEX;
		return NULL;
	}

	return (t->head + index); 
}


//Return a LiteRecord matching a certain type at a certain index
LiteRecord *lt_ret (Table *t, LiteType type, int index) {
	if ( index <= -1 || index > t->count ) {
		t->error = ZHASHER_ERR_LT_INVALID_INDEX;
		return (LiteRecord *)supernul; 
	}
#if 0
	}
	else {
		if ( index <= -1 || index < t->start || index > t->end ) {
			t->error = ZHASHER_ERR_LT_OUT_OF_SLICE;
			return (LiteRecord *)supernul;
		}	
	}
#endif	

	if ( (t->head + index)->value.type != type ) { 
		return (LiteRecord *)supernul; 
	}

	return &(t->head + index)->value.v; 
}


//Set the index to another one (absolutely)
int lt_absset( Table *t, int index ) {
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
int lt_set (Table *t, int index) {
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
void lt_reset (Table *t) {
	t->start = 0;
	t->end   = 0;
	t->index = 0;
}



//Iterate through the indices of a table
LiteKv *lt_next (Table *t) {
	LiteKv *curr = (t->index > t->count) ? NULL : t->head + t->index;
	t->index++;
	return curr;
}


LiteKv *lt_current (Table *t) {
	return ( t->index > t->count ) ? NULL : t->head + t->index;
}


//Loop from another point...
LiteKv *lt_items_by_index (Table *t, int ind)
{
	//Find a hash, and if it's a table... set some stuff
	LiteKv *curr = NULL;

	if ( t->cptr == -1 )
	{
		if ( (t->head + ind)->value.type != LITE_TBL ) 
			return NULL;
		t->index = ind;
		t->cptr = (t->head + ind)->value.v.vtable.ptr;
	}

	//
	if (t->index > t->count) 
		return NULL;

	//Set reference
	curr = t->head + t->index;

	//Check the key name and see if it matches t->cptr, return null if so
	if ( curr->key.type == LITE_TRM && curr->key.v.vptr == t->cptr ) 
	{
		t->index = 0;
		t->cptr = -1;
		return NULL;
	}

	//Increment and move on
	t->index++;
	return curr;
}



//Find a table by hash and return until it has no more keys.
LiteKv *lt_items_i (Table *t, uint8_t *src, int len) {
	//Find a hash, and if it's a table... set some stuff
	LiteKv *curr = NULL;

	//Check for a hash table
	if ( t->cptr == -1 ) {
		int in;
		if ( (in = lt_get_long_i ( t, src, len )) == -1 )
			return NULL;

		if ( (t->head + in)->value.type != LITE_TBL ) 
			return NULL;
		
		t->index = in;
		t->cptr = (t->head + in)->value.v.vtable.ptr;
	}

	//
	if (t->index > t->count) {
		return NULL;
	}

	//Set reference
	curr = t->head + t->index;

	//Check the key name and see if it matches t->cptr, return null if so
	if ( curr->key.type == LITE_TRM && curr->key.v.vptr == t->cptr ) {
		t->index = 0;
		t->cptr = -1;
		return NULL;
	}

	//Increment and move on
	t->index++;
	return curr;
} 



//Set a data source
void lt_setsrc (Table *t, void *src) {
	t->src = src;
}


//Will set boundaries on a new table
Table *lt_within_long( Table *st, uint8_t *src, int len ) {
#if 0
	//You can allocate the string here if both start and from are null
	if ( !t->start && !t->end )
	{
		if ( !(t->buf =  malloc( len + 2048 )) )
			return NULL;
		else {
			memcpy( t->buf, src, len );
		}
	}
#endif

	//Whenever we look for a string, we copy til the end
	int a      = 0;
	int count  = 0;
	st->buf    = src;  //set the buffer
	st->buflen = len;

	//Search for a table	
	if ( (a = lt_get_long_i( st, src, len )) == -1 || lt_vta( st, a ) != LITE_TBL ) {
		return NULL;
	}

	//Set start and end, then return the table
	st->start = a;
	st->end   = a + (&lt_table_at( st, a ))->count;
	return st;
}


void lt_unset (Table *t) {
	if (t->buf) {
		free( t->buf );
		t->buf = NULL;
	}
}


//Copy from start index to end index
//A macro will handle copying tables (and it'd be even better to do it without duplication)
Table *lt_copy (Table *dest, Table *src, int from, int to, int weak) {
	//Get a count of all elements
	int index = 0;
	int start = (from < 0 ) ? 0 : from;
	int end   = (to == -1) ? src->index : to;
#if 0
	Table *tt = NULL;

	//Have to allocate a new table here
	if ( !(tt = malloc( sizeof(Table))) )
	{
		t->error = ZHASHER_ERR_LT_ALLOCATE;
		return NULL;
	}
#endif

#if 0
	//Create one on heap (expensive...)
	if ( !(dest = lt_init( dest, NULL, lt_counti( t, index ))) )
	{	
		t->error = ZHASHER_ERR_LT_ALLOCATE;
		return NULL;
	}

	//Loop through each requested element and add it
	for (int ii=start; ii <= end; ii++) 
	{
		LiteValue *r[3] = 
		{
			&(t->head + ii)->key,
			&(t->head + ii)->value,
			NULL,	
		};

#define lt_mega( ... )
		for ( LiteValue **v=r; *v; v++ )
		{
			LiteValue *vv = *v;
			/*LITE_NODE is handled in printall*/
			if (vv->type == LITE_NON)
				lt_mega( tt );
			else if (vv->type == LITE_NUL)
				lt_mega( tt );
			else if (vv->type == LITE_USR)
				lt_mega( tt );
			else if (vv->type == LITE_TBL) 
				lt_mega( tt );
			else if (vv->type == LITE_FLT || vv->type == LITE_INT)
				lt_mega( tt );
			else if (vv->type == LITE_FLT)
				lt_mega( tt );
			else if (vv->type == LITE_TXT)
				lt_mega( tt );
			else if (vv->type == LITE_TRM)
				lt_mega( tt );
			else if (vv->type == LITE_BLB) 
				lt_mega( tt );
		}
	}

	//Lock so hashing works	
	lt_lock( tt );
#endif
	return NULL;
}


//Get a key or value somewhere
void lt_free (Table *t) {	
	//Free any text keys
	for ( int ii=0; ii < t->count; ii++ ) {
		LiteKv *k = t->head + ii;
		( k->key.type == LITE_TXT ) ? free( k->key.v.vchar ), k->key.v.vchar = NULL : 0;
		( k->value.type == LITE_TXT ) ? free( k->value.v.vchar ), k->value.v.vchar = NULL : 0;
	}

	if ( t->mallocd ) {
		free( t->head );
		t->head   = NULL;	
		t->error  = 0;
		t->total  = 0;
		t->count  = 0;
		t->index  = 0;
		t->rCount = NULL;
		t->mallocd= 0;
	}

	if ( t->srcmallocd /*t->src*/ ) {
		free( t->src );
		t->src = NULL;
	}
}

//Print out an initialized table
void lt_printt (Table *t) 
{
	fprintf( stderr, "t->total:      %d\n", t->total );
	fprintf( stderr, "t->modulo:     %d\n", t->modulo );
	fprintf( stderr, "t->index:      %d\n", t->index );
	fprintf( stderr, "t->count:      %d\n", t->count );
	fprintf( stderr, "t->rCount:     %p\n", (void *)t->rCount );
	fprintf( stderr, "t->head:       %p\n", (void *)t->head );
}


//Print a set of values at a particular index
static void lt_printindex (LiteKv *tt, int showkey, int ind) {
	int w = 0;
	int maxlen = (showkey) ? 24576 : lt_buflen;
  char b[maxlen]; 
	memset(b, 0, maxlen);
	struct { int t; LiteRecord *r; } items[2] = {
		{ tt->key.type  , &tt->key.v    },
		{ tt->value.type, &tt->value.v  } 
	};

	for ( int i=0; i<2; i++ ) {
		LiteRecord *r = items[i].r; 
		int t = items[i].t;
		if ( i ) {
			memcpy( &b[w], " -> ", 4 );
			w += 4;
			/*LITE_NODE is handled in printall*/
			if (t == LITE_NON)
				w += snprintf( &b[w], maxlen - w, "%s", "is uninitialized" );
		#ifdef LITE_NUL
			else if (t == LITE_NUL)
				w += snprintf( &b[w], maxlen - w, "is terminator" );
		#endif
			else if (t == LITE_USR)
				w += snprintf( &b[w], maxlen - w, "userdata [address: %p]", r->vusrdata );
			else if (t == LITE_TBL) {
				LiteTable *rt = &r->vtable;
				w += snprintf( &b[w], maxlen - w, 
					"table [address: %p, ptr: %ld, elements: %d]", (void *)rt, rt->ptr, rt->count );
			}
		}

	//TODO: This just got ugly.  Combine the different situations better...
	if ( !i && showkey ) { 
		if ( t == LITE_TRM )
			w += snprintf( &b[w], maxlen - w, "%ld", r->vptr );
		else if ( t == LITE_NON || t == LITE_NUL )
			w += snprintf( &b[w], maxlen - w, "(null)" );
		else {
			w += build_backwards( tt, (unsigned char *)b, maxlen );
		}
	}
	else {
		//I want to see the full key
		if (t == LITE_FLT || t == LITE_INT)
			w += snprintf( &b[w], maxlen - w, "%d", r->vint );
		else if (t == LITE_FLT)
			w += snprintf( &b[w], maxlen - w, "%f", r->vfloat );
		else if (t == LITE_TXT)
			w += snprintf( &b[w], maxlen - w, "%s", r->vchar );
		else if (t == LITE_TRM)
			w += snprintf( &b[w], maxlen - w, "%ld", r->vptr );
		else if (t == LITE_BLB) {
			LiteBlob *bb = &r->vblob;
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

	write( LT_DEVICE, b, w );
	write( LT_DEVICE, "\n", 1 );
}	


//Dump all values in a table.
static const char __lt_fmt[] =
	"[%-5d] (%d) %s";
static const char __lt_tabs[] = 
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
static const char __lt_spaces[] = 
	"                                                  "
	"                                                  "
;
 

//Dump a table (needs some flags for debugging) 
int __lt_dump ( LiteKv *kv, int i, void *p ) {
	LiteType vt = kv->value.type;
	LtInner *pp = (LtInner *)p; 
	if ( pp->indextype ) {
		char buf[ 128 ] = { 0 };
		int l = snprintf( buf, sizeof(buf), __lt_fmt, i, pp->level, &__lt_spaces[100 - pp->level] );
		write( LT_DEVICE, buf, l );
	}
	lt_printindex( kv, pp->dumptype, pp->level );
	pp->level += (vt == LITE_NUL) ? -1 : (vt == LITE_TBL) ? 1 : 0;
	return 1;
}


//A complicated iterator
int lt_exec_complex (Table *t, int start, int end, void *p, int (*fp)( LiteKv *kv, int i, void *p ) ) {
	int level = 0;	

	//Bounds violations should stop.
	if ( start < 0 || start > t->count || end < 0 || end > t->count ) {
		return 0;
	}

#if 0
	//Loop through each index
	for ( int i=start, status=0; i <= end; i++ ) {
		//VPRINT( "kv at __lt_dump: %p", (t->head + i ));
		if ( (status = fp( (t->head + i), i, p )) == 0 ) {
			return 0;
		}
	}
#else
	//Loop through each index
	int i = start;
	int status = 0;
	do {
		if ( (status = fp( (t->head + i), i, p )) == 0 ) {
			return 0;
		}
	} while ( ++i < end );
#if 0
	for ( int i=start, status=0; i <= end; i++ ) {
		//VPRINT( "kv at __lt_dump: %p", (t->head + i ));
		if ( (status = fp( (t->head + i), i, p )) == 0 ) {
			return 0;
		}
	}
#endif
#endif
	return 1;
}



#ifdef DEBUG_H 
//Get a key or value somewhere
void lt_printall ( Table *t ) {
	//Header
	fprintf( stderr, fmt, "Index", "KType", "VType", "Value", "CombinedValue", "HashOf", "Hashes" );

	for ( int ii=0; ii < t->index; ii++ ) {
		LiteKv *k = t->head + ii;
		LiteType kt;
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
		if ((kt = lt_rettype( t, 0, ii )) == LITE_INT )
			sprintf( strbuf, "%d, ", (t->head + ii )->key.v.vint );
		else if ( kt == LITE_TXT )
			sprintf( strbuf, "%s, ", (t->head + ii )->key.v.vchar );
		else if ( kt == LITE_BLB ) 
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
		build_backwards( t->head + ii, (uint8_t *)bkbuf, 1024 );
		hash = lt_hashu( (uint8_t *)bkbuf, strlen(bkbuf), t->modulo );
		sprintf( habuf, "%d", ( kt == LITE_NON ) ? -1 : hash );
		fprintf( stderr, fmt, inbuf, kk, vv, strbuf, bkbuf, habuf, nmbuf );
	}
}
#endif
