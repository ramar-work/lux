/**
 * lxcommon.c
 * -------
 * Utilities shared across command line tooling.
 *
 *
 * LICENSE
 * -------
 * Copyright 2020-2024 Antonio R. Collins II (ramar@ramar.work)
 *
 * See LICENSE in the top-level directory for more information.
 *
 */
#include "lxcommon.h"

struct kv point[ 64 ] = { 0 };



/**
 * char *pbasename ( char *path )
 *
 * Returns the basename of a path.
 *
 */
char *pbasename ( const char *path ) {
	int len = strlen( path );
	char *p = (char *)path + len;
	for ( ; len > 0 && *p != '/'; p--, --len );
	return ( *p == '/' ) ? ++p : p;
}


/**
 * static struct kv * replace ( unsigned char *f, kset_t *kset )
 *
 * Returns an struct kv * with key and value replacements.
 *
 */
struct kv * replace ( unsigned char *f, kset_t *kset ) {
	unsigned char *f1 = f;
	int size = 0;
	struct kv *p = point;
	memset( p, 0, sizeof( point ) /	sizeof( struct kv ) );

	// This is the default character being used for any replacements
	const char ch = '@';

	for ( int inner = 0; *f; f++ ) {
		if ( *f == ch ) { //&& ( *f = '"' ) ) {
		
			if ( !inner ) {
				p->size = size;
				p->value = f1;
			}
			else {
				f1++;
				for ( kset_t *k = kset; k->key; k++ ) {
					// TODO: Be careful here...
					if ( !memcmp( f1, k->key, k->len ) && *k->ptr ) {
						p->size = strlen( *k->ptr );
						p->value = (unsigned char *)*k->ptr;
						f++;
					}
				}
			}

			inner = !inner;
			f1 = f;
			size = 0;
			p++;
		}
		size++;
	}

	p->size = size, p->value = f1, ++p, p->size = -1, p->value = NULL;
	return point;
}



/**
 * int create_dirs( const char *refdir, dir_t *def, kset_t *rep, char *err, int errlen )
 *
 * Creates a new set of directories and files.  Also performs find and
 * replace on files needing string replacements.
 *
 */
int create_dirs( const char *refdir, dir_t *def, kset_t *reps, char *err, int errlen ) {

	// Define 
	char dir[ PATH_MAX ];
	int dp = 0;
	struct stat sb = { 0 };
	zw_t x;

	// Initialize all structures that need it.
	memset( &x, 0, sizeof( zw_t ) );
	memset( &dir, 0, sizeof( dir ) );
	memset( &sb, 0, sizeof( struct stat ) );	

	// Make each parent directory as long as they don't exist
	//while ( strwalk( &x, refdir, "/" ) ) fprintf(stderr,"dir: %s\n", x.src );
	while ( strwalk( &x, refdir, "/" ) ) {

		// Write a real string
		if ( x.size - 1 ) {

			// Do some low-level string buffer copy madness
			memcpy( &dir[ dp ], &refdir[ dp ], x.size ), dp += x.size;
			//write(2,&refdir[dp],x.size), write(2,"\n",1);

			// Check for existence or create the current child directory
			if ( access( dir, F_OK ) == -1 && mkdir( dir, 0755 ) == -1 ) {
				snprintf( err, errlen, "Failed to create directory '%s': %s", refdir, strerror( errno ) );
				return 0;
			}

		}
	}

	// Create all the directories
	for ( const dir_t *layout = def; layout->name; layout++ ) {
		char rname[ 2048 ] = { 0 };
		snprintf( rname, sizeof( rname ) - 1, "%s%s", refdir, layout->name );
		//FPRINTF( "resource name: %s\n", rname );

		if ( layout->type == H_DIR ) {	
			FPRINTF( "-- Creating directory: %s\n", rname );
			if ( mkdir( rname, 0755 ) == -1 ) {
				snprintf( err, errlen, "Directory creation failure at %s: %s", rname, strerror( errno ));
				return 0;
			}
		}
		else {

			int fd = 0, len = 0;
			unsigned char *content = NULL;
			// TODO: Work on my dereferencing, there's no reason I should need this
			char lerr[ 1024 ] = { 0 };

			//fprintf( stderr, "PATH: %s\n", layout->path );
			// TODO: The file size and length SHOULD always be able to load, but...
			FPRINTF( "Creating file: %s\n", rname );
			if ( !( content = read_file( layout->path, &len, lerr, sizeof( lerr ) ) ) ) {
				memcpy( err, lerr, strlen( lerr ) );
				return 0;
			}

			// Handle file open failure
			if ( ( fd = open( rname, O_CREAT | O_RDWR | O_TRUNC, 0644 ) ) == -1 ) {
				snprintf( err, errlen, "Failed to open file at %s: %s", rname, strerror( errno ));
				return 0;
			}

			// Handle file write failure
			if ( layout->type == H_BINFILE && write( fd, content, len ) == -1 ) {
				snprintf( err, errlen, "Failed to write to file at %s: %s", rname, strerror( errno ));
				return 0;
			}

			// Do all the replacements in template files
			else if ( reps ) {
				for ( struct kv *c = replace( content, reps ); c->size > -1; c++ ) {
					FPRINTF( "kv_t = %p\n", c );
					write( fd, c->value, c->size );
				}
			}

			// Write a file
			else {
				write( fd, content, len );
			}

			if ( close( fd ) == -1 ) {
				snprintf( err, errlen, "Could not close file at %s: %s", rname, strerror( errno ));
				return 0;
			}
	
			free( content );
		}
	}	
	return 1;
}


