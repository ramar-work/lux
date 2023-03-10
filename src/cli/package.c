/* ------------------------------------------- * 
 * package.c
 * =========
 *
 * Summary 
 * -------
 * Command line tooling to create extensions for
 * hypno.
 *
 * Usage
 * -----
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * See LICENSE in the top-level directory for more information.
 *
 * Changelog
 * ---------
 * 
 * TODO
 * ---------
 * - Compress a package (by selectively pulling things?)
 * - Create a new package (by creating a new template)
 * - De-cmopress a pcakage (to a specific location)
 * ------------------------------------------- */
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
	"-d, --directory <arg>    Define where to set directory.\n" \
	"-i, --install-to <arg>   Install a package to this instance.\n" \
	"-p, --package <arg>      Define where to create a new application.\n" \
	"-r, --repository <arg>   ?\n" \
	"-h, --help               Dump passed arguments.\n"


//...
struct arg {
	char *package;
	char *instance;
	char *source;
	char *repository;
	int verbose;
	int dump;
};


struct repository {
	char *type;
	char *address;
};


//Load the config from /etc/hypno/...


//can do tar or your own thing, which ever you want...
int unpack_archive ( const char *name, const char *dir ) {
	int s, r;
	struct archive_entry *entry = NULL;
	struct archive *a = archive_read_new();
	char rootname[ 1024 ] = {0}; 

	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);
	if ( ( r = archive_read_open_filename( a, name, 4096 ) ) != ARCHIVE_OK ) {
		fprintf( stderr, "Archive read failed: %s\n", strerror( errno ) );
		return 0;
	}

#if 0
	//Create a new folder?
	if ( mkdir( dir, 0755 ) == -1 ) {
		fprintf( stderr, "mkdir failed\n" );
		return 0;
	}
#endif

	//Check that dir exists and can be written to.
	if ( access( dir, R_OK | W_OK | X_OK ) == -1 ) {
		fprintf( stderr, "Can't modify instance at: %s\n", strerror( errno ) );
		return 0;
	}

	//The root name can be extracted here...
	if ( ( s = archive_read_next_header( a, &entry ) ) == ARCHIVE_OK ) 
		snprintf( rootname, sizeof( rootname ), "%s/", archive_entry_pathname( entry ) );
	else {
		fprintf( stderr, "root get failed\n" );
		return 0;
	}

	//Loop through all entries in the archive
	for ( ; ( s = archive_read_next_header( a, &entry ) ) != ARCHIVE_EOF && s != ARCHIVE_FATAL ; ) {
		int status = 0;
		char fmt[2048] = {0}, *fname = NULL, *pname = NULL;
		pname = (char *)archive_entry_pathname( entry );
		fname = &pname[ strlen( rootname ) - 1 ];
		mode_t mode = archive_entry_filetype( entry );

		status = mode & S_IFMT;
		snprintf( fmt, sizeof(fmt), "%s/%s", dir, fname );
		//fprintf( stdout, "path: %s\n", fmt );

		//If mode == DIR, 
		if ( status != S_IFDIR && status != S_IFREG )
			fprintf( stdout, "%s - not a file or dir\n", pname );	
		else if ( status == S_IFDIR ) {
			//If the directory already exists, don't worry about it
			if ( access( fmt, F_OK ) > -1 ) { 
				continue;
			}

			//...
			if ( mkdir( fmt, archive_entry_perm( entry ) ) == -1 ) {
				fprintf( stderr, "mkdir failed: couldn't create %s\n", fmt );
				return 0;	
			}
		}
		else if ( status == S_IFREG ) {
			int fd = 0;
			if ( ( fd = open( fmt, O_CREAT | O_RDWR | O_TRUNC, 0755 ) ) == -1 ) {
				fprintf( stderr, "create file failed: couldn't create %s\n", fmt );
				return 0;	
			}
			if ( archive_read_data_into_fd( a, fd ) != ARCHIVE_OK ) {
				fprintf( stderr, "populate file failed: couldn't fill up %s\n", fmt );
				return 0;	
			}
			close( fd );
		}
	}

	if ( ( r = archive_read_free(a) ) != ARCHIVE_OK ) {
		fprintf( stderr, "Archive close failed: %s\n", strerror( errno ) );
		return 0;
	}

	return 1;
}



int main ( int argc, char *argv[] ) {
	struct arg arg = { 0 };

	if ( argc < 2 ) {
		fprintf( stderr, PP ":\n%s\n", HELP );
		return 1;
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "-i" ) || !strcmp( *argv, "--install-to" ) )
			arg.instance = *( ++argv );
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--package" ) )
			arg.package = *( ++argv );
		else if ( !strcmp( *argv, "-s" ) || !strcmp( *argv, "--source" ) )
			arg.source = *( ++argv );
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--repository" ) )
			arg.repository = *( ++argv );
		else if ( !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
			fprintf( stderr, "%s\n", HELP );
			return 0;
		}	
		argv++;
	}

#if 0
	//Check for a directory
	if ( !arg.instance ) {
		fprintf( stderr, "No instance specified!\n" );
		return 1;
	}

	//Check for a package
	if ( !arg.package ) {
		fprintf( stderr, "No package specified!\n" );
		return 1;
	}
	//Check the package name to see if it's file or http[s]

	//Depending on location, 
	printf( "package: %s\n", arg.package );
	if ( !unpack_archive( arg.package, arg.instance ) ) {
		fprintf( stderr, "\n" );
		return 1;
	}
#endif
	return 0;
}
