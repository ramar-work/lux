/* ------------------------------------------------------- *
 * zrender.c
 * =========
 * 
 * Summary 
 * -------
 * Enables zTables (and eventually other data structures in C)
 * to be used in templating.
 *
 *
 * Usage
 * -----
 * Basic usage in a C program looks something like:
 * <pre>
 * unsigned char *buf = NULL;
 * int buflen = 0;
 * char *template = $generic_file_read_to_buffer( ... );
 * 
 * zRender *rz = zrender_init();
 * zrender_set_default_dialect( rz );
 * zrender_set_fetchdata( rz, tt );
 * 
 * if ( !(buf = zrender_render( rz, template, strlen( template ), &buflen )) ) {
 * 	fprintf( stderr, "%s\n", zrender_strerror( rz ) );
 * 	zrender_free( rz );
 * 	return 1;
 * }
 * 
 * write( 1, buf, buflen );
 * zrender_free( rz );
 * </pre>
 *
 * Changes made in the future will most likely be making it easier to
 * define your own templating languages, and cutting down on the code
 * needed to get rendering working.
 *
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
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
 *
 * CHANGELOG 
 * ---------
 * 01-08-21 - Rewrote entire mapping routine, takes more memory, but far
 * simpler to reason about and less error prone.
 *
 * ------------------------------------------------------- */
#include "zrender.h"

//Duplicate a block 
static unsigned char * zr_dupblk( const unsigned char *v, int vlen ) {
	unsigned char * vv = malloc( vlen );
	memset( vv, 0, vlen );
	memcpy( vv, v, vlen );
	return vv;
}



//Write full path to a node
static char * lookup_xmap ( struct xmap *xp, char *xb, int xblen ) {
	struct xdesc *tp = xp->parent;	
	int px = 0, len = 0;
	struct parent { int l, i; char *c; } parents[ 64 ] = { 0 };

	//Really should check this...
	char e[xblen];
	char *d = memset(e, 0, xblen);
	//e = d;

	//Move backwards among parents until we reach the top
	while ( tp->pxmap ) {
		struct parent *x = &parents[ px ];
		x->c = (char *)(tp->pxmap)->ptr; 
		x->l = (tp->pxmap)->len; 
		x->i = tp->index; 
		px++;
		tp = (tp->pxmap)->parent;
	}

	//Create a bigger string
	while ( --px > -1 ) {
		int bi = 0;
		struct parent x = parents[ px ];	
		memcpy( d, x.c, x.l ) ? d += x.l, len += x.l : 0;
		( bi = sprintf( d, ".%d", x.i ) ) ? d += bi, len += bi : 0; 
	}

	//Add the last key
	memcpy( d, xp->ptr, xp->len ) ? d += xp->len, len += xp->len : 0; 

	//TODO: This is an extra step
	memcpy( xb, e, len ); 

	//Write the whole thing
	return xb;
}


//Utility to add to a series of items
static void * add_item_to_list( void ***list, void *element, int size, int * len ) {
	//Reallocate
	if (( (*list) = realloc( (*list), size * ( (*len) + 2 ) )) == NULL ) {
		return NULL;
	}

	(*list)[ *len ] = element; 
	(*list)[ (*len) + 1 ] = NULL; 
	(*len) += 1; 
	return list;
}


//Utility to trim an unsigned character block 
static unsigned char *zr_trim 
 ( const unsigned char *msg, const char *trim, int len, int *nlen ) {
	unsigned char *forwards = (unsigned char *)msg;
	unsigned char *backwards = (unsigned char *)&msg[ len - 1 ];
	int nl = len;
	int tl = strlen( trim );
	while ( nl ) {
		int dobreak = 1;
		if ( memchr( trim, *forwards, tl ) )
			forwards++, nl--, dobreak = 0;
		if ( memchr( trim, *backwards, tl ) )
			backwards--, nl--, dobreak = 0;
		if ( dobreak ) {
			break;
		}	
	}
	( nlen ) ? *nlen = nl : 0;
	return forwards;
}



//Initialize a premap
static struct premap * init_premap() {
	struct premap * p = malloc ( sizeof( struct premap ) );
	return ( !p ) ? NULL : memset( p, 0, sizeof( struct premap ) ); 
}


//Initialize a new xmap
static struct xmap * init_xmap() {
	struct xmap * p = malloc ( sizeof( struct xmap ) );
	return ( !p ) ? NULL : memset( p, 0, sizeof( struct xmap ) ); 
}

//Clone xmap?
static struct xmap * clone_xmap( struct xmap *x ) {
	struct xmap * p = malloc ( sizeof( struct xmap ) );
	if ( !p ) {
		return NULL; 
	}
	memset( p, 0, sizeof( struct xmap ) ); 
	p->ptr = x->ptr;
	p->parent = x->parent;
	p->len = x->len;
	p->type = x->type;
	return p;
}


