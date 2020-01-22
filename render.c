/* ---------------------------------------------------
testrender.c 

Test out rendering...

TODO / TASKS
------------
- Get proper renders to work...
- Make it work with nested anything

 * --------------------------------------------------- */
#include "../vendor/single.h"
#include "../bridge.h"
#define SQROOGE_H

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

#define DPRINTF(...) \
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

#define DUMPTMPROW( row ) \
	fprintf( stderr, "[%d] => len: %3d, hash: --, action: %-16s, rbptr: %p, ptr: ", i, \
		item->len, DUMPACTION( item->action ), item->hashList ); \
	ENCLOSE( row->ptr, 0, row->len );

#define DUMPPTRS( row )



#if 0
	write( 2, item->ptr, item->len ); \
	fprintf( stderr, "', rbptr: %p }", item->rbptr );	\
	write( 2, "\n", 1 );
#endif


//TODO: Embed the tests here.
#if 0
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



//Should I still rely on function pointers?
//No, let's do something different...

//{{ # xxx }} - LOOP START
//{{ / xxx }} - LOOP END 
//{{ x     }} - SIMPLE EXTRACT
//{{ .     }} - COMPLEX EXTRACT
//{{ $     }} - EACH KEY OR VALUE IN A TABLE 
//{{ `xxx` }} - EXECUTE 
//{{ !xxx  }} - BOOLEAN? 
//{{ xxx ? y : z }} - TERNARY

