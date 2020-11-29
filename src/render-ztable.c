#include "render-ztable.h"

#ifdef DEBUG_H
static int di = 0;
static char rprintchar[127] = {0};
static char * replace_chars ( char *src, int srclen ) {
	char *srcsrc = src;
	while ( srclen ) {
		( *srcsrc == '\n' || *srcsrc == '\r' ) ? *srcsrc = '|' : 0; 
		srclen--, srcsrc++;
	}
	return src;
}
#endif


//Didn't I write something to add to a buffer?
void extract_table_value ( zKeyval *lt, uint8_t **ptr, int *len, uint8_t *t, int tl ) {
	if ( lt->value.type == LITE_TXT ) {
		*len = strlen( lt->value.v.vchar ); 
		*ptr = (uint8_t *)lt->value.v.vchar;
	}
	else if ( lt->value.type == LITE_BLB ) {
		*len = lt->value.v.vblob.size; 
		*ptr = lt->value.v.vblob.blob;
	}
	else if ( lt->value.type == LITE_INT ) {
		*len = snprintf( (char *)t, tl - 1, "%d", lt->value.v.vint );
		*ptr = (uint8_t *)t;
	}
	else {
		//If for some reason we can't convert, just use a blank value.
		*len = 0;
		*ptr = (uint8_t *)"";
	}
}


MAPPER(map_raw_extract) {
	RPRINTF( "\nRAW", ptr, len ); 
	row->ptr = ptr;
	row->len = len;
	row->action = RAW;
}


MAPPER(map_simple_extract) {
	RPRINTF( "\nSIMPLE EXTRACT", ptr, len ); 
	zTable *tt = (zTable *)t;
	int hlen=0, hash = lt_get_long_i( tt, ptr, len ); 
	FPRINTF( "\nHASH: %d\n", hash ); 
	if ( hash > -1 ) {
		add_item( &row->hashList, zrender_copy_int( hash ), int *, &hlen );
	}
	row->action = SIMPLE_EXTRACT; 
}


//...
MAPPER(map_loop_start) { 
	int hash = -1;
	int hlen = 0;
	int blen = 0;
	int element_count = 0;
	uint8_t bbuf[ 2048 ] = { 0 };
	zTable *tt = (zTable *)t;
	RPRINTF( "LOOP_START", ptr, len );

	//If a parent should exist, copy the parent's text 
	if ( !( *parent ) ) {
FPRINTF( "no parent\n" );
		//Copy the data
		memcpy( bbuf, ptr, blen = len );

		//Get the hash
		if ( ( hash = lt_get_long_i( tt, bbuf, blen ) ) > -1 ) {
			add_item( &row->hashList, zrender_copy_int( hash ), int *, &hlen );
			row->children = element_count = lt_counti( tt, hash );
		}
	}
	else {
FPRINTF( "there is a parent\n" );
		//Get a count of the number of elements in the parent.
		int maxCount = 0;
		struct map **cp = &( *parent )[ *plen - 1 ];
FPRINTF( "struct map: %p", cp );
#if 0

		//Save all of the root elements
		for ( int i=0, cCount=0; i < cp->children; i++ ) {
	#if 0
			//All of this is stupid, it should just be:
			snprintf( this, thislen, "%s.%d.%s", (*parent)->text, num, p );
	#else
			char num[ 64 ] = { 0 };
			int numlen = snprintf( num, sizeof( num ) - 1, ".%d.", i );	
			memcpy( bbuf, cp->ptr, cp->len );
			blen = cp->len;
			memcpy( &bbuf[ blen ], num, numlen );
			blen += numlen;
			memcpy( &bbuf[ blen ], ptr, len );
			blen += len;
			hash = lt_get_long_i( tt, bbuf, blen );
			add_item( &row->hashList, zrender_copy_int( hash ), int *, &hlen ); 
		
			if ( hash > -1 && (cCount = lt_counti( tt, hash )) > maxCount ) {
				maxCount = cCount;	
			}
	#endif
		}
	#if 0
		cp = pp[ pplen - 1 ];

		//
		for ( int i=0, cCount=0; i < cp->childCount; i++ ) {
			char num[ 64 ] = { 0 };
			int numlen = snprintf( num, sizeof( num ) - 1, ".%d.", i );	

			//Copy to static buffer
			memcpy( bbuf, cp->text, cp->len );
			blen = cp->len;
			memcpy( &bbuf[ blen ], num, numlen );
			blen += numlen;
			memcpy( &bbuf[ blen ], p, alen );
			blen += alen;
			hash = lt_get_long_i(t, bbuf, blen );
			add_item( &row->hashList, zrender_copy_int( hash ), int *, &hashListLen ); 
		
			if ( hash > -1 && (cCount = lt_counti( t, hash )) > maxCount ) {
				maxCount = cCount;	
			}
		}
	#endif
		row->len = eCount = maxCount;
#endif
	}

	//Find the hash
	if ( hlen ) {
		struct map *np = malloc( sizeof( struct map ) );
		if ( !np ) {
			//Free and destroy things
			return;
		}

		//NOTE: len will contain the number of elements to loop
		memset( np, 0, sizeof( struct map ) );
		np->action = 0; 
		np->hashList = NULL; 
		np->children = element_count;
		np->len = blen;
		np->ptr = ptr; 
		add_item( parent, np, struct parent *, plen );
	}
}


