/* ---------------------------------------------------
render.c 

Expected results


TODO / TASKS
------------
- Rewrite to support error messages (print to buffer and call it a day)

- Rewrite so that each of the handlers uses function pointers, will be easier
	to maintain and add to later.  

- Get nested complex extracts to work (or for that matter, nested anythings)

- Make a cleaner map? for debugging (it's so long, and I can't read it)


USAGE
-----
The template tag rules look a little like this.  One character will be used
to determine how the result is extracted.

{{ # xxx }} - LOOP START
{{ / xxx }} - LOOP END 
{{ x     }} - SIMPLE EXTRACT
{{ .     }} - COMPLEX EXTRACT
{{ $     }} - EACH KEY OR VALUE IN A TABLE 


EXPERIMENTAL
-----------
{{ !xxx  }} - BOOLEAN? 
{{ `xxx` }} - EXECUTE 
{{ xxx ? y : z }} - TERNARY

 * --------------------------------------------------- */
#include "render.h"

#define DUMPACTION( NUM ) \
	( NUM == LOOP_START ) ? "LOOP_START" : \
	( NUM == LOOP_END ) ? "LOOP_END" : \
	( NUM == COMPLEX_EXTRACT ) ? "COMPLEX_EXTRACT" : \
	( NUM == SIMPLE_EXTRACT ) ? "SIMPLE_EXTRACT" : \
	( NUM == EACH_KEY ) ? "EACH_KEY" : \
	( NUM == EXECUTE ) ? "EXECUTE" : \
	( NUM == BOOLEAN ) ? "BOOLEAN" : \
	( NUM == RAW ) ? "RAW" : "UNKNOWN" 

const int LOOP_START = 30;
const int LOOP_END = 31;
const int SIMPLE_EXTRACT = 32;
const int COMPLEX_EXTRACT = 33;
const int EACH_KEY = 34;
const int EXECUTE = 35;
const int BOOLEAN = 36;
const int RAW = 37;
const int BLOCK_START = 0;
const int BLOCK_END = 0;
const int TERM = -2;
const int UNINIT = 166;
const int maps[] = {
	['#'] = LOOP_START,
	['/'] = LOOP_END,
	['.'] = COMPLEX_EXTRACT,
	['$'] = EACH_KEY, 
	['`'] = EXECUTE, //PAIR_EXTRACT
	['!'] = BOOLEAN,
	[254] = RAW,
	[255] = 0
};


struct parent { 
	uint8_t *text; 
	int len, pos, childCount; 
}; 

struct dep { 
	struct map **index; 
	int current, childCount; 
};

struct map { 
	int action; 
	int **hashList; 
	int len; 
	void *ptr; 
};


