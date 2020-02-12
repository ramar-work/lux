/*let's try this again.  It seems never to work like it should...*/
#include "../vendor/single.h"
#include <gnutls/gnutls.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "luabind.h"
#include "http.h"
#include "socket.h"
#include "util.h"
#include "ssl.h"


extern const char http_200_custom[];
extern const char http_200_fixed[];
extern const char http_400_custom[];
extern const char http_400_fixed[];
extern const char http_500_custom[];
extern const char http_500_fixed[];

#if 0
#define ADD_ELEMENT( ptr, ptrListSize, eSize, element ) \
	if ( ptr ) \
		ptr = realloc( ptr, sizeof( eSize ) * ( ptrListSize + 1 ) ); \
	else { \
		ptr = malloc( sizeof( eSize ) ); \
	} \
	*(&ptr[ ptrListSize ]) = element; \
	ptrListSize++;
#endif


//send (sends a message, may take many times)
int t_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//write (write all the data in one call if you fork like this) 
	const char http_200[] = ""
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 11\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h2>Ok</h2>";
	if ( write( fd, http_200, strlen(http_200)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//read (reads a message)
int t_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message...\n" );
		close(fd);
		return 0;
	}

	return 0;
}


//Any processing can be done in the middle.  Since we're in another "thread" anyway
int h_proc ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//...
	char *err = malloc( 2048 );
	memset( err, 0, 2048 );
	struct stat sb =  {0};

	//Check that the directory exists
	if ( stat( "www", &sb ) == -1 ) {
		WRITE_HTTP_500( "Could not locate www/ directory", strerror( errno ) );
		return 0;
	}

	//Check for a primary framework file
	memset( &sb, 0, sizeof( struct stat ) );
	if ( stat( "www/main.lua", &sb ) == -1 ) {
		WRITE_HTTP_500( "Could not find file www/main.lua", strerror( errno ) );
		return 0;
	}

	//Initialize Lua environment and add a global table
	//fprintf( stderr, "Initializing Lua env.\n" );
	lua_State *L = luaL_newstate();
	luaL_openlibs( L );
	lua_newtable( L ); 

	//Put all of the HTTP data on the stack
	const char *names[] = { "headers", "get", "post" };
	struct HTTPRecord **t[] = { rq->headers, rq->url, rq->body };
	for ( int i=0; i < sizeof(t)/sizeof(struct HTTPRecord **); i++ ) {
		//Add the new name first
		lua_pushstring( L, names[ i ] ); 
		lua_newtable( L );

		//Add each value
		struct HTTPRecord **w = t[i];
		while ( *w ) {
			lua_pushstring( L, (*w)->field );
			lua_pushlstring( L, (char *)(*w)->value, (*w)->size );
			lua_settable( L, 3 );
			w++;
		}
	
		//Set this table and key as a value in the global table
		lua_settable( L, 1 );
	}

	//Push the path as well
	lua_pushstring( L, "url" );
	lua_pushstring( L, rq->path );
	lua_settable( L, 1 );

	//Additionally, the framework methods and whatnot are also needed...
	//...

	//Assign this globally
	lua_setglobal( L, "newenv" );	

	//Try running a few files and see what the stack looks like
	char *files[] = { "www/main.lua", "www/etc.lua", "www/def.lua" };
	for ( int i=0; i<3;i++ ) {
		char *f = files[i];
		int lerr = 0;  //Follow errors with this
		char fileerr[2048] = {0};
		memset( fileerr, 0, sizeof(fileerr) );

		//Load the file first 
		fprintf( stderr, "Attempting to load file: %s\n", f );
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
			WRITE_HTTP_500( fileerr, (char *)lua_tostring( L, -1 ) );
			lua_pop( L, lua_gettop( L ) );
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
			//fprintf(stderr, "LUA EXECUTE ERROR: %s, stack top is: %d\n", fileerr, lua_gettop(L) );
			WRITE_HTTP_500( fileerr, (char *)lua_tostring( L, -1 ) );
			lua_pop( L, lua_gettop( L ) );
			break;
		}
	}

	//If we're here, and the stack has values, start running renders
	//This is the model.
	lua_stackdump( L );

	//View

	//Close Lua environment
	//fprintf( stderr, "Killing Lua env.\n" );
	lua_close( L );	

#if 1
	//Write the response (this should really be a thing of its own)
	if ( write( fd, http_200_fixed, strlen(http_200_fixed)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}
#endif

	return 0;
}