MAPPER(map_complex_extract) {
	RPRINTF( "\nCOMPLEX", ptr, len );
	FPRINTF( "plen: %d\n", *plen );
	if ( *plen ) {
		int c = 0;
		int hlen = 0;
		int pos = 0;
		struct map **w = &( *parent )[ *plen - 1 ]; FPRINTF( "ptr: %p, len: %d\n", w, *plen );
		FPRINTF( "pos: %d, children: %d\n", (*w)->pos, (*w)->children );
 
		while ( (*w)->pos < (*w)->children ) {
#if 1
			//Move to the next block or build a sequence
			if ( c < (*plen - 1) ) {
				w++, c++;
				continue;
			}
			
			//Generate the hash strings
			if ( 1 ) {
			struct map **xx = *parent;
			uint8_t tr[ 2048 ] = { 0 };
			int trlen = 0;
			
			for ( int ii=0; ii < *plen; ii++ ) {
				memcpy( &tr[ trlen ], (*xx)->ptr, (*xx)->len );
				trlen += (*xx)->len;
				trlen += sprintf( (char *)&tr[ trlen ], ".%d.", (*xx)->pos );
				xx++;
			}
			memcpy( &tr[ trlen ], ptr, len );
			trlen += len;

			//TODO: Replace me with zrender_copy_int or a general copy_ macro
			//Check for this hash, save each and dump the list...
			int hh = lt_get_long_i( t, tr, trlen );
			add_item( &row->hashList, zrender_copy_int( hh ), int *, &hlen ); 
			}

			//Increment the number 
			while ( 1 ) {
				(*w)->pos++;
				//printf( "L%d %d == %d, STOP", c, (*w)->a, (*w)->b );
				if ( c == 0 )
					break;
				else { // ( c > 0 )
					if ( (*w)->pos < (*w)->children ) 
						break;
					else {
						(*w)->pos = 0;
						w--, c--;
					}
				}
			}
#endif
		}
		(*w)->pos = 0;
	}
}


//....
MAPPER(map_loop_end) {
	RPRINTF( "LOOP_END", ptr, len );
	FPRINTF( "Parent: %p  length: %d\n", parent, *plen );
	if ( *parent ) {
		//free( &( *parent )[ *plen ] );
		parent--; 
		(*plen)--;
		FPRINTF( "Parent: %p, length: %d\n", *parent, *plen );
	}
}


//....
EXTRACTOR(extract_raw) {
	FPRINTF( "%-20s, len: %3d\n", "xRAW", (**row)->len );
	append_to_uint8t( dst, dlen, src, len );
}

