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

 * --------------------------------------------------- */
#include "vendor/single.h"
//#include "bridge.h"
#include "luabind.h"

#define SQROOGE_H

#define DEBUG 

#define ENCLOSE(SRC, POS, LEN) \
	write( 2, "'", 1 ); \
	write( 2, &SRC[ POS ], LEN ); \
	write( 2, "'\n", 2 );

#define CREATEITEM(TPTR,SIZE,SPTR,HASH,LEN) \
	struct rb *TPTR = malloc( sizeof( SIZE ) ); \
	memset( TPTR, 0, sizeof( SIZE ) ); \
	TPTR->len = LEN;	\
	TPTR->ptr = SPTR; \
	TPTR->hash = HASH; \
	TPTR->rbptr = NULL;

#define ADDITEM(TPTR,SIZE,LIST,LEN) \
	if (( LIST = realloc( LIST, sizeof( SIZE ) * ( LEN + 1 ) )) == NULL ) { \
		fprintf (stderr, "Could not reallocate new rendering struct...\n" ); \
		return NULL; \
	} \
	LIST[ LEN ] = TPTR; \
	LEN++;

#ifdef DEBUG
 #define FPRINTF(...) \
	fprintf( stderr, __VA_ARGS__ );

 #define DUMPACTION( NUM ) \
	( NUM == LOOP_START ) ? "LOOP_START" : \
	( NUM == LOOP_END ) ? "LOOP_END" : \
	( NUM == COMPLEX_EXTRACT ) ? "COMPLEX_EXTRACT" : \
	( NUM == SIMPLE_EXTRACT ) ? "SIMPLE_EXTRACT" : \
	( NUM == EACH_KEY ) ? "EACH_KEY" : \
	( NUM == EXECUTE ) ? "EXECUTE" : \
	( NUM == BOOLEAN ) ? "BOOLEAN" : \
	( NUM == RAW ) ? "RAW" : "UNKNOWN" 
#else
 #define FPRINTF(...)
 #define DUMPACTION( NUM )
#endif


const char *files[] = {
	"multi", 
#if 0
	"castigan", 

	"african", 
	"roche", 
	"tyrian"
#endif
};