//Destroy the premap
static void free_premap( struct premap **premap ) {
	struct premap **p = premap;
	while ( p && *p ) {
		free( *p );
		p++;
	}	
	free( premap );
}


//Destroy the map
static void free_xmap( struct xmap **map ) {
	struct xmap **p = map;
	while ( p && *p ) {
		if ( (*p)->free ) {
			free( (*p)->ptr );
		}
		free( *p );
		p++;
	}
	free( map );
}


//Hmm, this works for tables, but not for any other data structure....
static void extract_value ( zRender *rz, int hash, struct xmap *xp ) {
	zKeyval *lt = lt_retkv( rz->userdata, hash );
	if ( lt->value.type == ZTABLE_TXT && lt->value.v.vchar != NULL ) {
		xp->len = strlen( lt->value.v.vchar ); 
		xp->ptr = (unsigned char *)lt->value.v.vchar;
	}
	else if ( lt->value.type == ZTABLE_BLB ) {
		xp->len = lt->value.v.vblob.size; 
		xp->ptr = lt->value.v.vblob.blob; 
		xp->free = 0;
	}
	else if ( lt->value.type == ZTABLE_INT ) {
		char intptr[32] = {0}; 
		xp->len = snprintf( intptr, sizeof( intptr ), "%d", lt->value.v.vint ); 
		xp->ptr = (unsigned char *)zr_dupstr( intptr );
		xp->free = 1;
	}
	else {
		xp->ptr = (unsigned char *)"";
		xp->len = 0;
		xp->free = 0; 
	}
}



//Set specific error strings
static void zr_set_strerror ( short error ) {
	//can probably use some snprintf magic to make this work
}


//Initialize the object
zRender * zrender_init() {
	zRender *zr = malloc( sizeof( zRender ) );
	if ( !zr || !memset( zr, 0, sizeof( zRender ) ) ) {
		return NULL;
	}
	//SX is the default, because it matches pretty much anything else...
	memset( zr->xmapset, SX, sizeof(zr->xmapset));
	return zr;
}


//Set the source for fetching data
void zrender_set_fetchdata( zRender *rz, void *t ) { 
	rz->userdata = t;	
}



//...
void zrender_set_boundaries ( zRender *rz, const char *s, const char *end ) {
	( s ) ? rz->zStart = zr_dupstr( s ) : 0;
	( end ) ? rz->zEnd = zr_dupstr( end ) : 0;
}



//...
void zrender_set( zRender *rz, const char map, short code ) {
	rz->xmapset[ (short)map ] = code;
}



//....
const char * zrender_strerror( zRender *z ) {
	return z->errmsg;
}



//Use the default templating language (mustache)
void zrender_set_default_dialect( zRender *rz ) {
	zrender_set_boundaries( rz, "{{", "}}" );
	zrender_set( rz, '#', LS ); 
	zrender_set( rz, '/', LE ); 
	zrender_set( rz, '.', CX ); 
	zrender_set( rz, 1, SX ); 
	//TODO: Do I need to mark this?
	zrender_set( rz, 0, RW ); 
#if 0
	//Simple extracts are anything BUT the other characters (but raw is also 1, so...)
	zrender_set( rz, '!', BL ); 
	zrender_set( rz, '`', EX ); 
	zrender_set( rz, '$', EK ); 
#endif
}