#if 0
/**
 * int connect_via_ssh( const char *, char *, int )
 *
 * Connects to a server with SSH protocol.
 *
 */
int connect_via_ssh( const config_t *ua, const conn_t *conn, char *err, int errlen ) {
	ssh_session mss = NULL;
	int verbosity = SSH_LOG_PROTOCOL;

	if ( !( mss = ssh_new() ) ) {
		fprintf( stderr, "Error initializing session: %s\n", ssh_get_error( mss ) );
		return 0;	
	}

	ssh_options_set( mss, SSH_OPTIONS_HOST, conn->hostname );
	ssh_options_set( mss, SSH_OPTIONS_PORT, &conn->port );

	if ( ua->verbose )
		ssh_options_set( mss, SSH_OPTIONS_LOG_VERBOSITY, &verbosity );

	if ( ssh_connect( mss ) != SSH_OK ) {
		fprintf( stderr, "Error connecting to %s: %s\n", conn->hostname, ssh_get_error( mss ) );
		return 0;	
	}

	ssh_free( mss );
	return 1;
}



/**
 * int scp_write( ssh_session, const char *, char *, int )
 *
 * Write a file to a server via SSH.
 *
 */
int scp_write( ssh_session session, const char *pkg, char *err, int errlen ) {

	ssh_scp scp;
	int len = 0;
	struct stat sb;

	// Initialize stuff
	memset( &sb, 0, sizeof( struct stat ) );
	
	// Get file metadata
	if ( stat( pkg, &sb ) == -1 ) {
		snprintf( err, errlen, "Error getting file info: %s", strerror( errno ) );
		return 0;
	}

	//
	if ( !( scp = ssh_scp_new( session, SSH_SCP_WRITE, "." ) ) ) {
		snprintf( err, errlen, "Error allocating scp session: %s", ssh_get_error( session ) );
		return 0;
	}

	if ( ssh_scp_init( scp ) != SSH_OK ) {
		snprintf( err, errlen, "Error initializing scp session: %s", ssh_get_error( session ) );
		return 0;
	}

	//
	if ( ssh_scp_push_file( scp, pkg, len, S_IRUSR | S_IWUSR ) != SSH_OK ) {
		snprintf( err, errlen, "Error opening remote file: %s", ssh_get_error( session ) );
		return 0;
	}

	if ( ssh_scp_write( scp, pkg, sb.st_size ) != SSH_OK ) {
		snprintf( err, errlen, "Error writing to remote file: %s", ssh_get_error( session ) );
		return 0;
	}


	ssh_scp_close( scp );
	ssh_scp_free( scp );
	return 1;
}



/**
 * int scp_read ( ssh_session, const char *, char *, int )
 *
 * Read a package from an SSH-enabled server.
 *
 */
int scp_read ( ssh_session session, const char *pkg, char *err, int errlen ) {

	ssh_scp scp;
	
	if ( !( scp = ssh_scp_new( session, SSH_SCP_READ, pkg ) ) ) {
		snprintf( err, errlen, "Error allocating scp session: %s", ssh_get_error( session ) );
		return 0;
	}

	if ( ssh_scp_init( scp ) != SSH_OK ) {
		snprintf( err, errlen, "Error initializing scp session: %s", ssh_get_error( session ) );
		return 0;
	}


	ssh_scp_close( scp );
	ssh_scp_free( scp );
	return 1;
}
#endif