//{{ # xxx }} - LOOP START
//{{ / xxx }} - LOOP END 
//{{ x     }} - SIMPLE EXTRACT
//{{ .     }} - COMPLEX EXTRACT
//{{ $     }} - EACH KEY OR VALUE IN A TABLE 
//{{ `xxx` }} - EXECUTE 
//{{ !xxx  }} - BOOLEAN? 
//{{ xxx ? y : z }} - TERNARY
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
const int maps[] = {
	//['#'] = SIMPLE_EXTRACT,
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

struct rb { 
	int action, **hashList; 
	int len; 
	void *ptr; 
};


//Purely for debugging, see what came out
void print_render_table() {
#if 0
	//DEBUG: Show me what's on the list...
	for ( int i=0; i<rrlen; i++ ) {
		struct rb *item = rr[ i ];

		//Dump the unchanging elements out...
		fprintf( stderr, "[%3d] => action: %-16s", i, DUMPACTION( item->action ) );

		if ( item->action == RAW || item->action == EXECUTE ) {
			fprintf( stderr, " len: %3d, ", item->len ); 
			ENCLOSE( item->ptr, 0, item->len );
		}
		else {
			fprintf( stderr, " len: %3d, list: %p => ", item->len, item->hashList );
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
#endif
}


//Bitmasking will tell me a lot...
#if 0
uint8_t *table_to_map ( Table *t, const uint8_t *src, int srclen, int *newlen ) {
	uint8_t *dest = NULL;
	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
	int SKIP = 0;
	int INSIDE = 0;
	struct rb **rr = NULL ; 
	struct parent **pp = NULL;
	int rrlen = 0;
	int pplen = 0;
	Mem r;
	memset( &r, 0, sizeof( Mem ) );

	//Allocate a new block to copy everything to
	if (( dest = malloc( 8 ) ) == NULL ) {
		return NULL;
	}

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
				uint8_t *p = trim( (uint8_t *)&src[r.pos], " \t", r.size, &nlen );
				struct rb *rp = malloc( sizeof( struct rb ) );
				if ( !rp ) {
					//Free and destroy things
					return NULL;
				}

				//Extract the first character
				if ( !maps[ *p ] ) {
					rp->action = SIMPLE_EXTRACT; 
					int hash = -1;
					if ( (hash = lt_get_long_i(t, p, nlen) ) > -1 ) {
						int *h = malloc( sizeof(int) );
						memcpy( h, &hash, sizeof( int ) );
						ADDITEM( h, int *, hashList, hashListLen ); 
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
								ADDITEM( h, int *, hashList, hashListLen ); 
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
								ADDITEM( h, int *, hashList, hashListLen ); 
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
							ADDITEM( np, struct parent, pp, pplen );
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
									ADDITEM( h, int *, hashList, hashListLen ); 
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
				FPRINTF( "\n@END: Adding new row to template set.  rrlen: %d, pplen: %d.  Got ", rrlen, pplen );
				ENCLOSE( rp->ptr, 0, rp->len );
				if ( !hashListLen )
					rp->hashList = NULL;
				else {
					rp->hashList = hashList; 
					ADDITEM( NULL, int *, rp->hashList, hashListLen );
				}
				
				ADDITEM( rp, struct rb, rr, rrlen );
			}
		}
		else {
			FPRINTF( "@RAW BLOCK COPY" ); 
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
				//struct rb rbb = { 0 };
				struct rb *rp = malloc( sizeof( struct rb ) );
				if ( !rp ) {
					//Teardown and destroy
					return NULL;
				}
				fprintf( stderr, "DO RAW COPY OF: " );
				write( 2, &src[ r.pos ], r.size );
				write( 2, "\n", 1 );
		
				//Set defaults
				rp->len = r.size;	
				//rp->hash = -2;
				rp->action = RAW;
				rp->hashList = NULL;
				rp->ptr = (uint8_t *)&src[ r.pos ];	
				
				//Save a new record
				ADDITEM(rp, struct rb, rr, rrlen);
			}	
		}
	}
	return NULL;
}


uint8_t *map_to_uint8t ( Table *t, const uint8_t *src, int srclen, int *newlen ) {
	//Start the writes, by using the structure as is
	uint8_t *block = NULL;
	int blockLen = 0;
	struct dep { int index, current, childCount; } depths[100] = { 0, 0, 0 };
	struct dep *d = depths;

	fprintf( stderr, "RENDER\n======\n" );
	for ( int i = 0; i < rrlen; i++ ) {
		struct rb *item = rr[ i ];
		if ( item->action == RAW || item->action == EXECUTE ) {
		#ifdef DEBUG
			fprintf( stderr, "%-20s", "RAW" );
			fprintf( stderr, "len: %3d, ", item->len ); 
			ENCLOSE( item->ptr, 0, item->len );
		#endif
			blockLen += item->len;
			if ( (block = realloc( block, blockLen )) == NULL ) {
				//NOTE: Teardown properly or you will cry...
				return NULL;
			}
			//If I do a memset, where at?
			memcpy( &block[ blockLen - item->len ], item->ptr, item->len ); 
		}
		else {
			if ( item->action == LOOP_START ) {
			#ifdef DEBUG
				fprintf( stderr, "%-20s", "LOOP_START" );
				fprintf( stderr, "len: %3d ", item->len );
			#endif
				d++;
				d->index = i;
				d->current = 0;
				d->childCount = item->len;
			} 
			else if ( item->action == LOOP_END ) {
				d->current++;
			#ifdef DEBUG
				fprintf( stderr, "%-20s", "LOOP_END" );
				fprintf( stderr, "%d =? %d", d->current, d->childCount );
			#endif
				if ( d->current == d->childCount ) {
					d--;
				}
				else {
					i = d->index;
				}
			}
			else if ( item->action == COMPLEX_EXTRACT ) {
			#ifdef DEBUG
				fprintf( stderr, "%-20s", "COMPLEX_EXTRACT " );
			#endif
				if ( item->hashList ) {
				#ifdef DEBUG
					//If there is a pointer, it does not move until I get through all three
					fprintf( stderr, "%d", **item->hashList );
				#endif
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
	struct rb **map = NULL;
	uint8_t *block = NULL;
	int blocklen = 0;

	//Check tables first
	//if ( !t ???? )
	
	//Convert T to a map
	if ( !table_to_map( ... ) ) {

	}

	//Block can be managed from here now

	//Do the map	
	if ( !map_to_uint8t( ... ) ) {

	}

	return block; 
}
#endif


uint8_t *table_to_uint8t( Table *t, const uint8_t *src, int srclen, int *newlen ) {
	//Constants for now, b/c I forgot how to properly bitmask
	uint8_t *dest = NULL;
	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
	int SKIP = 0;
	int INSIDE = 0;
	struct rb **rr = NULL ; 
	struct parent **pp = NULL;
	int rrlen = 0;
	int pplen = 0;
	Mem r;
	memset( &r, 0, sizeof( Mem ) );

	//Allocate a new block to copy everything to
	if (( dest = malloc( 8 ) ) == NULL ) {
		return NULL;
	}

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
				uint8_t *p = trim( (uint8_t *)&src[r.pos], " \t", r.size, &nlen );
				struct rb *rp = malloc( sizeof( struct rb ) );
				if ( !rp ) {
					//Free and destroy things
					return NULL;
				}

				//Extract the first character
				if ( !maps[ *p ] ) {
					rp->action = SIMPLE_EXTRACT; 
					int hash = -1;
					if ( (hash = lt_get_long_i(t, p, nlen) ) > -1 ) {
						int *h = malloc( sizeof(int) );
						memcpy( h, &hash, sizeof( int ) );
						ADDITEM( h, int *, hashList, hashListLen ); 
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
								ADDITEM( h, int *, hashList, hashListLen ); 
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
								ADDITEM( h, int *, hashList, hashListLen ); 
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
							ADDITEM( np, struct parent, pp, pplen );
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
									ADDITEM( h, int *, hashList, hashListLen ); 
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
				FPRINTF( "\n@END: Adding new row to template set.  rrlen: %d, pplen: %d.  Got ", rrlen, pplen );
				ENCLOSE( rp->ptr, 0, rp->len );
				if ( !hashListLen )
					rp->hashList = NULL;
				else {
					rp->hashList = hashList; 
					ADDITEM( NULL, int *, rp->hashList, hashListLen );
				}
				
				ADDITEM( rp, struct rb, rr, rrlen );
			}
		}
		else {
			FPRINTF( "@RAW BLOCK COPY" ); 
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
				//struct rb rbb = { 0 };
				struct rb *rp = malloc( sizeof( struct rb ) );
				if ( !rp ) {
					//Teardown and destroy
					return NULL;
				}
				fprintf( stderr, "DO RAW COPY OF: " );
				write( 2, &src[ r.pos ], r.size );
				write( 2, "\n", 1 );
		
				//Set defaults
				rp->len = r.size;	
				//rp->hash = -2;
				rp->action = RAW;
				rp->hashList = NULL;
				rp->ptr = (uint8_t *)&src[ r.pos ];	
				
				//Save a new record
				ADDITEM(rp, struct rb, rr, rrlen);
			}	
		}
	}


	//Start the writes, by using the structure as is
	uint8_t *block = NULL;
	int blockLen = 0;
	struct dep { int index, current, childCount; } depths[100] = { 0, 0, 0 };
	struct dep *d = depths;

	fprintf( stderr, "RENDER\n======\n" );
	for ( int i = 0; i < rrlen; i++ ) {
		struct rb *item = rr[ i ];
		if ( item->action == RAW || item->action == EXECUTE ) {
		#ifdef DEBUG
			fprintf( stderr, "%-20s", "RAW" );
			fprintf( stderr, "len: %3d, ", item->len ); 
			ENCLOSE( item->ptr, 0, item->len );
		#endif
			blockLen += item->len;
			if ( (block = realloc( block, blockLen )) == NULL ) {
				//NOTE: Teardown properly or you will cry...
				return NULL;
			}
			//If I do a memset, where at?
			memcpy( &block[ blockLen - item->len ], item->ptr, item->len ); 
		}
		else {
			if ( item->action == LOOP_START ) {
			#ifdef DEBUG
				fprintf( stderr, "%-20s", "LOOP_START" );
				fprintf( stderr, "len: %3d ", item->len );
			#endif
				d++;
				d->index = i;
				d->current = 0;
				d->childCount = item->len;
			} 
			else if ( item->action == LOOP_END ) {
				d->current++;
			#ifdef DEBUG
				fprintf( stderr, "%-20s", "LOOP_END" );
				fprintf( stderr, "%d =? %d", d->current, d->childCount );
			#endif
				if ( d->current == d->childCount ) {
					d--;
				}
				else {
					i = d->index;
				}
			}
			else if ( item->action == COMPLEX_EXTRACT ) {
			#ifdef DEBUG
				fprintf( stderr, "%-20s", "COMPLEX_EXTRACT " );
			#endif
				if ( item->hashList ) {
				#ifdef DEBUG
					//If there is a pointer, it does not move until I get through all three
					fprintf( stderr, "%d", **item->hashList );
				#endif
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


int main (int argc, char *argv[]) {
	lua_State *L = luaL_newstate();
	char err[ 2048 ] = { 0 };

	//A good test would be to modify this to where either files*[] can be run or a command line specified file.
	for ( int i=0; i < sizeof(files)/sizeof(char *); i++ ) {
		//Filename
		Render R;
		Table *t = NULL; 
		int br = 0;
		int fd = 0;
		char ren[ 10000 ] = { 0 };
		char *m = strcmbd( "/", "tests/render-data", files[i], files[i], "lua" );
		char *v = strcmbd( "/", "tests/render-data", files[i], files[i], "tpl" );
		m[ strlen( m ) - 4 ] = '.';
		v[ strlen( v ) - 4 ] = '.';

		//Choose a file to load
		char fileerr[2048] = {0};
		char *f = m;
		int lerr;
		fprintf( stderr, "Attempting to load model file: %s\n", f );
		if (( lerr = luaL_loadfile( L, f )) != LUA_OK ) { 
			int errlen = 0;
			if ( lerr == LUA_ERRSYNTAX )
				errlen = snprintf( fileerr, sizeof(fileerr), "Syntax error at file: %s", f );
			else if ( lerr == LUA_ERRMEM )
				errlen = snprintf( fileerr, sizeof(fileerr), "Memory allocation error at file: %s", f );
			else if ( lerr == LUA_ERRGCMM )
				errlen = snprintf( fileerr, sizeof(fileerr), "GC meta-method error at file: %s", f );
			else if ( lerr == LUA_ERRFILE ) {
				errlen = snprintf( fileerr, sizeof(fileerr), "File access error at: %s", f );
			}
			
			fprintf(stderr, "LUA LOAD ERROR: %s, %s", fileerr, (char *)lua_tostring( L, -1 ) );	
			lua_pop( L, lua_gettop( L ) );
			exit( 1 );
			break;
		}

		//Then execute
		fprintf( stderr, "Attempting to execute file: %s\n", f );
		if (( lerr = lua_pcall( L, 0, LUA_MULTRET, 0 ) ) != LUA_OK ) {
			if ( lerr == LUA_ERRRUN ) 
				snprintf( fileerr, sizeof(fileerr), "Runtime error at: %s", f );
			else if ( lerr == LUA_ERRMEM ) 
				snprintf( fileerr, sizeof(fileerr), "Memory allocation error at file: %s", f );
			else if ( lerr == LUA_ERRERR ) 
				snprintf( fileerr, sizeof(fileerr), "Error while running message handler: %s", f );
			else if ( lerr == LUA_ERRGCMM ) {
				snprintf( fileerr, sizeof(fileerr), "Error while runnig __gc metamethod at: %s", f );
			}

			fprintf(stderr, "LUA EXEC ERROR: %s, %s", fileerr, (char *)lua_tostring( L, -1 ) );	
			lua_pop( L, lua_gettop( L ) );
			exit( 1 );
			break;
		}

		//Dump the stack
		lua_stackdump( L );

		//Allocate a new "Table" structure...
		if ( !(t = malloc(sizeof(Table))) || !lt_init( t, NULL, 1024 )) {
			fprintf( stderr, "MALLOC ERROR FOR TABLE: %s\n", strerror( errno ) );
			exit( 1 );
		}

		//Convert Lua to Table
		if ( !lua_to_table( L, 1, t ) ) {
			fprintf( stderr, "%s\n", err );
			//goto cleanit;
		}

		//Show the table after conversion from Lua
		if ( 1 ) {
			lt_dump( t );
		}

	#if 1
		//Check for and load whatever file
		int fstat, bytesRead, fileSize;
		uint8_t *buf = NULL;
		struct stat sb;
		memset( &sb, 0, sizeof( struct stat ) );

		//Check for the file 
		if ( (fstat = stat( v, &sb )) == -1 ) {
			fprintf( stderr, "FILE STAT ERROR: %s\n", strerror( errno ) );
			exit( 1 );
		}

		//Check for the file 
		if ( (fd = open( v, O_RDONLY )) == -1 ) {
			fprintf( stderr, "FILE OPEN ERROR: %s\n", strerror( errno ) );
			exit( 1 );
		}

		//Allocate a buffer
		fileSize = sb.st_size + 1;
		if ( !(buf = malloc( fileSize )) || !memset(buf, 0, fileSize)) {
			fprintf( stderr, "COULD NOT OPEN VIEW FILE: %s\n", strerror( errno ) );
			exit( 1 );
		}

		//Read the entire file into memory, b/c we'll probably have space 
		if ( (bytesRead = read( fd, buf, sb.st_size )) == -1 ) {
			fprintf( stderr, "COULD NOT READ ALL OF VIEW FILE: %s\n", strerror( errno ) );
			exit( 1 );
		}

		//Dump the file just cuz...
		if ( 1 ) {
			write( 2, buf, sb.st_size );
		}

		//Finding the marks is good if there is enough memory to do it
		int renderLen = 0;
		uint8_t *rendered = table_to_uint8t( t, buf, sb.st_size , &renderLen );
		fprintf( stderr, "%p\n", rendered );
		if ( rendered ) {
			write( 2, rendered, renderLen );
		}

	#else	
		//Prepare the rendering engine
		fprintf( stderr, "Rendering against view file %s\n", v );
		if ( !render_init( &R, &t ) )
			{ fprintf( stderr, "render_init failed...\n"); goto cleanit ; }

		//Load up the file for the rendering engine
		fd = open( v, O_RDONLY );
		if (( br = read( fd, ren, sizeof( ren ) - 1 )) == -1 )
			{ fprintf( stderr, "loading file '%s' failed...\n", v); goto cleanit ; }

		//Dump the block
		if ( 1 )
			write( 2, ren, br );
		
		//"Score" the block to render
		if ( !render_map( &R, (uint8_t *)ren, br ) )
			{ fprintf( stderr, "render mapping failed...\n"); goto cleanit ; }

		//Start rendering
		if ( !render_render( &R ) )
			{ fprintf( stderr, "render_init failed...\n"); goto cleanit ; }

		//Show the results
		if ( 1 )
			write( 2, bf_data( render_rendered( &R ) ), bf_written( render_rendered( &R )) );

		//Clean up
		render_free( &R );
		lt_free( &t );
	#endif
		//Free things
	#if 0
cleanit:
		if ( fd ) { 
			close(fd); 
			fd = 0; 
		}
		lua_settop( L, 0 );
		free( m );
		free( v );
	#endif
	}
	return 0;
}