int proc_echo ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {

	//Allocate a big buffer and do work
	int progress = 0;
	char *buf = malloc( 6 );
	const char *names[] = { "Headers", "GET", "POST" };
	struct HTTPRecord **t[] = { rq->headers, rq->url, rq->body };
	char sbuf[ 4096 ];
	memset( sbuf, 0, sizeof( sbuf ) );

	//Reallocate a buffer
	int suplen = snprintf( sbuf, sizeof(sbuf)-1, "<h2>URL</h2>\n%s<br>\n", rq->path );
	if ( ( buf = realloc( buf, suplen ) ) == NULL ) {
		write( fd, http_500_fixed, strlen( http_500_fixed ) );
		close( fd );
		return 0;
	}

	//Add the URL (paths should never be more than 2048, but ensure this before write)
	memcpy( &buf[ progress ], sbuf, suplen );
	progress += suplen;

	//Loop through all three...
	for ( int i=0; i < sizeof(t)/sizeof(struct HTTPRecord **); i++ ) {
		
		//Define stuff
		struct HTTPRecord **w = t[i];
		char *endstr = ( *w ) ? "\n" : "\n-<br>\n";
		memset( sbuf, 0, sizeof(sbuf) );
		suplen = snprintf( sbuf, sizeof(sbuf)-1, "<h2>%s</h2>%s", names[i], endstr );

		//Reallocate a buffer
		if ( ( buf = realloc( buf, suplen + progress ) ) == NULL ) {
			write( fd, http_500_fixed, strlen( http_500_fixed ) );
			close( fd );
			return 0;
		}

		//Write the title out
		memset( &buf[ progress ], 0, suplen );
		memcpy( &buf[ progress ], sbuf, suplen );
		progress += suplen;

		//Now go through the rest
		while ( *w ) {
			struct HTTPRecord *r = *w;	
			int fieldLen = strlen( r->field );
			//Allocate enough for fields and length of strings: ' => ', '<br>', '\n' & '\0'
			int newSize = fieldLen + r->size + 10; 
			//Return early on lack of memory...
			if ( ( buf = realloc( buf, newSize + progress ) ) == NULL ) {
				write( fd, http_500_fixed, strlen(http_500_fixed) );
				close( fd );
				return 0;
			}

			//Initialize the new memory
			memset( &buf[ progress ], 0, newSize );

			//Go through and copy everything else.
			memcpy( &buf[ progress ], r->field, fieldLen );
			progress += fieldLen;
			memcpy( &buf[ progress ], " => ", 4 );
			progress += 4;
			memcpy( &buf[ progress ], r->value, r->size ); 
			progress += r->size;
			memcpy( &buf[ progress ], "<br>\n", 5 ); 
			progress += 5;
			w++;
		}
	}

	//Get the length of the format string and allocate enough for buffer and thing
	int sendLen = strlen( http_200_custom ) + 6 + progress; //get the length of number 
	int actualLen = 0, written = 0;
	char *sendBuf = malloc( sendLen );	
	memset( sendBuf, 0, sendLen );
	written = snprintf( sendBuf, sendLen,	http_200_custom, progress );
	memcpy( &sendBuf[ written ], buf, progress );
	actualLen = written + progress;

	//Send the message to server, and see if it's read or not...
	if ( write( fd, sendBuf, actualLen ) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//Write
int h_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	//if ( 1 ) write( 2, rq->msg, rq->mlen );
	int sent = 0;
	int total = rs->mlen;
	int pos = 0;
	int try = 0;

	while ( 1 ) { 
		if (( sent = send( fd, &rs->msg[ pos ], total, MSG_DONTWAIT )) == -1 ) {
			if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED )
				close(fd);
			else if ( errno == EFAULT )
				close(fd);
			else if ( errno == EINTR )
				close(fd);
			else if ( errno == EINVAL )
				close(fd);
			else if ( errno == ENOMEM )
				close(fd);
			else if ( errno == ENOTCONN )
				close(fd);
			else if ( errno == ENOTSOCK )
				close(fd);
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
				 #ifdef HTTP_VERBOSE
					fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
				 #endif
					fprintf(stderr, "rs->mlen: %d\n", rs->mlen );
					//rq->msg = buf;
					break;
				}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
			}
		}
		else if ( sent == 0 ) {

		}
		else {
			//continue resending...
			pos += sent;
			total -= sent;	
		}
	}
	return 0;
}


