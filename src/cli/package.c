//package.c
//should be used to compress and decompress stuff
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <zlib.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

#define PP "hypno-package"

#define HELP \
	"-p, --package <arg>      Define where to create a new application.\n"\
	"-d, --directory <arg>    Define where to set directory.\n"\
	"-h, --help               Dump passed arguments.\n"

struct arg {
	char *package;
	char *directory;
	char *source;
	int verbose;
	int dump;
};

//can do tar or your own thing, which ever you want...
void move_archive ( const char *name, const char *dir ) {
	int r;
	struct archive_entry *entry = NULL;
	struct archive *a = archive_read_new();

	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	if ( ( r = archive_read_open_filename( a, name, 4096 ) ) != ARCHIVE_OK ) {
		fprintf( stderr, "Archive read failed: %s\n", strerror( errno ) );
		return;
	}

	//Create a new folder?
	if ( mkdir( dir, 0755 ) == -1 ) {
		fprintf( stderr, "mkdir failed\n" );
		return;
	}

	for ( int fd = 0; archive_read_next_header( a, &entry ) == ARCHIVE_OK ; ) {
		char fmt[2048] = {0}, *bname = basename( (char *)archive_entry_pathname(entry) );
		snprintf( fmt, sizeof(fmt), "%s/%s", dir, bname );
		fd = open( fmt, O_CREAT | O_RDWR | O_TRUNC, 0755 );
		archive_read_data_into_fd( a, fd );
		close( fd );
	}

	if ( ( r = archive_read_free(a) ) != ARCHIVE_OK ) {
		fprintf( stderr, "Archive close failed: %s\n", strerror( errno ) );
		return;
	}
}


int main ( int argc, char *argv[] ) {
	struct arg arg;

	if ( argc < 2 ) {
		fprintf( stderr, PP ":\n%s\n", HELP );
		return 1;
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--package" ) )
			arg.package = *( ++argv );
		else if ( !strcmp( *argv, "-s" ) || !strcmp( *argv, "--source" ) )
			arg.source = *( ++argv );
		else if ( !strcmp( *argv, "-i" ) || !strcmp( *argv, "--install-to" ) )
			arg.directory = *( ++argv );
		else if ( !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
			fprintf( stderr, "%s\n", HELP );
			return 0;
		}	
		argv++;
	}


	//like previously, we grab stuff from a source, and unpack it
	//
	//unsure how to compress (and even what to create)

	//move_archive( *argv, "bomb" );
	return 0;
}
