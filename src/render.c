/* ---------------------------------------------------
render.c 

Test out rendering...

TODO / TASKS
------------
- Get proper renders to work...
- Make it work with nested anything


Stuck on COMPLEX_EXTRACT.
- !!! Keeping the hashes still helps, so don't get rid of that...

1.
- Extend single.c to return a short key or full key from a location
(this way all of the hashes can be used)
2.
- Or just allocate individual strings with what you need at the parent
	2a.
	- Generate full strings at the COMPLEX_EXTRACT part
	- Full strings also have to be generated at LOOP_START

//{{ # xxx }} - LOOP START
//{{ / xxx }} - LOOP END 
//{{ x     }} - SIMPLE EXTRACT
//{{ .     }} - COMPLEX EXTRACT
//{{ $     }} - EACH KEY OR VALUE IN A TABLE 
//{{ `xxx` }} - EXECUTE 
//{{ !xxx  }} - BOOLEAN? 
//{{ xxx ? y : z }} - TERNARY

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


struct map { 
	int action; 
	int **hashList; 
	int len; 
	void *ptr; 
};


//Purely for debugging, see what came out
void print_render_table( struct map **map, int maplen ) {
	for ( int i=0; i < maplen; i++ ) {
		struct map *item = map[ i ];

		//Dump the unchanging elements out...
		FPRINTF( "[%3d] => action: %-16s", i, DUMPACTION( item->action ) );

		if ( item->action == RAW || item->action == EXECUTE ) {
			uint8_t *p = (uint8_t *)item->ptr;
			FPRINTF( " len: %3d, ", item->len ); 
			ENCLOSE( p, 0, item->len );
		}
		else {
			FPRINTF( " len: %3d, list: %p => ", item->len, item->hashList );
			int **ii = item->hashList;
			if ( !ii ) 
				fprintf( stderr , "NULL" );
			else {
				int d=0;
				while ( *ii ) {
					fprintf( stderr, "%c%d", d++ ? ',' : ' ',  **ii );
					ii++;
				}
			}
			fprintf( stderr, "\n" );
		}
	}
}


void destroy_render_table( struct map **map, int maplen ) {
	for ( int i=0; i < maplen; i++ ) {
		struct map *item = map[ i ];

		//Dump the unchanging elements out...
		FPRINTF( "[%3d] => action: %-16s", i, DUMPACTION( item->action ) );

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
#if 1
			while ( ii && *ii ) {
				fprintf( stderr, "item->intlist: %p\n", *ii );	
				//free( *ii ); 
				ii++;
			}
#else
			if ( ii ) {
				while ( *ii ) {
					free( *ii );
					ii++;
				}
			}
#endif
			FPRINTF( "\n" );
		}
		free( item );
	}
}


//Bitmasking will tell me a lot...
struct map **table_to_map ( Table *t, const uint8_t *src, int srclen, int *elen ) {
	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
	int SKIP = 0;
	int INSIDE = 0;
	struct map **rr = NULL ; 
	struct parent **pp = NULL;
	int rrlen = 0;
	int pplen = 0;
	Mem r;
	memset( &r, 0, sizeof( Mem ) );

	//Allocating a list of characters to elements is easiest.
	while ( memwalk( &r, (uint8_t *)src, (uint8_t *)"{}", srclen, 2 ) ) {
		//More than likely, I'll always use a multi-byte delimiter
		if ( r.size == 0 ) { /*&& r.pos > 0 ) {*/
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
				int **hashList = NULL;
				int hashListLen = 0;
				uint8_t *p = trim( (uint8_t *)&src[r.pos], " ", r.size, &nlen );
				struct map *rp = NULL; 
				if ( !(rp = malloc( sizeof( struct map ) )) ) {
					//Free and destroy things
					return NULL;
				}

				rp->action = 0; 
				rp->ptr = NULL; 
				rp->len = 0; 
				rp->hashList = NULL; 

				//Extract the first character
				if ( !maps[ *p ] ) {
					rp->action = SIMPLE_EXTRACT; 
					int *h = NULL; 
					int hash = -1;
					if ( (hash = lt_get_long_i(t, p, nlen) ) > -1 ) {
						( h = malloc( sizeof(int) ) ) && ( *h = hash );
						add_item( &hashList, h, int *, &hashListLen );
					}
				}
				else {
					//Advance and reset p b/c we need just the text...
					int alen = 0;
					rp->action = maps[ *p ];
					p = trim( p, ". #/$`!\t", nlen, &alen );
					FPRINTF("GOT ACTION %s, and TEXT = ", DUMPACTION(rp->action));
					ENCLOSE( p, 0, alen );

					//Figure some things out...
					if ( rp->action == LOOP_START ) {
						FPRINTF( "@LOOP_START - " );	
						int hash = -1;
						int blen = 0;
						int eCount = 0;
						uint8_t bbuf[ 2048 ] = { 0 };
						struct parent *cp = NULL;

						//If a parent should exist, copy the parent's text 
						//TODO: Eventually, numbers shouldn't be necessary on this check
						if ( !INSIDE ) {
							//Copy the data
							memcpy( &bbuf[ blen ], p, alen );
							blen += alen;
						
							FPRINTF( "Checking level[0] table " );	
							ENCLOSE( bbuf, 0, blen );
							//This is the only thing, get the hash and end it
							if ( (hash = lt_get_long_i( t, bbuf, blen ) ) > -1 ) {
								int *h = malloc( sizeof( int ) );
								memcpy( h, &hash, sizeof( int ) );
								add_item( &hashList, h, int *, &hashListLen ); 
								eCount = lt_counti( t, hash );
							}
						}
						else {
							//If there are 3 parents, you need to start at the beginning and come out
							//Find the MAX count of all the rows that are there... This way you'll have the right count everytime...
							FPRINTF( "Checking level[n+1] table " );	

							//Get a count of the number of elements in the parent.
							int maxCount = 0;
							cp = pp[ pplen - 1 ];
							FPRINTF( "containing %d members.\n", cp->childCount );
							FPRINTF( "\tParent strings are:\n" ); 

							for ( int i=0, cCount=0; i<cp->childCount; i++ ) {
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
							#ifdef DEBUG	
								//DEBUG: See the string to check for...	
								//write( 2, "\t\t'", 3 ); write( 2, bbuf, blen ); write( 2, "'", 1 );
							#endif
								//Check for the hash
								hash = lt_get_long_i(t, bbuf, blen ); 
								int *h = malloc( sizeof(int) );
								memcpy( h, &hash, sizeof( int ) );
								add_item( &hashList, &h, int *, &hashListLen ); 
								FPRINTF( "; hash is: %3d, ", hash );	
							
								if ( hash > -1 && (cCount = lt_counti( t, hash )) > maxCount ) {
									maxCount = cCount;	
									FPRINTF( " child count is: %3d\n", maxCount );	
								}
							}
							eCount = maxCount;
						}

						rp->len = eCount;

						//Find the hash
						if ( hashListLen ) {
							//Set up the parent structure
							struct parent *np = NULL; 
							if (( np = malloc( sizeof(struct parent) )) == NULL ) {
								//TODO: Cut out and free things
							}

							//NOTE: len will contain the number of elements to loop
							np->childCount = eCount;
							np->pos = 0;
							np->len = alen;
							np->text = p; 
							add_item( &pp, np, struct parent, &pplen );
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

									//Check for this hash, save each and dump the list...
									int hh = lt_get_long_i(t, tr, trlen ); 
									int *h = malloc( sizeof(int) );
									memcpy( h, &hh, sizeof(int) );	
									add_item( &hashList, h, int *, &hashListLen ); 
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

				//Create a new row with what we found.
			#if 1
				rp->hashList = !hashListLen ? NULL : hashList; 
			#else
				if ( !hashListLen )
					rp->hashList = NULL;
				else {
					rp->hashList = hashList; 
				}
			#endif
				
				add_item( &rr, rp, struct map, &rrlen );
			}
		}
		else {
			FPRINTF( "@RAW BLOCK COPY" ); 
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
				//struct map rbb = { 0 };
				struct map *rp = malloc( sizeof( struct map ) );
				if ( !rp ) {
					//Teardown and destroy
					return NULL;
				}
		
				//Set defaults
				rp->len = r.size;	
				//rp->hash = -2;
				rp->action = RAW;
				rp->hashList = NULL;
				rp->ptr = (uint8_t *)&src[ r.pos ];	
				
				//Save a new record
				add_item( &rr, rp, struct map, &rrlen );
			}	
		}
	}

	*elen = rrlen;
	return rr;
}