#if 0
//Return the total bytes read.
int read_from_socket ( int fd, uint8_t **b, void (*readMore)(int *, void *) ) {
	return 0;
	int mult = 0;
	int try=0;
	int mlen = 0;
	const int size = 32767;	
	uint8_t *buf = malloc( 1 );

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//read into new buffer
		//if (( rd = read( fd, &buf[ rq->mlen ], size )) == -1 ) {
		//TODO: Yay!  This works great on Arch!  But let's see what about Win, OSX and BSD
		if (( rd = recv( fd, buf2, size, MSG_DONTWAIT )) == -1 ) {
			//A subsequent call will tell us a lot...
			fprintf(stderr, "Couldn't read all of message...\n" );
			//whatsockerr(errno);
			if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED )
				close(fd);
			else if ( errno == EFAULT )
				close(fd);
			else if ( errno == EINTR )
				close(fd);
			else if ( errno == EINVAL )
				close(fd);
			else if ( errno == ENOMEM )
				close(fd);
			else if ( errno == ENOTCONN )
				close(fd);
			else if ( errno == ENOTSOCK )
				close(fd);
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
				 #ifdef HTTP_VERBOSE
					fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
				 #endif
					fprintf(stderr, "mlen: %d\n", mlen );
					//fprintf(stderr, "%p\n", buf );
					//rq->msg = buf;
					break;
			}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
				fprintf(stderr, "Uncaught condition at read!\n" );
				return 0;
			}
		}
		else if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			//rq->msg = buf;
			break;
		}
		else {
			//realloc manually and read
			if (( b = realloc( b, bfsize )) == NULL ) {
				fprintf(stderr, "Couldn't allocate buffer..." );
				close(fd);
				return 0;
			}

			//Copy new data and increment bytes read
			memset( &b[ bfsize - size ], 0, size ); 
			fprintf(stderr, "pos: %d\n", bfsize - size );
			memcpy( &b[ bfsize - size ], buf2, rd ); 
			mlen += rd;
			//rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			if ( 1 ) {
				fprintf( stderr, "Recvd %d bytes on fd %d\n", rd, fd ); 
			}
		}
	}

	return mlen;
}
#endif


int write_to_socket ( int fd, uint8_t *b ) {
	return 0;
}

int sread_from_socket ( int fd, uint8_t *b ) {
	return 0;
}

int swrite_to_socket ( int fd, uint8_t *b ) {
	return 0;
}


