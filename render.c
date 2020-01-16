/* ---------------------------------------------------
testrender.c 

Test out rendering...
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

#define DUMPTMPROW( row ) \
	fprintf( stderr, "[%d] => len: %d, hash: %d, rbptr: %p, ptr: ", i, \
		item->len, item->hash, item->rbptr ); \
	ENCLOSE( row->ptr, 0, row->len );

#if 0
	write( 2, item->ptr, item->len ); \
	fprintf( stderr, "', rbptr: %p }", item->rbptr );	\
	write( 2, "\n", 1 );
#endif

const char *files[] = {
	"castigan", 
#if 0
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
	const int BLOCK_START = 0;
	const int BLOCK_END = 0;
	uint8_t *dest = NULL;
	int destlen = 0;
	int ACTION = 0;
	int BLOCK = 0;
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
#define RBDEF
#ifdef RBDEF
	struct rb { int action, len, **hashList; uint8_t *ptr; } **rr = NULL ; 
#else
	struct rb { int len; int hash; int action; uint8_t *ptr; int **rbptr; } **rr = NULL ; 
#endif
	struct parent { uint8_t *parent; uint8_t *text; int len; int plen; int childCount; } **pp = NULL;
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
		[256] = 0
	};

	//Allocate a new block to copy everything to
	if (( dest = malloc( 8 ) ) == NULL ) {
		return NULL;
	}

	//Allocating a list of characters to elements is easiest.
	while ( memwalk( &r, (uint8_t *)src, (uint8_t *)"{}", srclen, 2 ) ) {
	#if 0
		//See what was received.
		fprintf( stderr, "RENDER CALL GOT: %c, BLOCK SIZE: %d ", r.chr, r.size );
		write( 2, &src[ r.pos ], 5 );	
		write( 2, "\n", 1 );	
	#endif

		//More than likely, I'll always use a multi-byte delimiter
		if ( r.size == 0 ) { /*&& r.pos > 0 ) {*/
			//Check if there is a start or end block
			if ( r.chr == '{' ) {
				BLOCK = BLOCK_START;
			}
		}
		else if ( r.chr == '}' ) {
		#if 0		
			fprintf( stderr, "block end?: " );
			write( 2, "'", 1 );
			write( 2, &src[ r.pos + r.size + 1 ], 1 );
			write( 2, "', ", 3 );
		#endif
			if ( src[ r.pos + r.size + 1 ] == '}' ) {
				//Start extraction...
				BLOCK = BLOCK_END;
				int nlen = 0;	
				uint8_t *p = trim( (uint8_t *)&src[r.pos], " \t", r.size, &nlen );
				struct rb rbb = { 0 };

				//Extract the first character
				DPRINTF( "%c maps to => %d\n", *p, maps[ *p ] ); 
				if ( !maps[ *p ] ) {
					//Find the hash of whatever this is...
				#ifdef RBDEF
					rbb.ptr = p;
					rbb.len = nlen;
					rbb.hash = lt_get_long_i(t, p, nlen);
					rbb.rbptr = NULL;
				#else
					rbb.ptr = p;
					rbb.len = nlen;
					rbb.hash = lt_get_long_i(t, p, nlen);
					rbb.rbptr = NULL;
				#endif
				}
				else {
					int action = maps[ *p ];
					int alen = 0;
					char aa = *p;
					//Advance and reset p b/c we need just the text...
					p = trim( p, ". #/$`!\t", nlen, &alen );
					
					//Set defaults for the new object 
					rbb.ptr = p;
					rbb.len = alen;

DPRINTF( "GOT ACTION '%c' => %d, and TEXT = ", aa, action ); 
ENCLOSE( p, 0, alen );

					//Figure some things out...
					if ( action == LOOP_START ) {
						//If this hash is not here, skip this and each of the things in it...
						rbb.hash = lt_get_long_i( t, p, alen );
						DPRINTF( "@LOOP_START :: CHECK FOR HASH %d...", rbb.hash );

						if ( rbb.hash == -1 ) {
							//TODO: Things MAY need to be freed here...
							continue; 
						}

						//Set up the parent structure
						struct parent *par = NULL; 
						if (( par = malloc( sizeof(struct parent) )) == NULL ) {
							//TODO: Cut out and free things
						}

						par->len = alen;
						par->childCount = lt_counti( t, rbb.hash );
						par->text = p;

						if ( INSIDE > 1 ) {
							//Get the last parent and make that the thing
							struct parent *op = pp[ pplen ];
							par->parent = op->text;
							par->plen = op->len + alen;
						}
						ADDITEM( par, struct parent, pp, pplen );
						INSIDE++;
					}
					else if ( action == LOOP_END ) {
						//If inside is > 1, check for a period, strip it backwards...
						DPRINTF( "@LOOP_END :: Should these match?\n" );
						if ( !INSIDE ) 
							continue;
						else {
							//free( pp[ pplen - 1 ] );
							//( pplen > 1 ) ? pplen-- : 0;
							pplen--;
							INSIDE--;
						}
					}
					else if ( action == COMPLEX_EXTRACT ) {
						DPRINTF( "@COMPLEX_EXTRACT :: Check for children " );
						if ( !INSIDE ) //INSIDE should equal parent level (so pplen + 1 )
							continue;
						else { 
							char CHILD[ 2048 ];
							int CHILDLEN = 0;
DPRINTF( "Grabbing parent index at %d\n", pplen - 1 );
							struct parent *par = pp[ pplen - 1 ];

							//Append things to CHILD data
							memset( CHILD, 0, sizeof( CHILD ) );
							memcpy( CHILD, par->text, par->len );
							CHILDLEN += par->len;
							DPRINTF( "COMPLEX_EXTRACT CHECKING FOR PARENT: '%s'\n", CHILD );

							//Find all the children
							if ( par->childCount ) {
								int rblen = 0;
								for ( int i=0; i < ( par->childCount ); i++ ) {
									int *hash, br, cl = CHILDLEN; 
									br = sprintf( &CHILD[ cl ], ".%d.", i );
									cl += br;
									memcpy( &CHILD[ cl ], p, alen );
									cl += alen;
									hash = malloc( sizeof(int) );
									*hash = lt_get_long_i( t, (uint8_t *)CHILD, cl );
									DPRINTF( "COMPLEX_EXTRACT CHECKING FOR: " ); 
									DPRINTF( "%d '%s', GOT: %d\n", br, CHILD, *hash );
									//Add each int
									ADDITEM( hash, int *, rbb.rbptr, rblen ); 
								}
								#if 0
								//This is debug purposes as well...
								for ( int i=0; i<rblen - 1; i++ ) {
									fprintf( stderr, "\tEXTRACT HASHES GOT: %d\n", *rbb.rbptr[ i ] );
								}
								#endif
							}
						}
					}
					else if ( action == EACH_KEY ) {
						DPRINTF( "@EACH_KEY :: Nothing yet...\n" );
					}
					else if ( action == EXECUTE ) {
						DPRINTF( "@EXECUTE :: Nothing yet...\n" );
					}
					else if ( action == BOOLEAN ) {
						DPRINTF( "@BOOLEAN :: Nothing yet...\n" );
					}
				}

				//Create a new row with what we found.
				DPRINTF( "@END: Adding new row to template set.  rrlen: %d, pplen: %d.  Got ", rrlen, pplen );
				ENCLOSE( rbb.ptr, 0, rbb.len );
				struct rb *rp = malloc( sizeof( struct rb ) );
				memcpy( rp, &rbb, sizeof( struct rb ) ); 
				ADDITEM(rp, struct rb, rr, rrlen);
			}
		}
#if 0
		else {
			//We can simply copy if ACTION & BLOCK are 0 
			if ( !ACTION && !BLOCK ) {
				fprintf( stderr, "DO RAW COPY OF: " );
				write( 2, &src[ r.pos ], r.size );
				write( 2, "\n", 1 );

				struct rb *rb = malloc( sizeof( struct rb ) );
				memset( rb, 0, sizeof( struct rb ) );
				rb->len = r.size;	
				rb->ptr = (uint8_t *)&src[ r.pos ];	
				rb->hash = 0;
				rb->rbptr = NULL;
				if (( rr = realloc( rr, sizeof( struct rb ) * rrlen )) == NULL ) {
					fprintf (stderr, "Could not reallocate new rendering struct...\n" );
					return NULL;
				}
				rr[ rrlen - 1 ] = rb;
				rrlen++;
			}	
		}
#endif
	}

#if 1
	//Loop through all of items
	for ( int i=0; i<rrlen; i++ ) {
		struct rb *item = rr[ i ];
		DUMPTMPROW( item );
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