EXTRACTOR(extract_loop_start) {
	FPRINTF( "%-20s, %d\n", "xLOOP_START", (**row)->len );
	//struct dep *d = (struct dep * )t;
	(*ptr)++;
	(*ptr)->index = &(**row);
	(*ptr)->current = 0;
	(*ptr)->children = (**row)->children;
}

EXTRACTOR(extract_loop_end) {
	FPRINTF( "%-20s\n", "xLOOP_END" );
	(*ptr)->current++;
	FPRINTF( "%d ?= %d\n", (*ptr)->current, (*ptr)->children );
	FPRINTF( "%p ?= %p\n", (**row), (*ptr)->index );

	if ( (*ptr)->current == (*ptr)->children )
		(*ptr)--;
	else {
		*row = (*ptr)->index;
	}

	//FPRINTF( "%p ?= %p\n", (**row), (*ptr)->index );
}

EXTRACTOR(extract_simple_extract) {
	FPRINTF( "%-20s\n", "xSIMPLE_EXTRACT" );
	zTable *tt = (zTable *)t;
	int hash = 0;
	//rip me out
	if ( (**row)->hashList ) {
		//Get the type and length
		if ( ( hash = **( (**row)->hashList) ) > -1 ) {
			zKeyval *lt = lt_retkv( tt, hash );
			uint8_t *ptr = NULL, nbuf[64] = {0};
			int itemlen = 0;
			extract_table_value( lt, &ptr, &itemlen, nbuf, sizeof(nbuf) );
			append_to_uint8t( dst, dlen, ptr, len );
		}
		(**row)->hashList++;
	}
}

EXTRACTOR(extract_complex_extract) {
	FPRINTF( "%-20s\n", "xCOMPLEX EXTRACT" );
	if ( (**row)->hashList ) {
		//If there is a pointer, it does not move until I get through all three
		FPRINTF( "Getting entry: %d\n", **( (**row)->hashList) );
		int **list = (**row)->hashList;
		int hash = 0;
		//Get the type and length
		if ( ( hash = **list ) > -1 ) {
			zKeyval *lt = lt_retkv( t, hash );
			//NOTE: At this step, nobody should care about types that much...
			uint8_t *iptr = NULL, nbuf[ 64 ] = { 0 };
			int itemlen = 0;
			extract_table_value( lt, &iptr, &itemlen, nbuf, sizeof(nbuf) ); 
			append_to_uint8t( dst, dlen, iptr, itemlen );
		}
		(**row)->hashList++;
	}
}


int * zrender_copy_int ( int i ) {
	int *h = malloc( sizeof ( int ) );
	memcpy( h, &i, sizeof( int ) );
	return h; 
}


//Initialize the object
zRender * zrender_init() {
	zRender *zr = malloc( sizeof( zRender ) );
	if ( !zr ) {
		return NULL;
	}

	if ( !memset( zr, 0, sizeof( zRender ) ) ) {
		return NULL;
	}

	return zr;
}


//Use the default templating language (mustache)
void zrender_set_default_dialect( zRender *rz ) {
	zrender_set_boundaries( rz, "{{", "}}" );
	zrender_set( rz, '#', map_loop_start, extract_loop_start ); 
	zrender_set( rz, '/', map_loop_end, extract_loop_end ); 
	zrender_set( rz, 0, map_raw_extract, extract_raw ); 
	zrender_set( rz, '.', map_complex_extract, extract_complex_extract ); 
#if 0
	//Simple extracts are anything BUT the other characters (but raw is also 1, so...)
	zrender_set( rz, ' ', map_simple_extract, extract_simple_extract ); 
	zrender_set( rz, '!', map_boolean, extract_boolean ); 
	zrender_set( rz, '`', map_execute, extract_execute ); 
#endif
}


//...
struct map * init_map () {
	struct map *rp = malloc( sizeof( struct map ) );
	if ( !rp ) {
		//Free and destroy things
		return NULL;
	}

	memset( rp, 0, sizeof( struct map ) );
	rp->action = 0; 
	rp->ptr = NULL; 
	rp->len = 0; 
	rp->hashList = NULL; 
	return rp;
}


