#include "util.h"

uint8_t *read_file ( const char *filename, int *len, char *err, int errlen ) {
	//Check for and load whatever file
	int fd, fstat, bytesRead, fileSize;
	uint8_t *buf = NULL;
	struct stat sb;
	memset( &sb, 0, sizeof( struct stat ) );

	//Check for the file 
	if ( (fstat = stat( filename, &sb )) == -1 ) {
		fprintf( stderr, "FILE STAT ERROR: %s\n", strerror( errno ) );
		exit( 1 );
	}

	//Check for the file 
	if ( (fd = open( filename, O_RDONLY )) == -1 ) {
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
	if ( 0 ) {
		write( 2, buf, sb.st_size );
	}

	*len = sb.st_size;
	return buf;
}