int h_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *sess ) {

	//Read all the data from a socket.
	unsigned char *buf = malloc( 1 );
	int mult = 0;
	int try=0;
	const int size = 32767;	

	//Read first
	while ( 1 ) {
		int rd=0;
		int bfsize = size * (++mult); 
		unsigned char buf2[ size ]; 
		memset( buf2, 0, size );

		//Read functions...
		if ( !sess )
			rd = recv( fd, buf2, size, MSG_DONTWAIT );
		else {
			gnutls_session_t *ss = (gnutls_session_t *)sess;
			rd = gnutls_record_recv( *ss, buf2, size );
			if ( rd > 0 ) 
				fprintf(stderr, "SSL might be fine... got %d from gnutls_record_recv\n", rd );
			else if ( rd == GNUTLS_E_REHANDSHAKE ) {
				fprintf(stderr, "SSL got handshake reauth request..." );
				//TODO: There should be a seperate function that handles this.
				//It's a fail for now...	
				return 0;
			}
			else if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
				fprintf(stderr, "SSL was interrupted...  Try request again...\n" );
				continue;
			}
			else {
				fprintf(stderr, "SSL got error code: %d, meaning '%s'.\n", rd, gnutls_strerror( rd ) );
				continue;
			}
		}

		//read into new buffer
		//TODO: Yay!  This works great on Arch!  But let's see what about Win, OSX and BSD
		if ( rd == -1 ) {
			//A subsequent call will tell us a lot...
			fprintf(stderr, "Couldn't read all of message...\n" );
			whatsockerr( errno );
			if ( 0 )
				0;
			//ssl stuff has to go first...
			else if ( errno == EBADF )
				0; //TODO: Can't close a most-likely closed socket.  What do you do?
			else if ( errno == ECONNREFUSED )
				close(fd);
			else if ( errno == EFAULT )
				close(fd);
			else if ( errno == EINTR )
				close(fd);
			else if ( errno == EINVAL )
				close(fd);
			else if ( errno == ENOMEM )
				close(fd);
			else if ( errno == ENOTCONN )
				close(fd);
			else if ( errno == ENOTSOCK )
				close(fd);
			else if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
				if ( ++try == 2 ) {
				 #ifdef HTTP_VERBOSE
					fprintf(stderr, "Tried three times to read from socket. We're done.\n" );
				 #endif
					fprintf(stderr, "rq->mlen: %d\n", rq->mlen );
					fprintf(stderr, "%p\n", buf );
					//rq->msg = buf;
					break;
			}
			 #ifdef HTTP_VERBOSE
				fprintf(stderr, "Tried %d times to read from socket. Trying again?.\n", try );
			 #endif
			}
			else {
				//this would just be some uncaught condition...
			}
		}
		else if ( rd == 0 ) {
			//will a zero ALWAYS be returned?
			rq->msg = buf;
			break;
		}
	#if 0
		else if ( rd == GNUTLS_E_REHANDSHAKE ) {
			fprintf(stderr, "SSL got handshake reauth request..." );
			continue;
		}
		else if ( rd == GNUTLS_E_INTERRUPTED || rd == GNUTLS_E_AGAIN ) {
			fprintf(stderr, "SSL was interrupted...  Try request again...\n" );
			continue;
		}
	#endif
		else {
			//realloc manually and read
			if ((buf = realloc( buf, bfsize )) == NULL ) {
				fprintf(stderr, "Couldn't allocate buffer..." );
				close(fd);
				return 0;
			}

			//Copy new data and increment bytes read
			memset( &buf[ bfsize - size ], 0, size ); 
			fprintf(stderr, "buf: %p\n", buf );
			fprintf(stderr, "buf2: %p\n", buf2 );
			fprintf(stderr, "pos: %d\n", bfsize - size );
			memcpy( &buf[ bfsize - size ], buf2, rd ); 
			rq->mlen += rd;
			rq->msg = buf; //TODO: You keep resetting this, only needs to be done once...

			//show read progress and data received, etc.
			if ( 1 ) {
				fprintf( stderr, "Recvd %d bytes on fd %d\n", rd, fd ); 
			}
		}
	}

	//Prepare the rest of the request
	char *header = (char *)rq->msg;
	int pLen = memchrat( rq->msg, '\n', rq->mlen ) - 1;
	const int flLen = pLen + strlen( "\r\n" );
	int hdLen = memstrat( rq->msg, "\r\n\r\n", rq->mlen );

	//Initialize the remainder of variables 
	rq->headers = NULL;
	rq->body = NULL;
	rq->url = NULL;
	rq->method = get_lstr( &header, ' ', &pLen );
	rq->path = get_lstr( &header, ' ', &pLen );
	rq->protocol = get_lstr( &header, ' ', &pLen ); 
	rq->hlen = hdLen; 
	rq->host = msg_get_value( "Host: ", "\r", rq->msg, hdLen );

	//The protocol parsing can happen here...
	if ( strcmp( rq->method, "HEAD" ) == 0 )
		;
	else if ( strcmp( rq->method, "GET" ) == 0 )
		;
	else if ( strcmp( rq->method, "POST" ) == 0 ) {
		rq->clen = safeatoi( msg_get_value( "Content-Length: ", "\r", rq->msg, hdLen ) );
		rq->ctype = msg_get_value( "Content-Type: ", ";\r", rq->msg, hdLen );
		rq->boundary = msg_get_value( "boundary=", "\r", rq->msg, hdLen );
		//rq->mlen = hdLen; 
		//If clen is -1, ... hmmm.  At some point, I still need to do the rest of the work. 
	}

	if ( 0 )  {
		print_httpbody( rq );	
	}

	//Define records for each type here...
	//struct HTTPRecord **url=NULL, **headers=NULL, **body=NULL;
	int len = 0;
	Mem set;
	memset( &set, 0, sizeof( Mem ) );

	//Always process the URL (specifically GET vars)
	if ( strlen( rq->path ) == 1 ) {
		ADDITEM( NULL, struct HTTPRecord, rq->url, len, 0 );
	}
	else {
		int index = 0;
		while ( strwalk( &set, rq->path, "?&" ) ) {
			uint8_t *t = (uint8_t *)&rq->path[ set.pos ];
			struct HTTPRecord *b = malloc( sizeof( struct HTTPRecord ) );
			memset( b, 0, sizeof( struct HTTPRecord ) );
			int at = memchrat( t, '=', set.size );
			if ( !b || at == -1 || !set.size ) 
				;
			else {
				int klen = at;
				b->field = copystr( t, klen );
				klen += 1, t += klen, set.size -= klen;
				b->value = t;
				b->size = set.size;
				ADDITEM( b, struct HTTPRecord, rq->url, len, 0 );
			}
		}
		ADDITEM( NULL, struct HTTPRecord, rq->url, len, 0 );
	}


	//Always process the headers
	memset( &set, 0, sizeof( Mem ) );
	len = 0;
	uint8_t *h = &rq->msg[ flLen - 1 ];
	while ( memwalk( &set, h, (uint8_t *)"\r", rq->hlen, 1 ) ) {
		//Break on newline, and extract the _first_ ':'
		uint8_t *t = &h[ set.pos - 1 ];
		if ( *t == '\r' ) {  
			int at = memchrat( t, ':', set.size );
			struct HTTPRecord *b = malloc( sizeof( struct HTTPRecord ) );
			memset( b, 0, sizeof( struct HTTPRecord ) );
			if ( !b || at == -1 || at > 127 )
				;
			else {
				at -= 2, t += 2;
				b->field = copystr( t, at );
				at += 2 /*Increment to get past ': '*/, t += at, set.size -= at;
				b->value = t;
				b->size = set.size - 1;
			#if 0
				DUMP_RIGHT( b->value, b->size );
			#endif
				ADDITEM( b, struct HTTPRecord, rq->headers, len, 0 );
			}
		}
	}
	ADDITEM( NULL, struct HTTPRecord, rq->headers, len, 0 );

	//Always process the body 
	memset( &set, 0, sizeof( Mem ) );
	len = 0;
	uint8_t *p = &rq->msg[ rq->hlen + strlen( "\r\n" ) ];
	int plen = rq->mlen - rq->hlen;
	
	//TODO: If this is a xfer-encoding chunked msg, rq->clen needs to get filled in when done.
	if ( strcmp( "POST", rq->method ) != 0 ) {
		ADDITEM( NULL, struct HTTPRecord, rq->body, len, 0 );
	}
	else {
		struct HTTPRecord *b = NULL;
		#if 0
		DUMP_RIGHT( p, rq->mlen - rq->hlen ); 
		#endif
		//TODO: Bitmasking is 1% more efficient, go for it.
		int name=0, value=0, index=0;

		//url encoded is a little bit different.  no real reason to use the same code...
		if ( strcmp( rq->ctype, "application/x-www-form-urlencoded" ) == 0 ) {
			//NOTE: clen is up by two to assist our little tokenizer...
			while ( memwalk( &set, p, (uint8_t *)"\n=&", rq->clen + 2, 3 ) ) {
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( *m == '\n' || *m == '&' ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
					//TODO: Should be checking that allocation was successful
					b->field = copystr( ++m, set.size );
				}
				else if ( *m == '=' ) {
					b->value = ++m;
					b->size = set.size;
					ADDITEM( b, struct HTTPRecord, rq->body, len, 0 );
					b = NULL;
				}
			}
		}
		else {
			while ( memwalk( &set, p, (uint8_t *)"\r:=;", rq->clen, 4 ) ) {
				//TODO: If we're being technical, set.pos - 1 can point to a negative index.  
				//However, as long as headers were sent (and 99.99999999% of the time they will be)
				//this negative index will point to valid allocated memory...
				uint8_t *m = &p[ set.pos - 1 ];  
				if ( memcmp( m, "; name=", 7 ) == 0 )
					name = 1;
				//"\r\n\r\n"
				else if ( memcmp( m, "\r\n\r\n", 4 ) == 0 && !value )
					value = 1;
				else if ( memcmp( m, "\r\n-", 3 ) == 0 && !value ) {
					b = malloc( sizeof( struct HTTPRecord ) );
					memset( b, 0, sizeof( struct HTTPRecord ) ); 
				}
				else if ( memcmp( m, "\r\n", 2 ) == 0 && value == 1 ) {
					m += 2;
					b->value = m;//++t;
					b->size = set.size - 1;
					ADDITEM( b, struct HTTPRecord, rq->body, len, 0 );
					value = 0;
					b = NULL;
				}
				else if ( *m == '=' && name == 1 ) {
					//fprintf( stderr, "copying name field... pass %d\n", ++index );
					int size = *(m + 1) == '"' ? set.size - 2 : set.size;
				#if 1
					m += ( *(m + 1) == '"' ) ? 2 : 1 ;
				#else
					int ptrinc = *(m + 1) == '"' ? 2 : 1;
					m += ptrinc;
				#endif
					b->field = copystr( m, size );
					name = 0;
				}
			}
		}

		//Add a terminator element
		ADDITEM( NULL, struct HTTPRecord, rq->body, len, 0 );
		//This MAY help in handling malformed messages...
		( b && (!b->field || !b->value) ) ? free( b ) : 0;

		if ( 0 ) {
			fprintf( stderr, "BODY got:\n" );
			print_httprecords( rq->body );
		}
	}

	//for testing, this should stay here...
	//close(fd);
	return 0;
}


