/* ------------------------------------------- * 
 * lua.c 
 * ====
 * 
 * Summary 
 * -------
 * Helpful extensions for Lua
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include "lua.h"

struct dumper {
	//enum { zhInner.LT_DUMP_SHORT, zhInner.LT_DUMP_LONG } dumptype;
	int len, pos;
	char pretty, level, sep, *buffer;
};


static int __lua_dump ( zKeyval *kv, int ii, void *p ) {	
	//Define things
	struct dumper *pp = (struct dumper *)p;
	const int maxlen = 24576;
	const char spaces[] = 
	"                    "
	"                    "
	"                    "
	"                    "
	"                    ";
	struct { int t; zhRecord *r; } items[2] = {
		{ kv->key.type  , &kv->key.v    },
		{ kv->value.type, &kv->value.v  } 
	};

	//Initialize our buffer for writing and write the index plus tabs
	char b[ maxlen ];
	memset( b, 0, maxlen );
	int w = 0;
	if ( pp->pretty ) {
		w = snprintf( b, maxlen, "%s", &spaces[ 100 - pp->level ]  ); 
	}

	//Then loop through both sides of zKeyval and dump the values
	for ( int i = 0; i < 2; i++ ) {
		zhRecord *r = items[i].r; 
		int t = items[i].t;
		if ( i ) {
			w += snprintf( &b[w], maxlen - w, "%s", " -> " );
			/*ZTABLE_NODE is handled in printall*/
			if ( t == ZTABLE_NON )
				w += snprintf( &b[w], maxlen - w, "%s", "(null)" );
		#ifdef ZTABLE_NUL
			else if ( t == ZTABLE_NUL )
				w += snprintf( &b[w], maxlen - w, "}" );
		#endif
			else if ( t == ZTABLE_USR )
				w += snprintf( &b[w], maxlen - w, "userdata [address: %p]", r->vusrdata );
			else if ( t == ZTABLE_TBL ) {
				pp->level ++;
				zhTable *rt = &r->vtable;
				w += snprintf( &b[w], maxlen - w, "%s", "{" );
			}
			else {
				w += snprintf( &b[w], maxlen - w, "(%s) ", lt_typename( t ) );
				if ( t == ZTABLE_TXT )
					w += snprintf( &b[w], maxlen - w, "(len: %ld) ", strlen( r->vchar ) ); 
				else if ( t == ZTABLE_BLB ) {
					w += snprintf( &b[w], maxlen - w, "(len: %d) ", r->vblob.size ); 
				}	
			}
		}

		if ( t == ZTABLE_INT )
			w += snprintf( &b[w], maxlen - w, "%d", r->vint );
		else if ( t == ZTABLE_FLT )
			w += snprintf( &b[w], maxlen - w, "%f", r->vfloat );
		else if ( t == ZTABLE_TXT )
			w += snprintf( &b[w], maxlen - w, "'%s'", r->vchar );
		else if ( t == ZTABLE_TRM )
			w += snprintf( &b[w], maxlen - w, "} %ld", r->vptr );
		else if ( t == ZTABLE_BLB ) {
			zhBlob *bb = &r->vblob;
			if ( bb->size < 0 )
				return 1;
			if ( bb->size > ( maxlen - w ) )
				w += snprintf( &b[w], maxlen - w, "BLOB" );
			else {
				w += snprintf( &b[w], maxlen - w, "<<" );
				memcpy( &b[w], bb->blob, bb->size ); 
				w += bb->size;
				w += snprintf( &b[w], maxlen - w, ">>" );
			}
		}
	}

	//Add a newline
	w += snprintf( &b[w], maxlen - w, "%c ", pp->sep );

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
	return 1;
}



//Do the same job as a regular dump, but to a buffer
int lua_dump_var ( lua_State *L ) {
	luaL_checktype( L, 1, LUA_TTABLE );

	//Turn to ztable and dump?
	struct dumper d = { 0 };
	zTable tt, *results, *t = &tt;
	lt_init( t, NULL, 32 ); 
	d.sep = ',';

	//...		
	if ( !lua_to_ztable( L, 1, t ) || !lt_lock( t ) ) {
		lt_free( t );
		return luaL_error( L, "Failed to convert to zTable" );
	}

	//Start the string off.
	if ( !( d.buffer = malloc( 16 ) ) || !memset( d.buffer, 0, 16 ) ) {
		return luaL_error( L, "Failed to initialize dump buffer" );
	}

	//Do something with this.
	d.buffer[0] = '{', d.buffer[1] = ' ', d.len = 2, d.pos = 2;

	//Dump all keys and put them in a buffer
	if ( !lt_exec( t, &d, __lua_dump ) ) {
		free( d.buffer );
		return luaL_error( L, "Error when dumping this table." );
	}

	//Terminate the string properly
	d.buffer[ d.pos - 2 ] = ' ', d.buffer[ d.pos - 1 ] = '}';

	if ( !lua_pushlstring( L, d.buffer, d.len ) ) {
		//Needs to be handled properly 
		free( d.buffer );
		return 0;
	}

	free( d.buffer );
	lt_free( t );
	return 1;
}

struct luaL_Reg lua_set[] = {
	{ "dump", lua_dump_var }
,	{ NULL }
};