//Set start and optional end boundaries of whatever language is chosen.
void zrender_set_boundaries ( zRender *rz, const char *s, const char *end ) {
	if ( s ) {
		rz->zStart = strdup( s );	
	}
	if ( end ) {
		rz->zEnd = strdup( end );
	}
}


//Set the source for fetching data
void zrender_set_fetchdata( zRender *rz, void *t ) { 
	rz->userdata = t;	
}


//Set each character for replacement (we assume that it's just one)
void zrender_set( zRender *rz, const char map, Mapper mp, Extractor xp ) {
	struct zrSet *record = malloc( sizeof( struct zrSet ) );
	record->mapper = mp;
	record->extractor = xp;
	rz->mapset[ (int)map ] = record;
}


//Trim an unsigned character block 
uint8_t *zrender_trim ( uint8_t *msg, const char *trim, int len, int *nlen ) {
	//Define stuff
	//uint8_t *m = msg;
	uint8_t *forwards = msg;
	uint8_t *backwards = &msg[ len - 1 ];
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
	*nlen = nl;
	return forwards;
}


//Check that the data is balanced.
int zrender_check_balance ( zRender *rz, const uint8_t *src, int srclen ) {

	//This is the syntax to check for...
	zWalker r;
	memset( &r, 0, sizeof( zWalker ) );
	//Check these counts at the end...	
	int startList = 0, endList = 0;

	//just check that the list is balanced
	while ( memwalk( &r, src, (uint8_t *)"{}", srclen, 2 ) ) {
		if ( r.size == 0 ) {
			if ( r.chr == '{' ) {
				startList ++;
			}	
			else if ( r.chr == '}' ) {
				endList ++;
			}	
		}	
	}

	FPRINTF( "%s: %d ?= %d\n", __func__, startList, endList );
	return ( startList == endList );
}


//Convert userdata to an array map
struct map ** zrender_userdata_to_map ( zRender *rz, const uint8_t *src, int srclen ) {
	struct map **rr = NULL ; 
	struct parent **pp = NULL;
	struct map **pr = NULL;
	int rrlen = 0;
	int pplen = 0;
	zWalker r;
	memset( &r, 0, sizeof( zWalker ) );

	//The check map is now dynamically generated
	uint8_t check[] = { rz->zStart[0], rz->zEnd[0] };
	int checklen = 2;//sizeof(check);

	//Allocating a list of characters to elements is easiest.
	while ( memwalk( &r, (uint8_t *)src, check, srclen, checklen ) ) {
		//More than likely, I'll always use a multi-byte delimiter
		//FPRINTF( "MOTION == %s\n", DUMPACTION( rp->action ) );
		struct zrSet *z = NULL; 
		if ( r.size == 0 && r.chr == '{' ) {
		}
		else if ( r.chr != '}' ) {
			//We can simply copy if ACTION & BLOCK are 0 
			struct map *rp = init_map();
			rp->action = 0;  
			if ( ( z = rz->mapset[ 0 ] ) ) {
				z->mapper( rp, NULL, NULL, (uint8_t *)&src[ r.pos ], r.size, rz->userdata );
				add_item( &rr, rp, struct map *, &rrlen );
			}
		}
		else if ( src[ r.pos + r.size + 1 ] == '}' ) {
			//Start extraction...
			int alen=0, nlen = 0;	
			struct map *rp = init_map();
			uint8_t *p = zrender_trim( (uint8_t *)&src[ r.pos ], " ", r.size, &nlen );
			rp->action = *p;  

			if ( ( z = rz->mapset[ *p ] ) ) {
				//This should probably return some kind of error...
				p = zrender_trim( p, ". #/$`!\t", nlen, &alen );

				z->mapper( rp, &pr, &pplen, p, alen, rz->userdata ); 
				add_item( &rr, rp, struct map *, &rrlen );
			}
		}
	}

#if 0
	//Move through each of the rows 
	zrender_print_table( rr );
	//Destroy the parent list
	free( pp );
#endif
	return rr;
}