//Purely for debugging, see what came out
void print_render_table( struct map **map ) {
	int i=0;
	while ( *map ) {
		struct map *item = *map;

		//Dump the unchanging elements out...
		FPRINTF( "[%3d] => action: %-16s", i++, DUMPACTION( item->action ) );

		if ( item->action == RAW || item->action == EXECUTE ) {
			uint8_t *p = (uint8_t *)item->ptr;
			FPRINTF( " len: %3d, ", item->len ); 
			ENCLOSE( p, 0, item->len );
		}
		else {
		#ifdef DEBUG_H
			fprintf( stderr, " item: " );
			//( item->word ) ? write( 2, item->word, item->wordlen ) : 0;
			fprintf( stderr, "," );
		#endif
			FPRINTF( " len: %3d, list: %p => ", item->len, item->hashList );
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
}


void destroy_render_table( struct map **map ) {
	int i = 0;
	struct map **top = map;

	while ( *map ) {
		struct map *item = *map;

		//Dump the unchanging elements out...
		FPRINTF( "[%3d] => action: %-16s", i++, DUMPACTION( item->action ) );

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


int * copy_int ( int i ) {
	int *h = malloc( sizeof ( int ) );
	memcpy( h, &i, sizeof( int ) );
	return h; 
}

#if 0
//These are all the standard mappers and extractors...
void map_loop_start() {
}
void map_loop_end() {
}
void map_complex_extract() {
}
void map_simple_extract() {
}
void map_each_key() {
}
void map_execute() {
}
void map_boolean() {
}

void extract_loop_start() {
}
void extract_loop_end() {
}
void extract_complex_extract() {
}
void extract_simple_extract() {
}
void extract_each_key() {
}
void extract_execute() {
}
void extract_boolean() {
}
#endif



struct map * init_map () {
	struct map *rp = malloc( sizeof( struct map ) );
	if ( !rp ) {
		//Free and destroy things
		return NULL;
	}

	rp->action = 0; 
	rp->ptr = NULL; 
	rp->len = 0; 
	rp->hashList = NULL; 
	return rp;
}


struct parent * init_parent() {
	struct parent *np = NULL; 
	if (( np = malloc( sizeof(struct parent) )) == NULL ) {
		return NULL;
	}

	//NOTE: len will contain the number of elements to loop
	np->childCount = 0;
	np->pos = 0;
	np->len = 0;
	np->text = 0;
	return np; 
}


//Bitmasking will tell me a lot...
struct map **table_to_map ( zTable *t, const uint8_t *src, int srclen ) {

	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
	int SKIP = 0;
	int INSIDE = 0;
	struct map **rr = NULL ; 
	struct parent **pp = NULL;
	int rrlen = 0;
	int pplen = 0;
	zWalker r;
	memset( &r, 0, sizeof( zWalker ) );

	//Allocating a list of characters to elements is easiest.
	while ( memwalk( &r, (uint8_t *)src, (uint8_t *)"{}", srclen, 2 ) ) {
		//More than likely, I'll always use a multi-byte delimiter
		if ( r.size == 0 ) {
			//Check if there is a start or end block
			if ( r.chr == '{' ) {
				BLOCK = BLOCK_START;
			}
		}
		else if ( r.chr == '}' ) {
			if ( src[ r.pos + r.size + 1 ] == '}' ) {
				//Start extraction...
				BLOCK = BLOCK_END;
				int nlen = 0;	
				int hashListLen = 0;
				uint8_t *p = trim( (uint8_t *)&src[r.pos], " ", r.size, &nlen );
				struct map *rp = init_map();

				//Extract the first character
				if ( !maps[ *p ] ) {
					rp->action = SIMPLE_EXTRACT; 
					int hash = lt_get_long_i( t, p, nlen ); 
					if ( hash > -1 ) {
						add_item( &rp->hashList, copy_int( hash ), int *, &hashListLen );
					}
				}
				else {
					//Advance and reset p b/c we need just the text...
					int alen = 0;
					rp->action = maps[ *p ];
					p = trim( p, ". #/$`!\t", nlen, &alen );
					FPRINTF("GOT ACTION %s, and TEXT = ", DUMPACTION(rp->action)); ENCLOSE( p, 0, alen );

					//Figure some things out...
					if ( rp->action == LOOP_START ) {
						FPRINTF( "@LOOP_START - " );	
						int hash = -1;
						int blen = 0;
						int eCount = 0;
						uint8_t bbuf[ 2048 ] = { 0 };
						struct parent *cp = NULL;

						//If a parent should exist, copy the parent's text 
						if ( !INSIDE ) {
							//Copy the data
							memcpy( &bbuf[ blen ], p, alen );
							blen += alen;
							if ( ( hash = lt_get_long_i(t, bbuf, blen) ) > -1 ) {
								add_item( &rp->hashList, copy_int( hash ), int *, &hashListLen );
								eCount = lt_counti( t, hash );
							}
						}
						else {
							//Get a count of the number of elements in the parent.
							int maxCount = 0;
							cp = pp[ pplen - 1 ];

							for ( int i=0, cCount=0; i < cp->childCount; i++ ) {
								char num[ 64 ] = { 0 };
								uint8_t nbuf[ 2048 ] = { 0 };
								int numlen = snprintf( num, sizeof( num ) - 1, ".%d.", i );	

								//Copy to static buffer
								memcpy( bbuf, cp->text, cp->len );
								blen = cp->len;
								memcpy( &bbuf[ blen ], num, numlen );
								blen += numlen;
								memcpy( &bbuf[ blen ], p, alen );
								blen += alen;
								hash = lt_get_long_i(t, bbuf, blen );
								add_item( &rp->hashList, copy_int( hash ), int *, &hashListLen ); 
							
								if ( hash > -1 && (cCount = lt_counti( t, hash )) > maxCount ) {
									maxCount = cCount;	
								}
							}
							eCount = maxCount;
						}

						rp->len = eCount;

						//Find the hash
						if ( hashListLen ) {
							struct parent *np = init_parent(); 
							//NOTE: len will contain the number of elements to loop
							np->childCount = eCount;
							np->len = alen;
							np->text = p; 
							add_item( &pp, np, struct parent *, &pplen );
							INSIDE++;
						}
					}
					else if ( rp->action == LOOP_END ) {
						//If inside is > 1, check for a period, strip it backwards...
						FPRINTF( "@LOOP_END - " );
						//TODO: Check that the hashes match instead of just pplen
						//rp->hash = lt_get_long_i( t, p, alen );
						if ( !INSIDE )
							;
						else if ( pplen == INSIDE ) {
							free( pp[ pplen - 1 ] );
							pplen--;
							INSIDE--;
						}
					}
					else if ( rp->action == COMPLEX_EXTRACT ) {
						FPRINTF( "@COMPLEX_EXTRACT - " );
						if ( pplen ) {
							struct parent **w = pp;
							int c = 0;
							while ( (*w)->pos < (*w)->childCount ) {
								//Move to the next block or build a sequence
								if ( c < (pplen - 1) ) {
									w++, c++;
									continue;
								}
								
								//Generate the hash strings
								if ( 1 ) {
									struct parent **xx = pp;
									uint8_t tr[ 2048 ] = { 0 };
									int trlen = 0;
									
									for ( int ii=0; ii < pplen; ii++ ) {
										memcpy( &tr[ trlen ], (*xx)->text, (*xx)->len );
										trlen += (*xx)->len;
										trlen += sprintf( (char *)&tr[ trlen ], ".%d.", (*xx)->pos );
										xx++;
									}
									memcpy( &tr[ trlen ], p, alen );
									trlen += alen;

									//TODO: Replace me with copy_int or a general copy_ macro
									//Check for this hash, save each and dump the list...
									int hh = lt_get_long_i( t, tr, trlen );
#if 1
									add_item( &rp->hashList, copy_int( hh ), int *, &hashListLen ); 
#else									
									int *h = malloc( sizeof(int) );
									memcpy( h, &hh, sizeof(int) );	
									add_item( &rp->hashList, h, int *, &hashListLen ); 
#endif
									FPRINTF( "string = %s, hash = %3d, ", tr, hh );	
								}

								//Increment the number 
								while ( 1 ) {
									(*w)->pos++;
									//printf( "L%d %d == %d, STOP", c, (*w)->a, (*w)->b );
									if ( c == 0 )
										break;
									else { // ( c > 0 )
										if ( (*w)->pos < (*w)->childCount ) 
											break;
										else {
											(*w)->pos = 0;
											w--, c--;
										}
									}
								}
							}
							(*w)->pos = 0;
						}
					}
					else if ( rp->action == EACH_KEY ) {
						FPRINTF( "@EACH_KEY :: Nothing yet...\n" );
					}
					else if ( rp->action == EXECUTE ) {
						FPRINTF( "@EXECUTE :: Nothing yet...\n" );
					}
					else if ( rp->action == BOOLEAN ) {
						FPRINTF( "@BOOLEAN :: Nothing yet...\n" );
					}
				}

				add_item( &rr, rp, struct map *, &rrlen );
			}
		}
		else {
			FPRINTF( "@RAW BLOCK COPY" ); 
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
#if 1
				struct map *rp = init_map();
#else
				struct map *rp = malloc( sizeof( struct map ) );
				if ( !rp ) {
					//Teardown and destroy
					return NULL;
				}
#endif	
				//Set defaults
				rp->len = r.size;	
				rp->action = RAW;
				rp->ptr = (uint8_t *)&src[ r.pos ];	
				
				//Save a new record
				add_item( &rr, rp, struct map *, &rrlen );
			}	
		}
	}

	//Destroy the parent list
	free( pp );
	return rr;
}


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


uint8_t *map_to_uint8t ( zTable *t, struct map **map, int *newlen ) {
	//Start the writes, by using the structure as is
	uint8_t *block = NULL;
	int blockLen = 0;
	struct dep depths[100] = { { 0, 0, 0 } };
	struct dep *d = depths;

	while ( *map ) {
		struct map *item = *map;
		int hash = -1;

		if ( item->action == RAW || item->action == EXECUTE ) {
			FPRINTF( "%-20s, len: %3d", "RAW", item->len );
			append_to_uint8t( &block, &blockLen, item->ptr, item->len );
		}
		else if ( item->action == SIMPLE_EXTRACT ) {
			FPRINTF( "%-20s, len: %3d ", "SIMPLE_EXTRACT", item->len );
			//rip me out
			if ( item->hashList ) {
				//Get the type and length
				if ( ( hash = **item->hashList ) > -1 ) {
					zKeyval *lt = lt_retkv( t, hash );
					//NOTE: At this step, nobody should care about types that much...
					uint8_t *ptr = NULL, nbuf[64] = {0};
					int itemlen = 0;
					extract_table_value( lt, &ptr, &itemlen, nbuf, sizeof(nbuf) );
					append_to_uint8t( &block, &blockLen, (uint8_t *)ptr, itemlen );
				}
				item->hashList++;
			}
			FPRINTF( "\n" );
		}
		else {
			if ( item->action == LOOP_START ) {
				FPRINTF( "%-20s, len: %3d", "LOOP_START", item->len );
				d++;
				d->index = map;
				d->current = 0;
				d->childCount = item->len;
			}
			else if ( item->action == LOOP_END ) {
				FPRINTF( "%-20s, %d =? %d", "LOOP_END", d->current, d->childCount );
				if ( ++d->current == d->childCount )
					d--;
				else {
					map = d->index;
				}
			}
			else if ( item->action == COMPLEX_EXTRACT ) {
				FPRINTF( "%-20s", "COMPLEX_EXTRACT " );
				//rip me out, i am idential to SIMPLE_EXTRACT'S ROUTINE
				if ( item->hashList ) {
					//If there is a pointer, it does not move until I get through all three
					FPRINTF( "List?: %d", **item->hashList );
					int **list = item->hashList;
					//Get the type and length
					if ( ( hash = **list ) > -1 ) {
						zKeyval *lt = lt_retkv( t, hash );
						//NOTE: At this step, nobody should care about types that much...
						uint8_t *ptr = NULL, nbuf[ 64 ] = { 0 };
						int itemlen = 0;
						extract_table_value( lt, &ptr, &itemlen, nbuf, sizeof(nbuf) ); 
						append_to_uint8t( &block, &blockLen, ptr, itemlen );
					}
					list++; //item->hashList++;
				}
			}
		}
		fprintf( stderr, "%d, \n", blockLen );
		map++;
	}

	//The final step is to assemble everything...
	*newlen = blockLen;
	return block;
}



uint8_t *table_to_uint8t ( zTable *t, const uint8_t *src, int srclen, int *newlen ) {

	struct map **map = NULL;
	uint8_t *block = NULL;
	int blocklen = 0;

	//Convert T to a map
	if ( !( map = table_to_map( t, src, srclen ) ) ) {
		return NULL;
	}

#if 1
	//See the map 
	print_render_table( map );

	//Do the map	
	if ( !( block = map_to_uint8t( t, map, &blocklen ) ) ) {
		return NULL;
	}
#endif

	//Free the map
	//destroy_render_table( map );
	*newlen = blocklen;
	return block; 
}