uint8_t *map_to_uint8t ( Table *t, struct map **map, int elen, int *newlen ) {
	//Start the writes, by using the structure as is
	uint8_t *block = NULL;
	int blockLen = 0;
	struct dep { int index, current, childCount; } depths[100] = { { 0, 0, 0 } };
	struct dep *d = depths;

	for ( int i = 0; i < elen; i++ ) {
		struct map *item = map[ i ];
		if ( item->action == RAW || item->action == EXECUTE ) {
			FPRINTF( "%-20s, len: %3d", "RAW", item->len );
			blockLen += item->len;
			if ( (block = realloc( block, blockLen )) == NULL ) {
				//NOTE: Teardown properly or you will cry...
				return NULL;
			}
			//If I do a memset, where at?
			memcpy( &block[ blockLen - item->len ], item->ptr, item->len ); 
		}
		else if ( item->action == SIMPLE_EXTRACT ) {
			FPRINTF( "%-20s, len: %3d ", "SIMPLE_EXTRACT", item->len );
			//rip me out
			#if 1
			if ( item->hashList ) {
				//Get the type and length
				int hash = **item->hashList;
				if ( hash > -1 ) {
					LiteKv *lt = lt_retkv( t, hash );
				#ifdef DEBUG
					fprintf( stderr, ", WHAT IS THIS: %p = ", lt ); 
					if ( lt->value.type == LITE_INT ) {
						fprintf( stderr, " %d", lt->value.v.vint );
					}
					else if ( lt->value.type == LITE_TXT ) {
						fprintf( stderr, " %s", lt->value.v.vchar );
					}
				#endif
					//NOTE: At this step, nobody should care about types that much...
					char *ptr = NULL;
					char nbuf[64] = {0};
					int itemlen = 0;
					if ( lt->value.type == LITE_TXT ) {
						itemlen = strlen( lt->value.v.vchar ); 
						ptr = lt->value.v.vchar;
					}
					else if ( lt->value.type == LITE_INT ) {
						itemlen = snprintf( nbuf, sizeof( nbuf ) - 1, "%d", lt->value.v.vint );
						ptr = nbuf;
					}
					else {
						//This is a totally different situation...
						return NULL;
					}

					blockLen += itemlen;
					if ( (block = realloc( block, blockLen )) == NULL ) {
						return NULL;
					}
					memcpy( &block[ blockLen - itemlen ], ptr, itemlen ); 
				}
				item->hashList++;
			}
			#endif
			FPRINTF( "\n" );
		} 
		else {
			if ( item->action == LOOP_START ) {
				FPRINTF( "%-20s, len: %3d", "LOOP_START", item->len );
				d++;
				d->index = i;
				d->current = 0;
				d->childCount = item->len;
			} 
			else if ( item->action == LOOP_END ) {
				d->current++;
				FPRINTF( "%-20s, %d =? %d", "LOOP_END", d->current, d->childCount );
				if ( d->current == d->childCount ) {
					d--;
				}
				else {
					i = d->index;
				}
			}
			else if ( item->action == COMPLEX_EXTRACT ) {
				FPRINTF( "%-20s", "COMPLEX_EXTRACT " );
				//rip me out, i am idential to SIMPLE_EXTRACT'S ROUTINE
				#if 1
				if ( item->hashList ) {
					//If there is a pointer, it does not move until I get through all three
					FPRINTF( "%d", **item->hashList );
					//Get the type and length
					int hash = **item->hashList;
					if ( hash > -1 ) {
						LiteKv *lt = lt_retkv( t, hash );
					#ifdef DEBUG
						fprintf( stderr, ", WHAT IS THIS: %p = ", lt ); 
						if ( lt->value.type == LITE_INT ) {
							fprintf( stderr, " %d", lt->value.v.vint );
						}
						else if ( lt->value.type == LITE_TXT ) {
							fprintf( stderr, " %s", lt->value.v.vchar );
						}
					#endif
						//NOTE: At this step, nobody should care about types that much...
						char *ptr = NULL;
						char nbuf[64] = {0};
						int itemlen = 0;
						if ( lt->value.type == LITE_TXT ) {
							itemlen = strlen( lt->value.v.vchar ); 
							ptr = lt->value.v.vchar;
						}
						else if ( lt->value.type == LITE_INT ) {
							itemlen = snprintf( nbuf, sizeof( nbuf ) - 1, "%d", lt->value.v.vint );
							ptr = nbuf;
						}
						else {
							//This is a totally different situation...
							return NULL;
						}

						blockLen += itemlen;
						if ( (block = realloc( block, blockLen )) == NULL ) {
							return NULL;
						}
						memcpy( &block[ blockLen - itemlen ], ptr, itemlen ); 
					}
					item->hashList++;
				}
				#endif
			}
		#ifdef DEBUG
			fprintf( stderr, "\n" );
		#endif
		}
	}

	//The final step is to assemble everything...
	*newlen = blockLen;
	return block;
}



uint8_t *table_to_uint8t ( Table *t, const uint8_t *src, int srclen, int *newlen ) {
	//Define shit...
	struct map **map = NULL;
	uint8_t *block = NULL;
	int blocklen = 0;
	int maplen = 0;

	//Convert T to a map
	if ( !( map = table_to_map( t, src, srclen, &maplen ) ) ) {
		return NULL;
	}

	//See the map 
	//print_render_table( map, maplen );

	//Do the map	
	if ( !( block = map_to_uint8t( t, map, maplen, &blocklen ) ) ) {
		return NULL;
	}

	//Free the map
	destroy_render_table( map, maplen );
	*newlen = blocklen;
	return block; 
}
