#include "filter-static.h"

#if 0
int send_basic(int fd, char *err) {
	fprintf( stderr, "%s\n", err );
	if ( write( fd, http_200_fixed, strlen( http_200_fixed ) ) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}
	return 1;
}
#endif

char *getExtension ( char *filename ) {
	char *extension = &filename[ strlen( filename ) ];
	int fpathlen = strlen( filename );
	while ( fpathlen-- ) {
		if ( *(--extension) == '.' ) break; 
	}
	return ( !fpathlen ) ? NULL : extension;
}


int filter_static ( struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {
	struct stat sb;
	int fd = 0;
	int size = 0;
	char err[ 2048 ] = { 0 };
	char fpath[ 2048 ] = { 0 };
	char *fname = rq->path;
	char *extension = NULL;
	const char *mimetype_default = mmtref( "application/octet-stream" );
	const char *mimetype = NULL;
	uint8_t *content = NULL;
	struct config *config = (struct config *)ctx;

	if ( !config->path )
		return http_set_error( rs, 500, "No directory path specified for this site." );

	//Stop / requests when dealing with static servers
	if ( strlen( rq->path ) == 1 && *rq->path == '/' ) {
		//Check for a default page (like index.html, which comes from config)
		if ( !config->root_default ) {
			return http_set_error( rs, 404, "No default root specified for this site." );
		}
		fname = (char *)config->root_default; 
	}
		
	//Create a fullpath
	if ( snprintf( fpath, sizeof(fpath) - 1, "%s%s", config->path, fname ) == -1 )
		return http_set_error( rs, 500, "Full filepath probably truncated." );

	//Crudely check the extension before serving.
	mimetype = mimetype_default;
	if ( ( extension = getExtension( fpath ) ) ) {
		extension++;
		if ( ( mimetype = mmimetype_from_file( extension ) ) == NULL ) {
			mimetype = mimetype_default;
		}
	}
	
	//Check for the file 
	if ( stat( fpath, &sb ) == -1 ) {
		snprintf( err, sizeof( err ), "FILE STAT ERROR: %s: %s.", strerror( errno ), fpath );
		return http_set_error( rs, 404, err );
	}

	//Check for the file 
	if ( ( fd = open( fpath, O_RDONLY ) ) == -1 ) {
		snprintf( err, sizeof( err ), "FILE OPEN ERROR: %s: %s.", strerror( errno ), fpath );
		//depends on type of problem (permission, corrupt, etc)
		return http_set_error( rs, 500, err );
	}

	//Allocate a buffer
	if ( !( content = malloc( ++sb.st_size ) ) || !memset( content, 0, sb.st_size ) ) {
		snprintf( err, sizeof( err ), "COULD NOT ALLOCATE SPACE FOR FILE: %s\n", strerror( errno ) );
		return http_set_error( rs, 500, err );
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( ( size = read( fd, content, sb.st_size - 1 )) == -1 ) {
		snprintf( err, sizeof( err ), "COULD NOT READ ALL FILE %s: %s.", fpath, strerror( errno ) );
		free( content );
		return http_set_error( rs, 500, err );
	}

	//This should have happened before...
	if ( close( fd ) == -1 ) {
		snprintf( err, sizeof( err ), "COULD NOT CLOSE FILE %s: %s\n", fpath, strerror( errno ) );
		free( content );
		return http_set_error( rs, 500, err );
	}

	//Just set content messages...
	http_set_status( rs, 200 );
	http_set_ctype( rs, mimetype );
	http_set_content( rs, content, size ); 
	if ( !http_finalize_response( rs, err, sizeof( err ) ) ) {
		free( content );
		return http_set_error( rs, 500, err );
	}

	free( content );
	return 1;
}
