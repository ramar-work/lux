#include "filter-static.h"

#define FILTER_STATIC_DEBUG

#ifndef FILTER_STATIC_DEBUG
 #define FILTER_STATIC_PRINT(...)
#else
 #define FILTER_STATIC_PRINT(...) \
	fprintf( stderr, "[%s:%d]", __FILE__, __LINE__ ); \
	fprintf( stderr, __VA_ARGS__ )
#endif


char *getExtension ( char *filename ) {
	char *extension = &filename[ strlen( filename ) ];
	int fpathlen = strlen( filename );
	while ( fpathlen-- ) {
		if ( *(--extension) == '.' ) break; 
	}
	return ( !fpathlen ) ? NULL : extension;
}


int check_static_prefix( const char *path, const char *prefix ) {
	if ( !path || !prefix || strlen(path) < strlen(prefix) ) { 
		return 0;
	}
	if ( memcmp( prefix, ++path, strlen( prefix ) ) != 0 ) {
		return 0;
	} 
	return 1;
}


int filter_static ( struct HTTPBody *rq, struct HTTPBody *rs, struct config *config, struct host *host ) {
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
	//struct host *config = (struct host *)ctx;

	if ( !host->dir ) {
		return http_set_error( rs, 500, "No directory path specified for this site." );
	}

	//Stop / requests when dealing with static servers
FPRINTF( "rq->path is %s\n", rq->path );
	if ( !rq->path || ( strlen( rq->path ) == 1 && *rq->path == '/' ) ) {
		//Check for a default page (like index.html, which comes from config)
		if ( !config->root_default ) {
			return http_set_error( rs, 404, "No default root specified for this site." );
		}
		fname = (char *)config->root_default; 
	}
		
	//Create a fullpath
	if ( snprintf( fpath, sizeof(fpath) - 1, "%s%s", host->dir, fname ) == -1 ) {
		return http_set_error( rs, 500, "Full filepath probably truncated." );
	}
FPRINTF( "full path is %s\n", fpath );

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
		snprintf( err, sizeof( err ), "%s: %s.", strerror( errno ), fpath );
		return http_set_error( rs, 404, err );
	}

	//Check for the file 
	if ( ( fd = open( fpath, O_RDONLY ) ) == -1 ) {
		snprintf( err, sizeof( err ), "%s: %s.", strerror( errno ), fpath );
		//depends on type of problem (permission, corrupt, etc)
		return http_set_error( rs, 500, err );
	}

	//Allocate a buffer
	if ( !( content = malloc( sb.st_size ) ) || !memset( content, 0, sb.st_size ) ) {
		const char fmt[] = "Could not allocate space for file: %s\n";
		snprintf( err, sizeof( err ), fmt, strerror( errno ) );
		return http_set_error( rs, 500, err );
	}

	//Read the entire file into memory, b/c we'll probably have space 
	if ( ( size = read( fd, content, sb.st_size )) == -1 ) {
		const char fmt[] = "Could not read all of file %s: %s.";
		snprintf( err, sizeof( err ), fmt, fpath, strerror( errno ) );
		free( content );
		return http_set_error( rs, 500, err );
	}

	//This should have happened before...
	if ( close( fd ) == -1 ) {
		const char fmt[] = "Could not close file %s: %s\n";
		snprintf( err, sizeof( err ), fmt, fpath, strerror( errno ) );
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

	//free( content );
	return 1;
}
