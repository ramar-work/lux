#include "lua.h"

struct dumper {
	//enum { zhInner.LT_DUMP_SHORT, zhInner.LT_DUMP_LONG } dumptype;
	int len, pos;
	char level, *buffer;
};





int __lua_dump ( zKeyval *kv, int ii, void *p ) {	
	//Define things
	struct dumper *pp = (struct dumper *)p;
	const int maxlen = 24576;
	const char spaces[] = "                   "
	"                   "
	"                   "
	"                   "
	"                   ";
	struct { int t; zhRecord *r; } items[2] = {
		{ kv->key.type  , &kv->key.v    },
		{ kv->value.type, &kv->value.v  } 
	};

	//Initialize our buffer for writing and write the index plus tabs
	char b[ maxlen ];
	memset( b, 0, maxlen );
	int w = snprintf( b, maxlen, "[%d]%s", ii, &spaces[ 100 - pp->level ]  ); 

	//Then loop through both sides of zKeyval and dump the values
	for ( int i = 0; i < 2; i++ ) {
		zhRecord *r = items[i].r; 
		int t = items[i].t;
		if ( i ) {
			w += snprintf( &b[w], maxlen - w, "%s", " -> " );
			/*ZTABLE_NODE is handled in printall*/
			if ( t == ZTABLE_NON )
				w += snprintf( &b[w], maxlen - w, "%s", "is uninitialized" );
		#ifdef ZTABLE_NUL
			else if ( t == ZTABLE_NUL )
				w += snprintf( &b[w], maxlen - w, "is terminator" );
		#endif
			else if ( t == ZTABLE_USR )
				w += snprintf( &b[w], maxlen - w, "userdata [address: %p]", r->vusrdata );
			else if ( t == ZTABLE_TBL ) {
				pp->level ++;
				zhTable *rt = &r->vtable;
				w += snprintf( &b[w], maxlen - w, 
					"table [address: %p, ptr: %ld, elements: %d]", 
					(void *)rt, rt->ptr, rt->count );
			}
			else {
				w += snprintf( &b[w], maxlen - w, "(%s) ", lt_typename( t ) );
			}
		}

	#if 0
		//TODO: This just got ugly.  Combine the different situations better...
		if ( !i && ( pp->dumptype == LT_DUMP_LONG ) ) { 
			//I want to see the full key
			if ( t == ZTABLE_TRM )
				w += snprintf( &b[w], maxlen - w, "(trm) %ld", r->vptr ), pp->level--;
			else if ( t == ZTABLE_NON || t == ZTABLE_NUL )
				w += snprintf( &b[w], maxlen - w, "(null)" );
			else {
				//We want to see the length of the built string, and potentially its parent(s)
				char a[ maxlen ];
				memset( a, 0, maxlen );
				int sl = build_backwards( kv, (unsigned char *)a, maxlen );
				int sb = snprintf( &b[w], maxlen - w, "(%d) (%s)", sl, a );	
				w += sb;
			}
		}
		else {
	#endif
		if ( t == ZTABLE_INT )
			w += snprintf( &b[w], maxlen - w, "%d", r->vint );
		else if ( t == ZTABLE_FLT )
			w += snprintf( &b[w], maxlen - w, "%f", r->vfloat );
		else if ( t == ZTABLE_TXT )
			w += snprintf( &b[w], maxlen - w, "%s", r->vchar );
		else if ( t == ZTABLE_TRM )
			w += snprintf( &b[w], maxlen - w, "} %ld", r->vptr );
		else if ( t == ZTABLE_BLB ) {
			zhBlob *bb = &r->vblob;
			if ( bb->size < 0 )
				return 1;
			if ( bb->size > ( maxlen - w ) )
				w += snprintf( &b[w], maxlen - w, "is blob (%d bytes)", bb->size);
			else {
				memcpy( &b[w], bb->blob, bb->size ); 
				w += bb->size;
			}
		}
	#if 0
		}
	#endif
	}
#if 0
	write( pp->fd, b, w );
	write( pp->fd, "\n", 1 );
#else
	//Increment total size 
	pp->len += w;

	//...and reallocate
	if ( !( pp->buffer = realloc( pp->buffer, pp->len )) ) {
		//t->error = ALLOCATION_FAILURE_YAH;
		free( pp->buffer );
		return 0;
	}

	//pp->buffer[ i ] = buf, pp->buffer[ sz - 1 ] = NULL;
	//...and copy
	if ( !memset( &pp->buffer[ pp->pos ], 0, w ) || !memcpy( &pp->buffer[ pp->pos ], b, w ) ) {
		//t->error = ALLOCATION_FAILURE_YAH;
		free( pp->buffer );
		return 0;
	}

	//Increment the position and yay.
	pp->pos += w;	
#endif
	return 1;
}

//Do the same job as a regular dump, but to a buffer
int lua_dump_var ( lua_State *L ) {
	//Turn to ztable and dump?
	return 0;
}

struct luaL_Reg lua_set[] = {
	{ "dump", lua_dump_var }
,	{ NULL }
};


