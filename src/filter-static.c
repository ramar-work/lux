#include "filter-static.h"

int send_basic(int fd, char *err) {
	fprintf( stderr, "%s\n", err );
	if ( write( fd, http_200_fixed, strlen( http_200_fixed ) ) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}
	return 1;
}


char *getExtension ( char *filename ) {
	char *extension = &filename[ strlen( filename ) ];
	int fpathlen = strlen( filename );
	while ( fpathlen-- ) {
		if ( *(--extension) == '.' ) break; 
	}
	return ( !fpathlen ) ? NULL : extension;
}


int filter_static ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	struct stat sb;
	int file = 0;
	int len = 0;
	int size = 0;
	char err[ 1024 ] = { 0 };
	int errlen = sizeof(err); 
	char fpath[ 2048 ] = { 0 };
	char *extension = NULL;
	const char *mimetype_default = mmtref( "application/octet-stream" );
	const char *mimetype = NULL;
	uint8_t *content = NULL;
	char headerbuf[ 2048 ] = { 0 };
	const char headerfmt[] = 
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: %ld\r\n"
		"Content-Type: %s\r\n\r\n";

	fprintf( stderr, "Static filter was called!\n" );
	fprintf( stderr, "Path is: %s\n", rq->path );
	//send_basic( fd, err );

	/*
	path = /, check for routes.  if none, serve default
	ANYTHING ELSE, check that the file exists (relative to what though?)
	*/

	//Stop / requests when dealing with static servers
	if ( strlen( rq->path ) == 1 && *rq->path == '/' ) {
		//?
		return send_basic( fd, err );
	}
		
	//Create a fullpath
	if ( snprintf( fpath, sizeof(fpath) - 1, "%s%s", "www/dafoodsnob", rq->path ) == -1 ) {
		//500, something else is happening in this case
		return send_basic( fd, err );
	}

	//Crudely check the extension before serving.
	mimetype = mimetype_default;
	if ( ( extension = getExtension( fpath ) ) ) {
		extension++;
		if ( ( mimetype = mmimetype_from_file( extension ) ) == NULL ) {
			mimetype = mimetype_default;
		}
	}
	fprintf( stderr, "mimetype of file to serve: %s\n", mimetype );

#if 0	
	//Read it all
	if ( !( content = read_file( fpath, &len, err, sizeof(err) ) ) ) {
		fprintf( stderr, "%s\n", err );
		//errors reading are 500
		return send_basic( fd, err );
	}
#else
	//Check for the file 
	if ( stat( fpath, &sb ) == -1 ) {
		//fprintf( stderr, "FILE STAT ERROR: %s\n", strerror( errno ) );
		snprintf( err, errlen, "FILE STAT ERROR: %s\n", strerror( errno ) );
		//404
		return send_basic( fd, err );
	}

	//Check for the file 
	if ( ( file = open( fpath, O_RDONLY ) ) == -1 ) {
		snprintf( err, errlen, "FILE OPEN ERROR: %s\n", strerror( errno ) );
		//depends on type of problem (permission, corrupt, etc)
		return send_basic( fd, err );
	}

	//Allocate a buffer
	sb.st_size++;
	if ( !( content = malloc( sb.st_size ) ) || !memset( content, 0, sb.st_size ) ) {
		snprintf( err, errlen, "COULD NOT OPEN VIEW FILE: %s\n", strerror( errno ) );
		//500 (definite server issue)
		return send_basic( fd, err );	
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( ( size = read( file, content, sb.st_size - 1 )) == -1 ) {
		snprintf( err, errlen, "COULD NOT READ ALL OF VIEW FILE: %s\n", strerror( errno ) );
		free( content );
		close( fd );
		//500 (definite server issue)
		return send_basic( fd, err );	
	}

	//This should have happened before...
	if ( close( file ) == -1 ) {
		snprintf( err, errlen, "COULD NOT CLOSE FILE %s: %s\n", fpath, strerror( errno ) );
		free( content );
		//500 (definite server issue)
		return send_basic( fd, err );	
	}

	//Now, write the file with the right content type to socket
	int hlen = snprintf( headerbuf, sizeof( headerbuf ), headerfmt, sb.st_size, mimetype );
	size = ( sb.st_size - 1 ) + hlen;
	if ( !( content = realloc( content, size ) ) ) {
		snprintf( err, errlen, "MEMORY ALLOCATION FAILURE: %s\n", strerror( errno ) );
		free( content );
		return send_basic( fd, err );
	} 

	if ( !memmove( &content[ hlen ], content, sb.st_size - 1 ) ) {
		snprintf( err, errlen, "MEMORY MOVE FAILURE: %s\n", strerror( errno ) );
		free( content );
		return send_basic( fd, err );
	}

	if ( !memcpy( content, headerbuf, hlen ) ) {
		snprintf( err, errlen, "MEMORY COPY FAILURE: %s\n", strerror( errno ) );
		free( content );
		return send_basic( fd, err );
	}

	if ( write( fd, content, size ) == -1 ) {
		snprintf( err, errlen, "MESSAGE WRITE FAILURE: %s\n", strerror( errno ) );
		free( content );
		close( fd );
		return send_basic( fd, err );
	}

	free( content );
#endif

#if 0
	return 0;
		if ( strcmp( mt, "application/octet-stream" ) > 0 ) {
			//Check for the path name relative to the currently chosen directory
			HttpStreamer *hs = malloc( sizeof( HttpStreamer ) );
			memset( hs, 0, sizeof(HttpStreamer) );
			hs->filename = strcmbd( "/", pt->activeDir, &h->request.path[ 1 ] );
			hs->fd = 0;
			hs->size = 0;
			hs->bufsize = 1028;

			if ( stat( hs->filename, &sb ) == -1 ) {	
				free_hs( hs );
				return ERR_404( "File not found: %s", hs->filename );
			}

			if (( hs->size = sb.st_size ) == 0 ) {
				free_hs( hs );
				return ERR_500( "Requested file: %s is zero length.", hs->filename );
			}

			if (( hs->fd = open( hs->filename, O_RDONLY ) ) == -1 ) {
				free_hs( hs );
				return ERR_500( "Requested file: %s could not be opened.", hs->filename );
			}

			//Prepare the actual reponse
			h->userdata = hs;
			http_set_status( h, 200 );
			http_set_content_type( h, mt );
			http_set_header( h, "Transfer-Encoding", "chunked" );
			http_pack_response( h );
			r->stage = NW_AT_WRITE;
			*r->bypass = 1;
			return 1;
		}
	}
#endif
	return 0;
}