//mapping / marking
int zrender_set_marks( zRender *rz, const unsigned char *src, unsigned int srclen ) {
	zWalker r = {0};
	unsigned char check[] = { rz->zStart[0], rz->zEnd[0], '\n' };
	struct pos { int a, b, l, lp; } set[128] = {0}; 
	int ls = 0, le = 0, sl = 0, el = 0, nl = 0;

	//Early marking list goes here
	struct premap **premap = NULL;
	int pmlen = 0;

	//just check that the list is balanced
	while ( memwalk( &r, src, check, srclen, sizeof(check) ) ) {
		struct premap *p = init_premap(); 

		if ( r.chr == '\n' ) // NEWLINE CHECKS
			nl++, set[ sl ].lp = r.pos, p->ptr = r.src, p->len = r.size;
		else if ( r.chr == check[0] && *r.ptr == check[0] ) // RAW LOOPS
			set[ sl ].a = r.pos, set[ sl ].l = nl, sl++, p->ptr = r.src, p->len = r.size - 1;
		else if ( r.chr == check[1] && *r.ptr == check[1] ) { // INNER LOOPS
			set[ el ].b = r.pos;
			#if 1
			//Do another check for the end sequence before a start sequence
			if ( el > sl ) {
				//rz->error = ZRENDER_SYNTAX_LOOP;
				//TODO: Add exact positioning later
				const char fmt[] = "Loop end sequence detected at line %d, but no loop start found\n";
				snprintf( rz->errmsg, 1024, fmt, nl ); 
				return 0;	
			}
			#endif
			unsigned char * ts = zr_trim( r.src, " ", r.size - 1, NULL ); 
			if ( *ts == '#' )
				ls++;
			else if ( *ts == '/' ) {
				//TODO: Add exact positioning later
				if ( ++le > ls ) {
					//rz->error = ZRENDER_SYNTAX_LOOP;
					const char fmt[] = "Loop end detected at line %d, but no loop start found\n";
					snprintf( rz->errmsg, 1024, fmt, nl ); 
					return 0;	
				}
			}
			p->ptr = r.src - 1, p->len = r.size + 1, el++;
		}
		else if ( *r.src != check[0] && *r.src != check[1] ) {  //[[ ANYTHING ELSE ]]
			p->ptr = r.src, p->len = ( *r.ptr == check[0] ) ? r.size - 1 : r.size; 
		}

		if ( p->len )
			zr_add_item( &premap, p, struct premap *, &pmlen );
		else {
			free( p );
		}
	}

	rz->premap = premap;
	return 1;
}



//expanding to map
int zrender_convert_marks( zRender *rz ) {
	struct premap **pmap = rz->premap;
	struct xmap **xmap = NULL;
	struct xdesc xdarray[32], *xdptr = memset( xdarray, 0, sizeof( struct xdesc ) * 32 );
	const struct xdesc *zdptr = xdptr;
	
	//Loop through all premaps
	for ( int nlen = 0, hash = -1, xmaplen = 0, render = 1; pmap && *pmap; ) {
		struct xmap *xp = init_xmap();
		struct premap *pp = *pmap;
		unsigned char *t = NULL;
		char xb[ 1024 ] = { 0 };
		xp->parent = NULL;

		//Raw write first
		if ( *pp->ptr != '{' ) {
			if ( !render ) 
				free( xp );
			else {
				xp->type = RW, xp->len = pp->len, xp->ptr = pp->ptr; 
				zr_add_item( &xmap, xp, struct xmap *, &xmaplen );
			}
			pmap++;
			continue;
		}

		//Get the "magic" character
		t = zr_trim( pp->ptr, "{} ", pp->len, &nlen );
		xp->type = rz->xmapset[ *t ];

		if ( xp->type == LE ) {
			xp->len = 0, xp->ptr = NULL, xp->parent = xdptr, xdptr->index++;
			if ( xdptr->index < xdptr->children ) 
				pmap = xdptr->cxmap;
			else {
				xdptr->index = 0;
				pmap++;
			}
			//TODO: This needs a proper fix.  All this does is cure the symptom...
			if ( xdptr != zdptr ) {	
				xdptr--;
			}
			render = 1;
		}
		else if ( xp->type == LS ) {
			xp->ptr = zr_trim( t, "# ", nlen, &nlen ), xp->parent = xdptr, xp->len = nlen; 
			if ( *xp->ptr != '.' )
				hash = lt_get_long_i( rz->userdata, xp->ptr, xp->len );
			else {
				hash = lt_geti( rz->userdata, lookup_xmap( xp, xb, sizeof( xb ) ) );
			}

			if ( hash == -1 ) 
				render = 0;
			else {
				//get the data at that point
				xdptr++;
				xdptr->children = lt_counti( rz->userdata, hash );
				//xdptr->index = !xdptr->index ? 0 : xdptr->index; 
				xdptr->pxmap = xp;
				xdptr->cxmap = pmap;
			}
			pmap++;
		}
		else if ( xp->type == SX ) {
			xp->len = nlen, xp->ptr = t;
			if ( ( hash = lt_get_long_i( rz->userdata, xp->ptr, xp->len ) ) == -1 ) 
				xp->len = 0, xp->ptr = NULL;
			else {
				extract_value( rz, hash, xp );
			}
			pmap++;
		}
		else if ( xp->type == CX ) {
			xp->len = nlen, xp->ptr = t, xp->parent = xdptr;
			if ( ( hash = lt_geti( rz->userdata, lookup_xmap( xp, xb, sizeof( xb ) ) ) ) == -1 )
				xp->len = 0, xp->ptr = NULL;
			else {
				extract_value( rz, hash, xp );
				xp->parent = xdptr;
			}

			pmap++;
		}
#if 0
		else if ( xp->type == BL )
		else if ( xp->type == EK )
		else if ( xp->type == EK )
#endif
		else {
			xp->len = 1, xp->ptr = t, xp->parent = NULL, pmap++;
		}
		
		zr_add_item( &xmap, xp, struct xmap *, &xmaplen );
	}

	rz->xmap = xmap;
	return 1;
}