//Merge the values referenced in the map array into an unsigned character block
uint8_t *zrender_map_to_uint8t ( zRender *rz, struct map **xmap, int *newlen ) {
	//...
	uint8_t *block = NULL;
	int blocklen = 0;
	//struct dep depths[10] = { { 0, 0, 0 } };
	//struct dep *d = depths;
	struct ptr *ptr, mptr[10] = { { 0, 0, 0 } };
	ptr = mptr;

	//...
	struct map **map = xmap;

	while ( *map ) {
		struct map *rp = *map;
		struct zrSet *z = NULL; 
		if ( ( z = rz->mapset[ (int)rp->action ] ) ) {
			z->extractor( &map, &block, &blocklen, rp->ptr, rp->len, &ptr, rz->userdata );
		}
		map++;
	}

	//The final step is to assemble everything...
	*newlen = blocklen;
	return block;
}


//Do all the steps to make templating quick and easy.
uint8_t *zrender_render( zRender *rz, const uint8_t *src, int srclen, int *newlen ) {

	//Define things
	struct map **map = NULL;
	uint8_t *block = NULL;
	int blocklen = 0;

	//TODO: Mark the place where the thing is undone
	if ( !zrender_check_balance( rz, src, srclen ) ) {
		FPRINTF( "Syntax at supplied template is wrong..." );
		return NULL;
	}

	//TODO: Be sure to catch errors when mapping (like things aren't there or something)
	if ( !( map = zrender_userdata_to_map( rz, src, srclen ) ) ) {
		return NULL;
	}

#ifdef DEBUG_H
	zrender_print_map( map );
#endif

	//TODO: Same to catch errors here...
	if ( !( block = zrender_map_to_uint8t( rz, map, &blocklen ) ) ) {
		return NULL;
	}

#if 1
	zrender_free( map );
#endif

	*newlen = blocklen;
	return block; 
}


//Destroy the zRender structure
void zrender_free( struct map **map ) {
	struct map **top = map;

	while ( *map ) {
		struct map *item = *map;

		//Dump the unchanging elements out...
		FPRINTF( "[%3d] => action: %-16s", di++, DUMPACTION( item->action ) );

		if ( item->action == RAW ) { 
			FPRINTF( "Nothing to free...\n" );
		}
		else if ( item->action == EXECUTE ) {
			FPRINTF( "Freeing pointer to exec content..." );
			free( item->ptr );
		}
		else {
			FPRINTF( "Freeing int lists..." );
			int **ii = item->hashList;
			while ( ii && *ii ) {
				fprintf( stderr, "item->intlist: %p\n", *ii );	
				free( *ii ); 
				ii++;
			}
			free( item->hashList );
			FPRINTF( "\n" );
		}
		free( item );
		map++;
	}

	free( top );
}


#ifdef DEBUG_H
//Purely for debugging, see what came out
int zrender_print_map( struct map **map ) {
	if ( !map ) {
		return 0;
	}
	while ( *map ) {
		struct map *item = *map;

		//Dump the unchanging elements out...
		FPRINTF( "[%3d] => action: %-16s", di++, DUMPACTION( item->action ) );

		if ( item->action == RAW || item->action == EXECUTE ) {
			uint8_t *p = (uint8_t *)item->ptr;
			fprintf( stderr, " len: %3d, ", item->len ); 
			write( 2, p, item->len );
			fprintf( stderr, "\n" );
		}
		else {
		#ifdef DEBUG_H
			fprintf( stderr, " item: " );
			//( item->word ) ? write( 2, item->word, item->wordlen ) : 0;
			fprintf( stderr, "," );
		#endif
			fprintf( stderr, " len: %3d, list: %p => ", item->len, item->hashList );
			int **ii = item->hashList;
			if ( !ii ) {
				fprintf( stderr , "NULL" );
			}
			else {
				int d=0;
				while ( *ii ) {
					fprintf( stderr, "%c%d", d++ ? ',' : ' ',  **ii );
					ii++;
				}
			}
			fprintf( stderr, "\n" );
		}
		map++;
	}
	return 1;
}
#endif
