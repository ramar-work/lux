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
#define CREATEITEM(TPTR,SPTR,HASH,LEN) \
	struct rb *TPTR = malloc( sizeof( struct rb ) ); \
	memset( TPTR, 0, sizeof( struct rb ) ); \
	TPTR->len = LEN;	\
	TPTR->ptr = SPTR; \
	TPTR->hash = HASH; \
	TPTR->rbptr = NULL;
#define ADDITEM(TPTR,LIST,LEN) \
	if (( LIST = realloc( LIST, sizeof( struct rb ) * LEN )) == NULL ) { \
		fprintf (stderr, "Could not reallocate new rendering struct...\n" ); \
		return NULL; \
	} \
	LIST[ LEN - 1 ] = TPTR; \
	LEN++;

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
	struct rb { int len; int hash; uint8_t *ptr; int *rbptr; int childCount; char *parent; } **rr = NULL;
	int rrlen = 1;

	//Put all of this parent crap into a structure of it's own that is ridden up and down
	char PARENT[ 2048 ];
	int PARENTLEN = 0;
	memset( PARENT, 0, sizeof( PARENT ) );
	struct parent { int childCount; char *parent; int parentLen; int hash; };

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
	if (( dest = malloc( 16 ) ) == NULL ) {
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
				#if 0
				rbb.ptr = NULL;
				rbb.rbptr = NULL;
				rbb.hash = 0;
				rbb.len = 0;
				#endif

				//Extract the first character
				fprintf( stderr, "%c maps to => %d\n", *p, maps[ *p ] ); 
				if ( !maps[ *p ] ) {
					//Find the hash of whatever this is...
					rbb.ptr = p;
					rbb.len = nlen;
					rbb.hash = lt_get_long_i(t, p, nlen);
					rbb.rbptr = NULL;
				}
				else {
					int action = maps[ *p ];
					int alen = 0;
					//Advance and reset p b/c we need just the text...
					p = trim( p, ". #/$`!\t", nlen, &alen );
					
					//Set defaults for the new object 
					rbb.ptr = p;
					rbb.len = alen;

					//Figure some things out...
					if ( action == LOOP_START ) {
						//If this hash is not here, then just skip things.
						if (( rbb.hash = lt_get_long_i( t, p, alen )) == -1 ) {
							//I don't know what to do here...
							//This should just work...
							continue; 
						}	

						//Should I get the number of children?
						INSIDE++;
						memcpy( &PARENT[ PARENTLEN ], p, alen );
						PARENTLEN += alen;
						int childCount = lt_counti( t, rbb.hash ); 
						rbb.childCount = childCount;
						//fprintf( stderr, "PARENT LEN IS: %d, CHILD COUNT IS: %d, PARENT IS CURRENTLY: '%s'\n", alen, childCount, PARENT );
getchar();
						//PARENT = p;
						//Parent can be a raw string buffer that gets deallocated later.
					}
					else if ( action == LOOP_END ) {
						//If inside is > 1, check for a period, strip it backwards...
						INSIDE--;
					}
					else if ( action == COMPLEX_EXTRACT ) {
						char CHILD[ 2048 ];
						int CHILDLEN = 0;
						memset( CHILD, 0, sizeof( CHILD ) );
						memcpy( CHILD, PARENT, PARENTLEN );
						CHILDLEN += PARENTLEN;
						memcpy( &CHILD[ PARENTLEN ], ".", 1 );
						CHILDLEN += 1;
						memcpy( &CHILD[ PARENTLEN + 1 ], p, alen );
						CHILDLEN += alen;
fprintf( stderr, "COMPLEX_EXTRACT CHECKING FOR: %s", CHILD );
fprintf( stderr, "GOT HASH: %s", CHILD );
//This is a good time to check all the children
getchar();
					}
					else if ( action == EACH_KEY ) {
					}
					else if ( action == EXECUTE ) {
					}
					else if ( action == BOOLEAN ) {
					}
				}

				struct rb *rp = malloc( sizeof( struct rb ) );
				memcpy( rp, &rbb, sizeof( struct rb ) ); 
				ADDITEM(rp, rr, rrlen);

#if 0
			#if 1
				//Add a row
				//CREATEITEM(rb, p, 0, nlen );
				ADDITEM(rb, rr, rrlen);
			#else	
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
			#endif
#endif
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

#if 0
		//Start doing things...
		if ( BLOCK == BLOCK_END ) {
			//Trim the string in between...
			write( 2, &src[ r.pos ], r.size );
			if ( 0 )
				;
			else if ( ACTION == LOOP_START ) {
			}
			else if ( ACTION == LOOP_END ) {
			}
			else if ( ACTION == LOOP_START ) {
			}
			else if ( ACTION == LOOP_START ) {
			}
			else if ( ACTION == LOOP_START ) {
			}
			else if ( ACTION == LOOP_START ) {
			}
		}
#endif
	}

	//CREATEITEM(rb, p, 0, r.size );
	//ADDITEM(rb, rr, rrlen);
#if 1
	for ( int i=0; i<rrlen; i++ ) {
		struct rb *item = rr[ i ];
		if ( item->ptr ) {
			fprintf( stderr, "[%d] => { len: %d, hash: %d, ", i, item->len, item->hash );
			fprintf( stderr, "ptr: '" );	
			write( 2, item->ptr, item->len );
			fprintf( stderr, "', rbptr: %p }", item->rbptr );	
			write( 2, "\n", 1 );
		}
	}
#endif

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
			goto cleanit;
		}

		//Show the table after conversion from Lua
		if ( 1 ) {
			lt_dump( t );
		}

	#if 1
		//Check for and load whatever file
		int fstat, bytesRead;
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
		if ( !(buf = malloc(sb.st_size)) || !memset(buf, 0, sb.st_size)) {
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
			write( 2, buf, bytesRead );
		}

		//Finding the marks is good if there is enough memory to do it
		int renderLen = 0;
		uint8_t *rendered = rw( t, buf, bytesRead, &renderLen );
		write( 2, rendered, renderLen );

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
cleanit:
		if ( fd ) { 
			close(fd); 
			fd = 0; 
		}
		lua_settop( L, 0 );
		free( m );
		free( v );
	}
	return 0;
}