//Bitmasking will tell me a lot...
uint8_t *rw ( Table *t, const uint8_t *src, int srclen, int * newlen ) {
	//Constants for now, b/c I forgot how to properly bitmask
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
	uint8_t *dest = NULL;
	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
	int SKIP = 0;
	int INSIDE = 0;
	Mem r;
	memset( &r, 0, sizeof( Mem ) );

	//Hmm... this structure could use a little help
	//1) A union is probably best considering memory and type-safety.
	//2) void pointers work, but they are very ugly and error prone
	//3) A specific member for each type could work too

	//Hmm. len is not super necessary, only because the hash can be used to pull things
	//so, we're looking at:
	//{ int hash, action;  uint8_t *text;  int **hashlist; }
	//hash is the source hash if it's a simple replace, or the parent if hashlist is used
	//action is what to do, this cna just be an integer constant
	//text is original text if its there, a replacement if it's just one
	//hashlist is a list of hashes when we reach bigger tables
	//
	//NOTE:
	//this could probably be a two column table if I really think about it...
	//{ int **hashlist; uint8_t *block; }
	//if hashlist is null, assume that block is raw content
	//if hashlist is not null, pull each hash as you go through

	//NOTE:
	//Yet another way to do it, is to make the data structure much bigger.
	//Then you don't have to move backwards in a list of pointers...
	//If this is really a two-column list, then it won't take much space... 
//#define RBDEF
	struct rb { int action, **hashList; int len; uint8_t *ptr; } **rr = NULL ; 
	struct parent { struct parent *parent; uint8_t *text; int len, childCount; } **pp = NULL;
	int rrlen = 0;
	int pplen = 0;

	//TODO: Maps really should come from outside of the function.
	//char *mapchars = "#/.$`!";
	//struct map { int action; char a; } **maps = NULL;
	int maps[] = {
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
				struct rb rbb = { 0 };

				//Extract the first character
				DPRINTF( "%c maps to => %d\n", *p, maps[ *p ] ); 
				if ( !maps[ *p ] ) {
					rbb.action = SIMPLE_EXTRACT; 
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
					rbb.action = maps[ *p ];
					p = trim( p, ". #/$`!\t", nlen, &alen );
					DPRINTF("GOT ACTION %s, and TEXT = ", DUMPACTION(rbb.action));
					ENCLOSE( p, 0, alen );

					//Figure some things out...
					if ( rbb.action == LOOP_START ) {

						//We could be just about anywhere, so anticipate that the parent could be here
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
						
							DPRINTF( "@LOOP_START :: Checking for level[0] table " );	
							ENCLOSE( bbuf, 0, blen );
							//This is the only thing, get the hash and end it
							if ( (hash = lt_get_long_i(t, bbuf, blen ) ) > -1 ) {
								int *h = malloc( sizeof(int) );
								memcpy( h, &hash, sizeof( int ) );
								ADDITEM( h, int *, hashList, hashListLen ); 
								eCount = lt_counti( t, hash );
							}
						}
						else {
							//If there are 3 parents, you need to start at the beginning and come out
							//Find the MAX count of all the rows that are there... This way you'll have the right count everytime...
							DPRINTF( "@LOOP_START :: Checking for level[n+1] table " );	

							//Get a count of the number of elements in the parent.
							int maxCount = 0;
							cp = pp[ pplen - 1 ];
							DPRINTF("@LOOP_START :: Checking level[n+1] table w/ %d members\n", cp->childCount);

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
							
								//Check for the hash
								//This check is not always going to fly...
								hash = lt_get_long_i(t, bbuf, blen ); 
								int *h = malloc( sizeof(int) );
								memcpy( h, &hash, sizeof( int ) );
								ADDITEM( h, int *, hashList, hashListLen ); 
								DPRINTF( "@LOOP_START :: Checking for children, hash is: %3d, ", hash );	
							
								if ( hash > -1 && (cCount = lt_counti( t, hash )) > maxCount ) {
									maxCount = cCount;	
									DPRINTF( "@LOOP_START :: COUNT OF ELEMENTS ARE: %3d\n", maxCount );	
								}

								ENCLOSE( bbuf, 0, blen );
							}
							eCount = maxCount;
						}

						//Find the hash
						if ( hashListLen ) {
							struct parent *np = NULL; 
							//Set up the parent structure
							if (( np = malloc( sizeof(struct parent) )) == NULL ) {
								//TODO: Cut out and free things
							}

							//NOTE: len will contain the number of elements to loop
							np->childCount = eCount;
							np->len = alen;
							np->text = p; 
							np->parent = cp;
							ADDITEM( np, struct parent, pp, pplen );
							INSIDE++;
						}
					}
					else if ( rbb.action == LOOP_END ) {
						//If inside is > 1, check for a period, strip it backwards...
						DPRINTF( "@LOOP_END :: Should these match?\n" );
						//TODO: Check that the hashes match instead of just pplen
						//rbb.hash = lt_get_long_i( t, p, alen );
						if ( !INSIDE )
							;
						else if ( pplen == INSIDE ) {
							free( pp[ pplen - 1 ] );
							pplen--;
							INSIDE--;
						}
					}
					else if ( rbb.action == COMPLEX_EXTRACT ) {
						DPRINTF( "@COMPLEX_EXTRACT :: Check for children... " );

						//I have a parent structure, which I need to loop through to get stuff
						//I have a set of hashes which will tell me how long I need to loop for

						//Build a string out of the parent structure for each hash
						if ( pplen ) {

							//I feel like you need to make an array out of all the childCounts
		
							//Loop through them, generating a bigger string, each new invocation
							//needs to start with the same thing

							//We need to loop through each parent
							for ( int pi=0; pi < pplen; pi++ ) {
								struct parent *yp = pp[ pplen - 1 ];
								int yplen = 0;
								uint8_t ypstr[2048] = {0};
								DPRINTF( "@parent=%p, childCount=%d, ptext=", yp, yp->childCount );

								//We build something for each of these...
								for ( int ci=0; ci < yp->childCount; ci++ ) {
									//Build the root strings here (can't remember why)
									uint8_t ypnum[64] = {0};
									yplen = 0;
									memcpy( &ypstr[ yplen ], yp->text, yp->len);
									yplen += yp->len;	
									yplen += sprintf( (char *)&ypstr[ yplen ], ".%d.", ci );

									//Enclose
									ENCLOSE( ypstr, 0, yplen ); //ccp->text, 0, ccp->len );
								}

							}
						}
						fprintf(stderr,"Getchar()\n" );
						getchar();

#if 0
						char CHILD[ 2048 ] = { 0 };
						int CHILDLEN = 0;

						if ( !INSIDE ) {
							//INSIDE should equal parent level (so pplen + 1 )
							//TODO: Should we still not mark the points?
							continue;
						}
						else {
							//We'll have to work backwards to create the right thing...
							struct parent *cp = pp[ pplen - 1 ];
							memcpy( CHILD, cp->text, cp->len );
							CHILDLEN += cp->len;
							DPRINTF( "COMPLEX_EXTRACT CHECKING FOR PARENT: '%s'\n", CHILD );

							//Find all the children
							if ( cp->childCount ) {
								int rblen = 0;
								for ( int i=0; i < ( cp->childCount ); i++ ) {
									int *hash, br, cl = CHILDLEN; 
									br = sprintf( &CHILD[ cl ], ".%d.", i );
									cl += br;
									memcpy( &CHILD[ cl ], p, alen );
									cl += alen;
									if (( hash = malloc( sizeof(int) ) ) == NULL ) {
										//TODO: Cut out and die...
									}
									*hash = lt_get_long_i( t, (uint8_t *)CHILD, cl );
									DPRINTF( "COMPLEX_EXTRACT CHECKING FOR: " ); 
									DPRINTF( "%d '%s', GOT: %d\n", br, CHILD, *hash );
									ADDITEM( hash, int *, rbb.hashList, rblen ); 
								}
								//This could work for you...
								//rbb.len = rblen;
							}
						}
#endif
					}
					else if ( rbb.action == EACH_KEY ) {
						DPRINTF( "@EACH_KEY :: Nothing yet...\n" );
					}
					else if ( rbb.action == EXECUTE ) {
						DPRINTF( "@EXECUTE :: Nothing yet...\n" );
					}
					else if ( rbb.action == BOOLEAN ) {
						DPRINTF( "@BOOLEAN :: Nothing yet...\n" );
					}
				}

				//Create a new row with what we found.
				DPRINTF( "@END: Adding new row to template set.  rrlen: %d, pplen: %d.  Got ", rrlen, pplen );
				ENCLOSE( rbb.ptr, 0, rbb.len );

				struct rb *rp = malloc( sizeof( struct rb ) );
				memcpy( rp, &rbb, sizeof( struct rb ) ); 
				if ( !hashListLen )
					rp->hashList = NULL;
				else {
					rp->hashList = hashList; 
					ADDITEM( NULL, int *, rp->hashList, hashListLen );
				}
				
				ADDITEM(rp, struct rb, rr, rrlen);
			}
		}