int ssl_write ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//write (write all the data in one call if you fork like this) 
	if ( write( fd, http_200_fixed, strlen(http_200_fixed)) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}

	return 0;
}


//read (reads a message)
int ssl_read ( int fd, struct HTTPBody *rq, struct HTTPBody *rs ) {
	//read (read all the data in one call if you fork like this)
	const int size = 100000;
	unsigned char *rqb = malloc( size );
	memset( rqb, 0, size );	
	if (( rq->mlen = read( fd, rqb, size )) == -1 ) {
		fprintf(stderr, "Couldn't read all of message...\n" );
		close(fd);
		return 0;
	}

	return 0;
}


struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	int (*pre)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*post)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void *readf;
	void *writef;
} sr[] = {
	{ h_read, /*proc_echo*/ h_proc, h_write }
, { t_read, NULL, t_write  }
,	{ NULL }
};



int main (int argc, char *argv[]) {
	struct values {
		int port;
		int ssl;
		int start;
		int kill;
		int fork;
		char *user;
	} values = { 0 };

	//Process all your options...
	if ( argc < 2 ) {
		fprintf( stderr, "%s: No options received.\n", __FILE__ );
		const char *fmt = "  --%-10s       %-30s\n";
		fprintf( stderr, fmt, "start", "start new servers" );
		fprintf( stderr, fmt, "kill", "test killing a server" );
		fprintf( stderr, fmt, "fork", "daemonize the server" );
		fprintf( stderr, fmt, "port <arg>", "set a differnt port" );
		fprintf( stderr, fmt, "ssl", "use ssl or not.." );
		fprintf( stderr, fmt, "user <arg>", "choose a user to start as" );
		return 0;	
	}	
	else {
		while ( *argv ) {
			if ( strcmp( *argv, "--start" ) == 0 ) 
				values.start = 1;
			else if ( strcmp( *argv, "--kill" ) == 0 ) 
				values.kill = 1;
			else if ( strcmp( *argv, "--ssl" ) == 0 ) 
				values.ssl = 1;
			else if ( strcmp( *argv, "--daemonize" ) == 0 ) 
				values.fork = 1;
			else if ( strcmp( *argv, "--port" ) == 0 ) {
				argv++;
				if ( !*argv ) {
					fprintf( stderr, "Expected argument for --port!" );
					return 0;
				} 
				values.port = atoi( *argv );
			}
			else if ( strcmp( *argv, "--user" ) == 0 ) {
				argv++;
				if ( !*argv ) {
					fprintf( stderr, "Expected argument for --user!" );
					return 0;
				} 
				values.user = strdup( *argv );
			}
			argv++;
		}
	}	

	if ( 1 ) {
		const char *fmt = "%-10s: %s\n";
		fprintf( stderr, "Invoked with options:\n" );
		fprintf( stderr, "%10s: %d\n", "start", values.start );	
		fprintf( stderr, "%10s: %d\n", "kill", values.kill );	
		fprintf( stderr, "%10s: %d\n", "port", values.port );	
		fprintf( stderr, "%10s: %d\n", "fork", values.fork );	
		fprintf( stderr, "%10s: %s\n", "user", values.user );	
		fprintf( stderr, "%10s: %s\n", "ssl", values.ssl ? "true" : "false" );	
	}


	//Set all of the socket stuff
	const int defport = 2000;
	struct sockAbstr su;
	su.addrsize = sizeof(struct sockaddr);
	su.buffersize = 1024;
	su.opened = 0;
	su.backlog = 500;
	su.waittime = 5000;
	su.protocol = IPPROTO_TCP;
	su.socktype = SOCK_STREAM;
	//su.protocol = IPPROTO_UDP;
	//su.sockettype = SOCK_DGRAM;
	su.iptype = PF_INET;
	su.reuse = SO_REUSEADDR;
	su.port = !values.port ? (int *)&defport : &values.port;
	su.ssl_ctx = NULL;

	if ( 1 ) {
		const char *fmt = "%-10s: %s\n";
		FILE *e = stderr;
		fprintf( e, "Socket opened with options:\n" );
		fprintf( e, "%10s: %d\n", "addrsize", su.addrsize );	
		fprintf( e, "%10s: %d\n", "buffersize", su.buffersize );	
		fprintf( e, "%10s: %d connections\n", "backlog", su.backlog );	
		fprintf( e, "%10s: %d microseconds\n", "waittime", su.waittime );	
		fprintf( e, "%10s: %s\n", "protocol", su.protocol == IPPROTO_TCP ? "tcp" : "udp" );	
		fprintf( e, "%10s: %s\n", "socktype", su.socktype == SOCK_STREAM ? "stream" : "datagram" );	 
		fprintf( e, "%10s: %s\n", "IPv6?", su.iptype == PF_INET ? "no" : "yes" );	
		fprintf( e, "%10s: %d\n", "port", *su.port );	
		//How to dump all the socket info?
	}

	//Create the socket body
	struct sockaddr_in t;
	memset( &t, 0, sizeof( struct sockaddr_in ) );
	struct sockaddr_in *sa = &t;
	sa->sin_family = su.iptype; 
	sa->sin_port = htons( *su.port );
	(&sa->sin_addr)->s_addr = htonl( INADDR_ANY );

	//Open the socket (non-blocking, preferably)
	int status;
	if (( su.fd = socket( su.iptype, su.socktype, su.protocol )) == -1 ) {
		fprintf( stderr, "Couldn't open socket! Error: %s\n", strerror( errno ) );
		return 0;
	}

	#if 0
	//Set timeout, reusable bit and any other options 
	struct timespec to = { .tv_sec = 2 };
	if (setsockopt(su.fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1) {
		// su.free(sock);
		su.err = errno;
		return (0, "Could not reopen socket.");
	}
	#endif
	if ( fcntl( su.fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		fprintf( stderr, "fcntl error: %s\n", strerror(errno) ); 
		return 0;
	}

	if (( status = bind( su.fd, (struct sockaddr *)&t, sizeof(struct sockaddr_in))) == -1 ) {
		fprintf( stderr, "Couldn't bind socket to address! Error: %s\n", strerror( errno ) );
		return 0;
	}

	if (( status = listen( su.fd, su.backlog) ) == -1 ) {
		fprintf( stderr, "Couldn't listen for connections! Error: %s\n", strerror( errno ) );
		return 0;
	}

	//Mark open flag.
	su.opened = 1;

	//Handle SSL here
	#if 1 
	gnutls_certificate_credentials_t x509_cred = NULL;
  gnutls_priority_t priority_cache;
	const char *cafile, *crlfile, *certfile, *keyfile;
	#if 0
	cafile = 
	crlfile = 
	#endif
	#if 0
	//These should always be loaded, and there will almost always be a series
	certfile = 
	keyfile = 
	#else
#define MPATH "/home/ramar/prj/hypno/certs/collinsdesign.net"
	//Hardcode these for now.
	certfile = MPATH "/collinsdesign_net.crt";
	keyfile = MPATH "/server.key";
	#endif
	//Obviously, this is great for debugging TLS errors...
	//gnutls_global_set_log_function( tls_log_func );
	gnutls_global_init();
	gnutls_certificate_allocate_credentials( &x509_cred );
	//find the certificate authority to use
	//gnutls_certificate_set_x509_trust_file( x509_cred, cafile, GNUTLS_X509_FMT_PEM );
	//is this for password-protected cert files? I'm so lost...
	//gnutls_certificate_set_x509_crl_file( x509_cred, crlfile, GNUTLS_X509_FMT_PEM );
	//this ought to work with server.key and certfile
	gnutls_certificate_set_x509_key_file( x509_cred, certfile, keyfile, GNUTLS_X509_FMT_PEM );
	//gnutls_certificate_set_ocsp_status_request( x509_cred, OCSP_STATUS_FiLE, 0 );
	gnutls_priority_init( &priority_cache, NULL, NULL );
	#endif

	//If I could open a socket and listen successfully, then write the PID
	#if 0
	if ( values.fork ) {
		pid_t pid = fork();
		if ( pid == -1 ) {
			fprintf( stderr, "Failed to daemonize server process: %s", strerror(errno) );
			return 0;
		}
		else if ( !pid ) {
			//Close the parent?
			return 0;
		}
		else if ( pid ) {
			int len, fd = 0;
			char buf[64] = { 0 };

			if ( (fd = open( pidfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR )) == -1 )
				return ERRL( "Failed to access PID file: %s.", strerror(errno));

			len = snprintf( buf, 63, "%d", pid );

			//Write the pid down
			if (write( fd, buf, len ) == -1)
				return ERRL( "Failed to log PID: %s.", strerror(errno));
		
			//The parent exited successfully.
			if ( close(fd) == -1 )
				return ERRL( "Could not close parent socket: %s", strerror(errno));
			return SUC_PARENT;
		}
	}
	#endif


	//Let's start the accept loop here...
	for ( ;; ) {
		//Client address and length?
		struct sockaddr addrinfo;	
		socklen_t addrlen = sizeof (struct sockaddr);	
		int fd;	

		//Accept a connection if possible...
		if (( fd = accept( su.fd, &addrinfo, &addrlen )) == -1 ) {
			//TODO: Need to check if the socket was non-blocking or not...
			if ( 0 )
				; 
			else {
				fprintf( stderr, "Accept ran into trouble: %s\n", strerror( errno ) );
				continue;
			}
		}
#if 0
		//Make the new socket non-blocking too...
		if ( fcntl( fd, F_SETFD, O_NONBLOCK ) == -1 ) {
			fprintf( stderr, "fcntl error at child socket: %s\n", strerror(errno) ); 
			return 0;
		}
#endif

		//Dump the client info and the child fd
		if ( 1 ) {
			struct sockaddr_in *cin = (struct sockaddr_in *)&addrinfo;
			char *ip = inet_ntoa( cin->sin_addr );
			fprintf( stderr, "Got request from: %s on new file: %d\n", ip, fd );	
		}

		//Fork and go crazy
		pid_t cpid = fork();
		if ( cpid == -1 ) {
			//TODO: There is most likely a reason this didn't work.
			fprintf( stderr, "Failed to setup new child connection. %s\n", strerror(errno) );
		}
		else if ( cpid == 0 ) {
			//TODO: The parent should probably log some important info here.	
			fprintf(stderr, "in parent...\n" );
			//Close the file descriptor here?
			if ( close( fd ) == -1 ) {
				fprintf( stderr, "Parent couldn't close socket." );
			}
		}
		else if ( cpid ) {
			//TODO: Somewhere in here, a signal needs to run that allows this thing to die.
			//TODO: Handle read and write errno cases 
			#if 1
			//SSL again
			gnutls_session_t session, *sptr = NULL;
			if ( values.ssl ) {
				gnutls_init( &session, GNUTLS_SERVER );
				gnutls_priority_set( session, priority_cache );
				gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, x509_cred );
				//NOTE: I need to do this b/c clients aren't expected to send a certificate with their request
				gnutls_certificate_server_set_request( session, GNUTLS_CERT_IGNORE ); 
				gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT ); 
				//Bind the current file descriptor to GnuTLS instance.
				gnutls_transport_set_int( session, fd );
				//Do the handshake here
				//TODO: I write nothing that looks like this, please refactor it...
				int success = 0;
				do {
					success = gnutls_handshake( session );
				} while ( success == GNUTLS_E_AGAIN || success == GNUTLS_E_INTERRUPTED );	
				if ( success < 0 ) {
					close( fd );
					gnutls_deinit( session );
					//TODO: Log all handshake failures.  Still don't know where.
					fprintf( stderr, "%s\n", "SSL handshake failed." );
					continue;
				}
				fprintf( stderr, "%s\n", "SSL handshake successful." );
				sptr = &session;
			}
			#endif

			//All the processing occurs here.
			struct HTTPBody rq = { 0 }; 
			struct HTTPBody rs = { 0 };
			struct senderrecvr *f = &sr[ 0 ]; 
#if 0
			//TODO: This doesn't seem quite optimal, but I'm doing it.
			struct SSLContext ssl = {
				.read = gnutls_record_recv
			, .write = gnutls_record_send
			, .data = (void *)&rq
			}; 
#endif
			//Read the message	
			if (( status = f->read( fd, &rq, &rs, sptr )) == -1 ) {
				//what to do with the response...
			}

			//Generate a new message	
			if ( f->proc && ( status = f->proc( fd, &rq, &rs, NULL )) == -1 ) {
				//...
			}
			
			//Write a new message	
			if (( status = f->write( fd, &rq, &rs, &session )) == -1 ) {
				//...
			}

			if ( close( fd ) == -1 ) {
				fprintf( stderr, "Couldn't close child socket. %s\n", strerror(errno) );
				return 0;
			}
		}

	}

	//Close the server process.
	if ( close( su.fd ) == -1 ) {
		fprintf( stderr, "Couldn't close socket! Error: %s\n", strerror( errno ) );
		return 0;
	}

	return 0;
}