//rendering...
unsigned char * zrender_interpret( zRender *rz, unsigned char **bb, int *llen ) {
	unsigned char *buf = NULL;
	struct xmap **p = rz->xmap;
	int len = 0;

	//Either use append to uint
	while ( p && *p ) {
		if ( (*p)->type != LS && (*p)->type != LE && (*p)->len > 0 ) {
			len += (*p)->len;
		
			//add to buffer	
			if ( ( buf = realloc( buf, len ) ) == NULL ) {
				( buf ) ? free( buf ) : 0;
				//ZRENDER_MALLOC_ERR
				return NULL;
			}

			//shorter name
			unsigned char *b = &buf[ len - (*p)->len ];

			//copy to buffer
			if ( !memset( b, 0, (*p)->len ) || !memcpy( b, (*p)->ptr, (*p)->len ) ) {
				free( buf );
				//ZRENDER_MAPPEND_ERR
				return NULL;
			}
		}
		p++;
	}

	*llen = len, *bb = buf;
	return buf;
}



//Do all the steps for rendering in one step.
unsigned char * zrender_render 
 ( zRender *rz, const unsigned char *src, int srclen, int *dlen )
{
	unsigned char *buf = NULL;
	int buflen = 0;

	if ( !zrender_set_marks( rz, src, srclen ) ) {
		fprintf( stderr, "set_marks failed\n" ); 
		return NULL;
	}

	if ( !zrender_convert_marks( rz ) ) {
		fprintf( stderr, "convert_marks failed\n" ); 
		return NULL;
	}

	if ( !zrender_interpret( rz, &buf, &buflen ) ) {
		fprintf( stderr, "interpret failed\n" ); 
		return NULL;
	}

	*dlen = buflen;
	return buf;
}



//Free all the stuffs
void zrender_free( zRender *rz ) {
	free_premap( rz->premap );
	free_xmap( rz->xmap );
	free( (void *)rz->zStart );
	free( (void *)rz->zEnd );
	free( rz );
}


#ifdef DEBUG_H 
static const char *xmap_types[] = {
	"RAW"
, "SIMPLE EX"
, "BOOLEAN"
, "LOOPSTART"
, "LOOPEND"
, "COMPLEX"
, "EACH KEY"
, "EXECUTE"
, "TERMINAT"
, "UNINITIA"
};

//Dump different map types
const char * print_xmap_type ( short type ) {
	const char *a;
	( type == RW ) ? a = xmap_types[ 0 ] : 0;
	( type == SX ) ? a = xmap_types[ 1 ] : 0;
	( type == BL ) ? a = xmap_types[ 2 ] : 0;
	( type == LS ) ? a = xmap_types[ 3 ] : 0;
	( type == LE ) ? a = xmap_types[ 4 ] : 0;
	( type == CX ) ? a = xmap_types[ 5 ] : 0;
	( type == EK ) ? a = xmap_types[ 6 ] : 0;
	( type == EX ) ? a = xmap_types[ 7 ] : 0;
	( type == TM ) ? a = xmap_types[ 8 ] : 0;
	( type == UN ) ? a = xmap_types[ 9 ] : 0;
	return a;
};

//Dump premaps
void print_premap ( struct premap **p ) {
	while ( p && *p ) {
		fprintf( stderr, "%p => %3d, ", *p, (*p)->len );
		write( 2, (*p)->ptr, (*p)->len < XMAP_DUMP_LEN ? (*p)->len : XMAP_DUMP_LEN );
		write( 2, "...\n", 4 );
		p++;
	}
}


//Dump maps 
void print_xmap ( struct xmap **p ) {
	while ( p && *p ) {
		fprintf( stderr, "%p => ", *p );
		fprintf( stderr, "type: %-10s, ", print_xmap_type( (*p)->type ) );
		fprintf( stderr, "len : %4d, ", (*p)->len );
		fprintf( stderr, "par: %-15p, ", (*p)->parent );
		fprintf( stderr, "con: " );
		write( 2, (*p)->ptr, (*p)->len < XMAP_DUMP_LEN ? (*p)->len : XMAP_DUMP_LEN );
		write( 2, "...\n", 4 );
		if ( (*p)->parent ) {
			struct xdesc *pp = (*p)->parent;
			fprintf( stderr, "\t\t\t\t\t" );
			fprintf( stderr, "children: %3d, ", pp->children );
			fprintf( stderr, "index: %3d, ", pp->index );
			fprintf( stderr, "%p\n", pp->pxmap );
		}
		p++;
	}
}
#endif