#if 1
		else {
			DPRINTF( "@RAW BLOCK COPY" ); 
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
				struct rb rbb = { 0 };
				fprintf( stderr, "DO RAW COPY OF: " );
				write( 2, &src[ r.pos ], r.size );
				write( 2, "\n", 1 );
		
				//Set defaults
				rbb.len = r.size;	
				//rbb.hash = -2;
				rbb.action = RAW;
				rbb.hashList = NULL;
				rbb.ptr = (uint8_t *)&src[ r.pos ];	
				
				//Save a new record
				struct rb *rp = malloc( sizeof( struct rb ) );
				memcpy( rp, &rbb, sizeof( struct rb ) ); 
				ADDITEM(rp, struct rb, rr, rrlen);
			}	
		}
#endif
	}

#if 1
	//Loop through all of items
	for ( int i=0; i<rrlen; i++ ) {
		struct rb *item = rr[ i ];

		//Dump the unchanging elements out...
		fprintf( stderr, "[%d] => action: %-16s", i, DUMPACTION( item->action ) );

		if ( item->action == RAW || item->action == EXECUTE ) {
			fprintf( stderr, " len: %3d, ", item->len ); 
			ENCLOSE( item->ptr, 0, item->len );
		}
		else {
			fprintf( stderr, " list: %p => ", item->hashList );
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

	//The final step is to assemble everything...
	return NULL;
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
		uint8_t *rendered = rw( t, buf, sb.st_size , &renderLen );
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
