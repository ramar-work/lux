/**
 * lxpkg.c
 * -------
 *
 * Allows installation, creation and maintenance with lux packages.
 *
 * Usage
 * -----
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020-2024 Antonio R. Collins II (ramar@ramar.work)
 *
 * See LICENSE in the top-level directory for more information.
 *
 *
 * CHANGELOG
 * ---------
 *
 *
 * TODO
 * ----
 * - Compress a package (by selectively pulling things?)
 * - Create a new package (by creating a new template)
 * - De-cmopress a pcakage (to a specific location)
 *
 * ------------------------------------------- */
#include "lxcommon.h"
#include <archive.h>
#include <archive_entry.h>
#include <zlib.h>
#include <sys/socket.h>
#include <netdb.h>

#if !defined(DISABLE_TLS)
 #include <gnutls/gnutls.h>
#endif

#if 0
struct gnutls_abstr {
	gnutls_certificate_credentials_t *cbob;
	gnutls_session_t session;
	char sniname[ CTXHTTPS_SNI_LENGTH ]; // The SNI name?
}
#endif

#define NAME "lxpkg"

#define PORT 22

#define HOST "192.168.56.12"

#define HELP \
	"-c, --create             Create a new package.\n" \
	"-i, --install            Install a package.\n" \
	"-d, --directory <arg>    Define where to create the new package.\n" \
	"-n, --name <arg>         Define a name for the package.\n" \
	"-s, --search <arg>       Search for a package titled <arg>.\n" \
	"-t, --to <arg>           Define the full path to an instance (<arg>) \n" \
  "                         for package installation.\n" \
	"-r, --repository <arg>   Define a repository to intialize a search.\n" \
	"-C, --compress           Compress a package.\n" \
	"-o, --output <arg>       Define where to put a compressed package.\n" \
	"-V, --version            Show version information and quit.\n" \
	"-v, --verbose            Tell me everything.\n" \
	"-h, --help               Show the help menu.\n"



int copy_to_instance( const char *, const char *, char *, int );
unsigned char * get_package ( const char *, int *, char *, int );
int unpack_archive ( const unsigned char *, int , const char *, char *, int );
int is_tar_gz ( const char *, char *, int ) ;
int fetch_https_uri ( const char *, int, char *, int ) ;
int fetch_http_uri( const char *, int, char *, int );


struct repository {
	char *type;
	char *address;
};




// Default for packages for new instances
const dir_t pkg_def[] = {
	{ "/README.md", H_FILE, SHAREDIR "package.README.md" },
	{ "/Makefile", H_FILE, SHAREDIR "package.Makefile" },
	{ "/app/", H_DIR, NULL },
	{ "/app/stub.lua", H_FILE, SHAREDIR "app.stub.lua" },
	{ "/assets/", H_DIR, NULL },
	{ "/lib/", H_DIR, NULL },
	{ "/lib/dependencies/", H_DIR, NULL },
	{ "/private/", H_DIR, NULL },
	{ "/private/setup/", H_DIR, NULL },
	{ "/routes/", H_DIR, NULL },
	{ "/tests/", H_DIR, NULL },
	{ "/sql/", H_DIR, NULL },
	{ "/src/", H_DIR, NULL },
	{ "/views/", H_DIR, NULL },
	{ "/views/stub.tpl", H_FILE, SHAREDIR "views.stub.tpl" },
	{ NULL }
};

const char *skip_files[] = {
	".git/", 
	".gitignore",
	"Makefile",
	NULL
};



/**
 * int get_dirsize ( const char *path )
 *
 * Get the size of a directory recursively.
 *
 * I BELIEVE the length of the vpath is added to the total archive
 * size.  I'm adding so we don't just needlessly waste memory.
 *
 */
static unsigned long get_size ( const char *path ) {
	struct stat sb;
	unsigned long size = 0;
	memset( &sb, 0, sizeof( struct stat ) );

	if ( stat( path, &sb ) > -1 ) {
#if 1
		if ( ( sb.st_mode & S_IFMT ) != S_IFDIR )
			size = sb.st_size;
		// If it's a directory, add the size and descend
		else {
#else
		// Assume that directories really need this much space...
		size = sb.st_size;
		if ( ( sb.st_mode & S_IFMT ) == S_IFDIR ) {
#endif
			struct dirent *dn = NULL;
			DIR *dir = NULL;

			// Open the directory and move between the things
			if ( !( dir = opendir( path ) ) ) {
				fprintf( stderr, "Failed to open directory: %s.", strerror( errno ) );
				return 0;
			}

			// Read all files except current and one-level above dir	
			while( ( dn = readdir( dir ) ) ) {
				if ( *dn->d_name != '.' ) {
					char fname[ PATH_MAX ];
					//char vname[ PATH_MAX ];
					memset( fname, 0, PATH_MAX );	
					//memset( vname, 0, PATH_MAX );
					snprintf( fname, PATH_MAX - 1, "%s/%s", path, dn->d_name );
					//snprintf( vname, PATH_MAX - 1, "%s/%s", vpath, dn->d_name );
					//size += strlen( vname );
					size += get_size( fname );
				}
			}

			closedir( dir );
		}	
	}

	fprintf( stdout, "Size of %s is %ld\n", path, sb.st_size );
	return size;
}



/**
 * int compress_rpath( struct archive *a, const char *path, const char *tpath, char *err, int errlen )
 *
 * Recursively compress all files in a directory.
 *
 * NOTE: We use tpath to always make sure that we compress no more than 1 level deep on a package directory.
 *
 * What do I mean?
 * With a path: a/b/c/d/app, we NEVER should see a, b or c.  But rather
 * pack or unpack d/ and all its children.
 *
 */
static int compress_rpath( struct archive *a, const char *path, const char *vpath, char *err, int errlen ) {

	// Defines
	struct stat st;
	struct archive_entry *entry = NULL;
	unsigned char *src = NULL;
	memset( &st, 0, sizeof( struct stat ) );

	// Get the file metadata (catching errors MIGHT be important...)
// We only want the basename to a point...
char newpath[ PATH_MAX ] = {0};
fprintf( stderr, "REAL: %s\n", path );
fprintf( stderr, "VIRTUAL: %s\n", vpath );

	if ( stat( path, &st ) > -1 ) {

#if 1
		// Allocate a new entry for whatever this is...
		if ( !( entry = archive_entry_new() ) ) {
			snprintf( err, errlen, "Failed to open archive: %s.", archive_error_string( a ) );
			return 0;
		}
	
		// Copy details, set a pathname	and write header info
		archive_entry_copy_stat( entry, &st );
		archive_entry_set_pathname( entry, vpath );
		if ( archive_write_header( a, entry ) != ARCHIVE_OK ) {
			snprintf( err, errlen, "Archive header write failure: %s.", archive_error_string( a ) );
			return 0;
		}

		// If it's something else, don't even try
		if ( ( st.st_mode & S_IFMT ) != S_IFDIR && ( st.st_mode & S_IFMT ) != S_IFREG ) {
			;
		}

		// If it's a file, just add the file and it's contents.
		else if ( ( st.st_mode & S_IFMT ) == S_IFREG ) {
			fprintf( stdout, "Compressing: %s\n", path );

			int sz = (int)st.st_size;
		
			// Read the file to memory.  If this fails, you really should return immediately...
			if ( !( src = read_file( path, &sz, err, sizeof( err ) ) ) ) {
				return 0;
			}

//fprintf( stderr, "size of thing: %d\n", sz );write( 2, src, sz ); getchar();

			// Try to write to memory in one blow...
			// TODO: Will archive_write_data always write some large thing?
			if ( archive_write_data( a, src, sz ) < 0 ) {
				snprintf( err, errlen, "Failed to write data: %s.", archive_error_string( a ) );
				return 0;
			}

			free( src );
		}
#endif
		// If it's a directory, add the name and run the code again
		else {
			DIR *dir = NULL;
			struct dirent *dn = NULL;
			char npath[ PATH_MAX ];
			char nvpath[ PATH_MAX ];
			memset( &npath, 0, sizeof( npath ) );
			memset( &nvpath, 0, sizeof( nvpath ) );

			// Open the directory
			if ( !( dir = opendir( path ) ) ) {
				snprintf( err, errlen, "Failed to open directory: %s: %s", path, strerror( errno ) );
				fprintf( stderr, "Some kind of error occurred: %s \n", err);
				return 0;
			}

			// Read all the directories
			while( ( dn = readdir( dir ) ) ) {
				// TODO: We're just skipping hidden files for now.  9/10 times this is what we want
				if ( *dn->d_name != '.' ) {
					snprintf( npath, sizeof( npath ) - 1, "%s/%s", path, dn->d_name );
					snprintf( nvpath, sizeof( nvpath ) - 1, "%s/%s", vpath, dn->d_name );

					fprintf( stdout, "Recursively compressing: %s \n", npath );
					fprintf( stdout, "Renaming to: %s \n", nvpath );

					if ( !compress_rpath( a, npath, nvpath, err, errlen ) ) {
						fprintf( stderr, "Some kind of error occurred: %s \n", err);
						return 0;
					}
				}
			}

			closedir( dir );
		}

		// Destroy the entry
		archive_entry_free( entry );
	}

	return 1;
}



/**
 * unsigned char * pack_archive ( const char *, unsigned long , unsigned long *, char *, int )
 *
 * Packs an extension for distribution in memory.
 *
 */
unsigned char * pack_archive ( const char *path, size_t expsize, size_t *nsize, char *err, size_t errlen ) {
	struct archive *a = NULL;
	unsigned char *dest = NULL;
	size_t size = 0;
	fprintf( stdout, "PRE: Total expected = %ld\n", expsize );

	if ( !( a = archive_write_new() ) ) {
		snprintf( err, errlen, "Failed to open archive." );
		return NULL;
	}

	// Automatically write a gzipped tarball
	archive_write_add_filter_gzip( a );
	archive_write_set_format_ustar( a );

	// No need for temporary files, so let's try to make this work with uint8_t *
#if 1
	// If we knew the size ahead of time, then allocating that size ought to be fine...
	if ( !( dest = malloc( expsize ) ) || !memset( dest, 0, expsize ) ) {
		snprintf( err, errlen, "malloc() failed for new archive: %s", strerror( errno ) );
		return NULL;
	}

	// I feel like mmap() might be the only way to go, b/c we don't know the size yet
	if ( archive_write_open_memory( a, dest, expsize, &expsize ) != ARCHIVE_OK ) {
		snprintf( err, errlen, "Failed to open archive: %s.", archive_error_string( a ) );
		return NULL;
	}
#else
	if ( archive_write_open_filename( a, name ) != ARCHIVE_OK ) {
		snprintf( err, errlen, "Failed to open archive: %s.", archive_error_string( a ) );
		return NULL;
	}
#endif
	//fprintf( stdout, "About to enter: %s, Total expected = %ld\n", path, expsize );
	// Take dirname and basename

#if 1
	// Checking for a general value is a good idea..., but recursively hmm
	if ( !compress_rpath( a, path, ".", err, errlen ) ) {
		return NULL;
	}
#endif

	archive_write_free( a );
	fprintf( stdout, "Total used = %ld\n", expsize );
	*nsize = expsize;
	return dest;
}


/**
 * int make_http_request ( const char *p, int port, char *errmsg, int errlen ) 
 *
 * Creates a new HTTP request
 *
 */
int fetch_http_uri ( const char *p, int port, char *errmsg, int errlen ) {

	// Define
	struct addrinfo hints; 
	struct addrinfo *servinfo; 
	struct addrinfo *pp;
	int rv; 
	int sockfd;
	char b[10] = {0}; 
	char s[ INET6_ADDRSTRLEN ]; 
	char ipv4[ INET_ADDRSTRLEN ];

#if 0
	snprintf( b, sizeof(b), "%d", port );	
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Prepare our response structure
	memset( res, 0, sizeof( zhttp_t ) );

#ifdef INCLUDE_TIMEOUT
	//Set up a timer to kill requests that are taking too long.
	//TODO: There is an alternate way to do with with socket(), I think
	struct itimerval timer;
	memset( &timer, 0, sizeof(timer) );
#if 0
	signal( SIGVTALRM, timer_handler );
#else
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = &timer_handler;
	sigaction( SIGVTALRM, &sa, NULL );
#endif
	//Set timeout to 3 seconds (remember: no cleanup takes place...)
	timer.it_interval.tv_sec = 0;	
	timer.it_interval.tv_usec = 0;
	//NOTE: For timers to work, both it_interval and it_value must be filled out...
	timer.it_value.tv_sec = 2;	
	timer.it_value.tv_usec = 0;
	//if we had an interval, we would set that here via it_interval
	if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
		snprintf( errmsg, errlen, "timeout set error: %s\n", strerror( errno ) );
		return 0;
	}
#endif

	//Get the address info of the domain here.
	//TODO: Use Bind or another library for this.  Apparently there is no way to detect timeout w/o using a signal...
	if ( ( rv = getaddrinfo( r->host, b, &hints, &servinfo ) ) != 0 ) {
		snprintf( errmsg, errlen, "%s => %s", gai_strerror( rv ), r->host );
		return 0;
	}

	//Loop and find the right address
	for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
		if ( ( sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol ) ) == -1) {	
			snprintf( errmsg, errlen, "client socket error: %s", strerror( errno ) );
			continue;
		}

		//fprintf(stderr, "%d\n", sockfd);
		if ( connect( sockfd, pp->ai_addr, pp->ai_addrlen) == -1 ) {
			close( sockfd );
			snprintf( errmsg, errlen, "client connect error: %s", strerror( errno ) );
			continue;
		}

		break;
	}

	//If we completely failed to connect, do something.
	if ( pp == NULL ) {
		snprintf( errmsg, errlen, "client: failed to connect" );
		return 1;
	}

	//This is some weird stuff...
#ifdef INCLUDE_TIMEOUT
	struct sigaction da;
	da.sa_handler = SIG_DFL;
	sigaction( SIGVTALRM, &da, NULL );
#endif

	//Get the internet address
	inet_ntop( pp->ai_family,
		get_in_addr((struct sockaddr *)pp->ai_addr), ipv4, sizeof( ipv4 ));
	freeaddrinfo( servinfo );

	unsigned char *msg = NULL, *bmsg = r->msg;
	int sb = 0, rb = 0, mlen = r->mlen;
	int crlf = -1, first = 0;

	//Must account for the data sent.
	for ( int b; ; ) {
		//These should be blocking, so this is probably a legitimate error
		if ( ( b = send( sockfd, bmsg, mlen, 0 )) == -1 ) {
			snprintf( errmsg, errlen,
				"Error sending mesaage to: %s - %s\n", s, strerror(errno) );
			return 0;
		}
	
		if ( ( sb += b ) != mlen ) {
			continue;
		}

		break;
	}

	//WE most likely will receive a very large page... so do that here...	
#if 0
	if ( !( msg = malloc( 16 ) ) || !memset( msg, 0, 16 ) ) {
		return ERR( errmsg, errlen, "%s\n", "Allocation failure." );
	}
#endif

	for ( int blen = 0, chunked = 0;; ) {
		uint8_t xbuf[ 4096 ] = {0};

		if ( ( blen = recv( sockfd, xbuf, sizeof(xbuf), 0 ) ) == -1 ) {
			//SSLPRINTF( "Error receiving mesaage to: %s\n", s );
			break;
		}
		else if ( !blen ) {
			//SSLPRINTF( "%s\n", "No new bytes sent.  Jump out of loop." );
			break;
		}

		if ( !first++ ) {
			res->status = get_status( (char *)xbuf, blen );
			res->clen = get_content_length( (char *)xbuf, blen );
			if ( !get_content_type( r->ctype, (char *)xbuf, blen ) ) {
				return 0;
			}

			chunked = memstrat( xbuf, "Transfer-Encoding: chunked", blen ) > -1;
			if ( chunked ) {
				SSLPRINTF( "%s\n", "Chunked not implemented for HTTP" );
				return ERR( errmsg, errlen, "%s\n", "Chunked not implemented for HTTP" );
			}
			if (( crlf = memstrat( xbuf, "\r\n\r\n", blen ) ) == -1 ) {
				0;//this means that something went wrong...
			}
			//fprintf(stderr, "found content length and eoh: %d, %d\n", r->clen, crlf );
			crlf += 4;
			//write( 2, xbuf, blen );
		}

		//read into a bigger buffer	
		if ( !( res->msg = realloc( res->msg, res->mlen + blen ) ) ) {
			return ERR( errmsg, errlen, "%s\n", "Realloc of destination buffer failed." );
		}

		memcpy( &res->msg[ res->mlen ], xbuf, blen );
		res->mlen += blen;

		if ( !res->clen )
			return ERR( errmsg, errlen, "%s\n", "No length specified, parser error!." );
		else if ( res->clen && ( res->mlen - crlf ) == res->clen ) {
			SSLPRINTF( "Full HTTP message received\n" );
			break;
		}
	}

	//Close the fd
	if ( close( sockfd ) == -1 ) {
		//Report the error regardless
	}
#endif
	return 1;
}


#ifndef DISABLE_TLS
/**
 * int fetch_https_uri ( const char *uri, int port, char *err, int errlen ) 
 *
 * Fetch a package over HTTPS.
 *
 */
int fetch_https_uri ( const char *uri, int port, char *err, int errlen ) {

	//Define
	gnutls_session_t session;
	gnutls_datum_t out;
	gnutls_certificate_credentials_t xcred;
#if 0
	int ret, type, err = 0;
	unsigned int status;

	//Initialize
	memset( &session, 0, sizeof(gnutls_session_t));
	memset( &xcred, 0, sizeof(gnutls_certificate_credentials_t));

	//Do socket connect (but after initial connect, I need the file desc)
	if ( !gnutls_check_version("3.4.6") )
		return ERR( errmsg, errlen, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

	if ( ( err = gnutls_global_init() ) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( ( err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	//Set client certs this way...
	//gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );

	//Initialize gnutls and set things up
	if ( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, r->host, strlen(r->host)) ) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( ( err = gnutls_set_default_priority( session ) ) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));
	
	if ( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) < 0 )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	//Set random handshake details
	gnutls_session_set_verify_cert( session, r->host, 0 );

#if 0
	//socket connect is the shorter way to do this...
#else
	struct addrinfo hints, *servinfo, *pp;
	int rv, sockfd;
	char b[10] = {0}, s[ INET6_ADDRSTRLEN ], ipv4[ INET_ADDRSTRLEN ];
	snprintf( b, sizeof(b), "%d", port );	
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Prepare our response structure
	memset( res, 0, sizeof( zhttp_t ) );

#ifdef INCLUDE_TIMEOUT
	//Set up a timer to kill requests that are taking too long.
	//TODO: There is an alternate way to do with with socket(), I think
	struct itimerval timer;
	memset( &timer, 0, sizeof(timer) );
#if 0
	signal( SIGVTALRM, timer_handler );
#else
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = &timer_handler;
	sigaction( SIGVTALRM, &sa, NULL );
#endif
	//Set timeout to 3 seconds (remember: no cleanup takes place...)
	timer.it_interval.tv_sec = 0;	
	timer.it_interval.tv_usec = 0;
	//NOTE: For timers to work, both it_interval and it_value must be filled out...
	timer.it_value.tv_sec = 2;	
	timer.it_value.tv_usec = 0;
	//if we had an interval, we would set that here via it_interval
	if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
		snprintf( errmsg, errlen, "timeout set error: %s\n", strerror( errno ) );
		return 0;
	}
#endif

	//Get the address info of the domain here.
	//TODO: Use Bind or another library for this.  Apparently there is no way to detect timeout w/o using a signal...
	if ( ( rv = getaddrinfo( r->host, b, &hints, &servinfo ) ) != 0 ) {
		snprintf( errmsg, errlen, "%s => %s", gai_strerror( rv ), r->host );
		return 0;
	}

	//Loop and find the right address
	for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
		if ( ( sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol ) ) == -1) {	
			snprintf( errmsg, errlen, "client socket error: %s", strerror( errno ) );
			continue;
		}

		//fprintf(stderr, "%d\n", sockfd);
		if ( connect( sockfd, pp->ai_addr, pp->ai_addrlen) == -1 ) {
			close( sockfd );
			snprintf( errmsg, errlen, "client connect error: %s", strerror( errno ) );
			continue;
		}

		break;
	}

	//If we completely failed to connect, do something.
	if ( pp == NULL ) {
		snprintf( errmsg, errlen, "client: failed to connect" );
		return 1;
	}

	//This is some weird stuff...
#ifdef INCLUDE_TIMEOUT
	struct sigaction da;
	da.sa_handler = SIG_DFL;
	sigaction( SIGVTALRM, &da, NULL );
#endif

	//Get the internet address
	inet_ntop( pp->ai_family,
		get_in_addr((struct sockaddr *)pp->ai_addr), ipv4, sizeof( ipv4 ));
	freeaddrinfo( servinfo );
#endif

	//Set up GnuTLS to read things
	gnutls_transport_set_int( session, sockfd );
	gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

	//Do the first handshake
	do {
		 RUN( ret = gnutls_handshake( session ) );
	} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

	//Check the status of said handshake
	if ( ret < 0 ) {
		//fprintf( stderr, "ret: %d\n", ret );
		if ( RUN( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) ) {	
			type = gnutls_certificate_type_get( session );
			status = gnutls_session_get_verify_cert_status( session );
			err = gnutls_certificate_verification_status_print( status, type, &out, 0 );
			fprintf( stdout, "cert verify output: %s\n", out.data );
			gnutls_free( out.data );
			//jump to end, but I don't do go to
		}
		return ERR( errmsg, errlen, "Handshake failed: %s\n", gnutls_strerror( ret ));
	}
	else {
		//desc = gnutls_session_get_desc( session );		
		//consider dumping the session info here...
		//fprintf( stdout, "- Session info: %s\n", desc );
		//gnutls_free( desc );
	}

	//TODO: This needs to be a large loop...
	if ( RUN(( err = gnutls_record_send( session, r->msg, r->mlen ) ) < 0 ) ) {
		return ERR( errmsg, errlen, "%s", "GnuTLS 3.4.6 or later is required for this example." );	
	}

	//This is a sloppy quick way to handle EAGAIN
	int crlf = -1, first = 0, chunked = 0;
	#if 0
	if ( !( msg = malloc( 16 ) ) || !memset( msg, 0, 16 ) ) {
		return ERR( r->err, "%s\n", "Allocation failure." );
	}
	#endif

	//there should probably be another condition used...
	for ( ;; ) {	
		char xbuf[ 4096 ] = {0};
		int ret = gnutls_record_recv( session, xbuf, sizeof(xbuf));
		SSLPRINTF( "gnutls_record_recv returned %d\n", ret );

		//receive
		if ( !ret ) {
			SSLPRINTF( "Peer has closed the TLS Connection\n" );
			break;
		}
		else if ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
			SSLPRINTF( "Warning: %s\n", gnutls_strerror( ret ) );
			continue;
		}
		else if ( ret < 0 ) {
			SSLPRINTF( "Error: %s\n", gnutls_strerror( ret ) );
			break;	
		}
		else if ( ret > 0 ) {
			SSLPRINTF( "Recvd %d bytes:\n", ret );
			SSLPRINTF( "AM i HERE?!\n" );
			if ( !first++ ) {

				//Get status, content-length or xfer-encoding if available
				char ctypebuf[ 128 ] = {0};
				res->status = get_status( (char *)xbuf, ret );
				res->clen = get_content_length( xbuf, ret );
				if ( get_content_type( ctypebuf, (char *)xbuf, ret ) ) {
					//I feel like content-type should always be specified...
					//But you can get something smaller...?  or zero-length?
					res->ctype = zhttp_dupstr( ctypebuf );
				}
				chunked = memstrat( xbuf, "Transfer-Encoding: chunked", ret ) > -1;
				if ( ( crlf = memstrat( xbuf, "\r\n\r\n", ret )) == -1 ) {
					SSLPRINTF( "%s\n", "No CRLF sequence found, response malformed." );
					return ERR( errmsg, errlen, "%s\n", "No CRLF sequence found, response malformed." );
				}
				//Increment the crlf by the length of "\r\n\r\n"	
				crlf += 4;
				if ( chunked ) {
					char *m = &xbuf[ crlf ];
					//parse the chunked length
					int lenp = memstrat( m, "\r\n", ret - crlf );	
					//to save chunk length, I need to convert to hex to decimal
					char lenpxbuf[ 10 ] = {0};
					memcpy( lenpxbuf, m, lenp );
					int sz = 0; //atoi( lenpxbuf );
				}
			}
			SSLPRINTF( "Did I get HERE?!\n" );

			//Finalize chunked messages.
			if ( chunked && ret == 5 ) {
				if ( memcmp( xbuf, "0\r\n\r\n", 5 ) == 0 ) {
					break;
				}
				else {
					fprintf(stderr, "not sure what happened.." );
				}
			}

			//Read into a bigger buffer	
			if ( !( res->msg = realloc( res->msg, res->mlen + ( ret + 1 ) ) ) ) {
				SSLPRINTF( "%s\n", "Realloc of destination buffer failed." );
				return ERR( errmsg, errlen, "%s\n", "Realloc of destination buffer failed." );
			}

			memcpy( &res->msg[ res->mlen ], xbuf, ret ), res->mlen += ret;

			//If it's chunked, try sending a 100-continue
			if ( chunked ) {
				const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
				//int err = gnutls_record_send( session, cont, strlen(cont));
				fprintf(stderr, "%s (%d)\n", gnutls_strerror( err ), err );
			}
			else {
				if ( !res->clen ) {
					SSLPRINTF( "%s\n", "No length specified, parser error!." );
					return ERR( errmsg, errlen, "%s\n", "No length specified, parser error!." );
				}
				else if ( res->clen && ( res->mlen - crlf ) == res->clen ) {
					SSLPRINTF( "Full message received: clen: %d, mlen: %d", res->clen, res->mlen );
					break;
				}
				SSLPRINTF( "recvd: %d , clen: %d , mlen: %d\n", res->mlen - crlf, res->clen, res->mlen );
			}
		}
	} /* end while */

	//TODO: Unsure why this is here or what it really does...
	int tries=0;
	while ( tries++ < 3 && ( err = gnutls_bye(session, GNUTLS_SHUT_WR) ) != GNUTLS_E_SUCCESS ) ;
	if ( err != GNUTLS_E_SUCCESS ) {
		return ERR( errmsg, errlen, "%s\n",  gnutls_strerror( ret ) );
	}

	//Close the file and free all of GnuTLS's structures
	if ( close( sockfd ) == -1 ) {

	}
	gnutls_deinit( session );
#endif
	return 1;
}
#endif


/**
 * unsigned char * get_package ( const char *path )
 *
 * Get package content from 'file' or 'http'.
 *
 */
unsigned char * get_package ( const char *path, int *len, char *err, int errlen ) {

	// Source
	unsigned char *src = NULL;
	int srclen = 0;

	// Fetch www repository (we'll allow http only, but...)
	if ( STRLCMP( path, "http://" ) ) {
		// Make a request

		// Get the package
	}

	else if ( STRLCMP( path, "https://" ) ) {

		// Make a request
	
	}

	// Handle regular files
	else {
		//
		struct stat sb;
		memset( &sb, 0, sizeof( struct stat ) );

		// Check for the file prefix if available
		if ( STRLCMP( path, "file://" ) ) {
			path += 7;
		}

		// Check if it exists
		if ( stat( path, &sb ) == -1 ) {
			snprintf( err, errlen, "File not found: %s", strerror( errno ) );
			return NULL;
		} 

		#if 0
		// TODO: Add an access check.  Quickly throw permission errors if need be.
		#endif

		// Check if it's a directory
		if ( ( sb.st_mode & S_IFMT ) == S_IFDIR ) {
			snprintf( err, errlen, "Requested package '%s' is a directory.", pbasename(path) );
			return NULL;
		}

		// If it's zero length, it's always a problem
		if ( !sb.st_size /* TODO: There's probably a minimum length */ ) {
			snprintf( err, errlen, "File at path '%s' is zero-length", path );
			return NULL;
		}

		// Finally, check if it's an archive
		// TODO: We need to read the header and make sure that this is indeed the case
		if ( !is_tar_gz( path, err, errlen ) ) {
			snprintf( err, errlen, "File at path '%s' is not a gzip-compressed tar archive", path );
			return NULL;
		}

		// Fetch from system
		if ( !( src = read_file( path, &srclen, err, errlen ) ) ) {
			//snprintf( err, errlen, "Read file failed: %s", err );
			return NULL;
		}

		// Set new length
		*len = srclen;
	}

	// Return the pointer
	return src;
}



/**
 * int unpack_archive ( const unsigned char *, unsigned int, const char *, char *, int )
 *
 * Unpacks a tarball at path pointed to by [dir].
 *
 */
int unpack_archive ( const unsigned char *c, int clen, const char *dir, char *err, int errlen ) {

	// Define
	char rootname[ 1024 ] = {0};
	const char *extra[] = { "misc", "misc/docs", NULL };
	int s, r;
	struct archive *a = archive_read_new();
	struct archive_entry *entry = NULL;

	// Suppose these stop
	archive_read_support_filter_all(a);
	archive_read_support_format_all(a);

#if 1
	if ( ( r = archive_read_open_memory( a, c, clen ) ) != ARCHIVE_OK ) {
		fprintf( stderr, "Archive read failed: %s\n", strerror( errno ) );
		return 0;
	}
#else
	if ( ( r = archive_read_open_filename( a, name, 4096 ) ) != ARCHIVE_OK ) {
		fprintf( stderr, "Archive read failed: %s\n", strerror( errno ) );
		return 0;
	}

	// Create a new folder?
	if ( mkdir( dir, 0755 ) == -1 ) {
		fprintf( stderr, "mkdir failed\n" );
		return 0;
	}
#endif

	// Check that dir exists and can be written to.
	if ( access( dir, R_OK | W_OK | X_OK ) == -1 ) {
		fprintf( stderr, "Can't modify instance at: %s\n", strerror( errno ) );
		return 0;
	}

	// The root name can be extracted here...
	if ( ( s = archive_read_next_header( a, &entry ) ) == ARCHIVE_OK )
		snprintf( rootname, sizeof( rootname ), "%s/", archive_entry_pathname( entry ) );
	else {
		fprintf( stderr, "root get failed\n" );
		return 0;
	}

	// If we're in an instance, we should probably make a misc/docs/ folder
	for ( const char **x = extra; *x; x++ ) {

		// Define
		char buf[ PATH_MAX ] = {0};
		struct stat sb = {0};
		snprintf( buf, sizeof( buf ) - 1, "%s/%s", dir, *x );
		//FPRINTF( "new dir? %s\n", buf );//, *x, dir );

		// Check if it exists at all		
		if ( stat( buf, &sb ) == -1 && errno != ENOENT ) {
			FPRINTF( "extra dir check for %s failed\n", *x );
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}

		// File exists and is a directory
		if ( ( sb.st_mode & S_IFMT ) == S_IFDIR ) {
			FPRINTF( "File exists and is a dir.\n" );
			continue;
		}
 
		// File most likely exists 
		if ( mkdir( buf, 0744 ) == -1 ) {
			FPRINTF( "extra dir mkdir failed for %s: %s\n", *x, strerror( errno ) );
			snprintf( err, errlen, "%s", strerror( errno ) );
			return 0;
		}
	}

	// Loop through all entries in the archive
	for ( ; ( s = archive_read_next_header( a, &entry ) ) != ARCHIVE_EOF && s != ARCHIVE_FATAL ; ) {

		// Define
		int status = 0;
		char fmt[2048] = {0}; 
		char *fname = NULL; 
		char *pname = NULL;

		// Initialize
		pname = (char *)archive_entry_pathname( entry );
		fname = &pname[ strlen( rootname ) - 1 ];
		mode_t mode = archive_entry_filetype( entry );
		status = mode & S_IFMT;

		// Certain files go in certain places
		//FPRINTF( "dir = %s, fname = %s, pname = %s\n", dir, fname, pname );

		// Skip files with a particular prefix
		for ( const char **f = skip_files; *f; f++ ) {
			if ( memstrat( fname, *f, strlen(fname) ) == 0 ){
				FPRINTF( "found %s in filename!\n", *f );
				fprintf( stderr, "skipping path: %s\n", fname );
				continue;
			}
		}

		// Documentation (README.md and friends) should be handled differently
		// All other files can so far be copied, but we can still run into conflicts
		if ( strcmp( fname, "README.md" ) == 0 || strcmp( fname, "README.html" ) == 0 )
			snprintf( fmt, sizeof(fmt), "%s/misc/docs/%s", dir, fname );
		else {
			snprintf( fmt, sizeof(fmt), "%s/%s", dir, fname );
		}

		#if 0
		// TODO: The last problem here is namespacing.  But should this be a convention at all?
		#endif

		// If mode == DIR,
		if ( status != S_IFDIR && status != S_IFREG )
			fprintf( stdout, "%s - not a file or dir\n", pname );	
		else if ( status == S_IFDIR ) {
			// If the directory already exists, don't worry about it
			if ( access( fmt, F_OK ) > -1 ) {
				continue;
			}

			// ...
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

	FPRINTF( "ARCHIVE UNPACK DONE!\n" );
	return 1;
}




/**
 * int is_tar_gz ( const char *pkgname, char *err, int errlen ) 
 *
 * Is the file referenced a tar archive?
 *
 */
int is_tar_gz ( const char *pkgname, char *err, int errlen ) {

	if ( !strstr( pkgname, ".tar.gz" ) && !strstr( pkgname, ".tgz" ) ) {
		return 0;
	}

	return 1;
}


/**
 * int copy_to_instance( config_t *ua, char *err, int errlen )
 *
 * Copy to an instance from some place.
 *
 */
int copy_to_instance( const char *instdir, const char *pkgname, char *err, int errlen ) {
	// Define
	unsigned char *pkg = NULL;
	int pkglen = 0;

	// Package and instance need to be looked at
	if ( !( pkg = get_package( pkgname, &pkglen, err, errlen ) ) ) {
		return 0;	
	}

#if 1
	// Unpack to the directory
	if ( !unpack_archive( pkg, pkglen, instdir, err, errlen ) ) {
		return 0;	
	}	
#endif

	// Destroy
	free( pkg );	
	return 1;
}



/**
 * typedef struct config_t
 *
 * List of user arguments for `lxpkg`
 *
 */
typedef struct config_t {
#if 1
	/* Install a package to some instance */
	int install;

	/* Create a new package */
	int create;

	/* Compress an existing package? */
	int compress;

	/* Define a package name */
	char *package;

	/* Define an instance name */
	char *instance;

	/* Define an alternate repository to pull a package from */
	char *repository;
#endif

	/* The source directory of the new package */
	char *srcdir;

	/* Where to place the compressed files that comprise a package directory */
	char *comppath;

	/* Be verbose when doing things */
	int verbose;

} config_t;



/**
 * void print_config ( config_t *)
 *
 * Dump all in config_t.
 *
 */
void print_config ( config_t *ua ) {
	fprintf( stderr, "Create:      %d\n", ua->create );
	fprintf( stderr, "Install:     %d\n", ua->install );
	fprintf( stderr, "Package:     %s\n", ua->package );
	fprintf( stderr, "Instance:    %s\n", ua->instance );
	fprintf( stderr, "Repository:  %s\n", ua->repository );
	fprintf( stderr, "Source Dir:  %s\n", ua->srcdir );
	fprintf( stderr, "Compress path:  %s\n", ua->comppath );
	return;
}



int main ( int argc, char *argv[] ) {

	// Define some basics
	char err[ 1024 ] = { 0 };

	if ( argc < 2 ) {
		fprintf( stderr, NAME ":\n%s\n", HELP );
		return 1;
	}

	// Set up some sensible defaults
	// TODO: Make me global
	config_t ua = {
		.create = 0,
		.install = 0,
		.package = NULL,
		.instance = NULL,
		.repository = NULL,
		.verbose = 0,
		.srcdir = NULL,
	};

	// Skip the first argument...
	for ( argv++; *argv; ) {
	
		// We got a non argument option	
		if ( *( *argv ) != '-' )
			return EXITPRINTF( 1, NAME ": Got unknown argument %s!\n", *argv );
		else if ( EVALARG( *argv, "-v", "--verbose" ) )
			ua.verbose = 1;
		else if ( EVALARG( *argv, "-c", "--create" ) )
			ua.create = 1;
		// Compress a package and output to stdout or file
		else if ( EVALARG( *argv, "-t", "--to" ) && !SAVEARG( argv, ua.instance ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --to." );	
		// Specify a source directory for compression
		else if ( EVALARG( *argv, "-d", "--directory" ) && !SAVEARG( argv, ua.srcdir ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --directory." );	
		else if ( EVALARG( *argv, "-r", "--repository" ) && !SAVEARG( argv, ua.repository ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --repository." );	
		else if ( EVALARG( *argv, "-h", "--help" ) )
			return EXITPRINTF( 0, "%s\n", HELP );
		else if ( EVALARG( *argv, "-C", "--compress" ) && !SAVEARG( argv, ua.comppath ) ) {
			return EXITPRINTF( 1, "%s\n", "Argument required for --compress." );	
		}
		else if ( EVALARG( *argv, "-i", "--install" ) ) {
			if ( !SAVEARG( argv, ua.package ) ) {
				return EXITPRINTF( 1, "%s\n", "Argument required for --install." );	
			}
			ua.install = 1;
		}
#if 0
		// An alternate way of defining a package 
		else if ( EVALARG( *argv, "-p", "--package" ) && !SAVEARG( argv, ua.package ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --package." );	
		// Attempt to extract a package from a running instance
		else if ( EVALARG( *argv, "-E", "--extract" ) && !SAVEARG( argv, ua.extract ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --extract." );	
#endif
		else if ( EVALARG( *argv, "-V", "--version" ) ) {
			fprintf( stdout, "%s\n", PACKAGE_VERSION );
			return 0;
		}
#if 1
		else if ( !memcmp( *argv, "-", 1  ) ) {
			return EXITPRINTF( 1, NAME ": Got unknown argument %s!\n", *argv );
		}
#endif
		argv++;
	}

	// 
	print_config( &ua );

	// 
	if ( !ua.create && !ua.install ) {
		fprintf( stderr, NAME ": No actions specified.\n" );
		return 1;
	}

	// If the user specifies a directory, use the basename as package name
	if ( ua.srcdir && !ua.package ) {
		ua.package = pbasename( ua.srcdir );
	}

	// Create stuff
	if ( ua.create ) {

		// Check for the instance or the package?
		if ( !ua.package ) {
			fprintf( stderr, NAME ": No package specified\n" );
			return 1;
		}

		// Replacements
		kset_t reps[] = {
			{ "pkgname", &ua.package, 2 },
			{ NULL }
		};

		if ( !create_dirs( ua.srcdir, (dir_t *)pkg_def, reps, err, sizeof( err ) ) ) {
			fprintf( stderr, NAME ": Creating package '%s' failed: %s\n", ua.package, err );
			return 1;
		}

		// Stop early
		return 0;
	}

	// Copy a package from one site to another	
	if ( ua.install ) {

		// Define	
		int pkglen = 0;
		struct stat sb = {0};
		unsigned char *pkg = NULL;

		// Check for the instance or the package?
		if ( !ua.package ) {
			fprintf( stderr, NAME ": No package specified\n" );
			return 1;
		}

		// Some quick debugging info
		if ( 1 ) {
			FPRINTF( "basename %s\n", pbasename( ua.package ) );
			FPRINTF( "package %s\n", ua.package );
			FPRINTF( "instance %s\n", ua.instance );
		}

	#if 1
		// Copy it to instance	
		if ( !copy_to_instance( ua.instance, ua.package, err, sizeof( err ) ) ) {
			fprintf( stderr, NAME ": Installing package <pkg-name> to <dir> failed: %s\n", err );
			return 1;
		}
	#else
		unsigned char *pkg = NULL;
		int pkglen = 0;

		// Package and instance need to be looked at
		if ( !( pkg = get_package( pkgname, &pkglen, err, errlen ) ) ) {
			return 0;	
		}

	#if 1
		// Unpack to the directory
		if ( !unpack_archive( pkg, pkglen, instdir, err, errlen ) ) {
			return 0;	
		}	
	#endif

		// Destroy
		free( pkg );	
	#endif
		// Stop early
		return 0;
	}


#if 0
	// Do stuff with source
	else if ( ua.srcdir ) {
		FPRINTF( "source directory: %s\n", ua.srcdir );

		// Pack and unpack should go to memory whenever possible
		struct stat sd;
		memset( &sd, 0, sizeof( struct stat ) );

		// Check that the srcdir exists
		if ( stat( ua.srcdir, &sd )	== -1 ) {
			return EXITPRINTF( 1, "Error occurred: %s\n", strerror( errno ) );
		}

		// Also check that it is an actual directory
		if ( ( sd.st_mode & S_IFMT ) != S_IFDIR ) {
			return EXITPRINTF( 1, "Path specified via --source %s is not a directory\n", ua.srcdir );
		}

		// Handle compression and writing somewhere...
		if ( ua.comppath ) {
			unsigned char *archive = NULL;
			//unsigned long asize = 0;
			size_t asize = 0;
			size_t dirsize = 0;
			const char *filename = NULL;
			int fd = 1;

			// Get the size
			dirsize = get_size( ua.srcdir );
			dirsize *= 4;
			//dirsize += 200;
			FPRINTF( "dirsize = %ld\n", dirsize );

			// Compress it
			if ( !( archive = pack_archive( ua.srcdir, dirsize, &asize, err, sizeof( err ) ) ) || !asize ) {
				fprintf( stderr, NAME ": package compression failed: %s\n", err );
				return 1;
			}

#if 1
			// Open the expected file (should just overwrite, unless you use --no-clobber...)
			if ( ( *ua.comppath != '-' || strlen( ua.comppath ) > 1 ) && ( fd = open( ua.comppath, O_CREAT | O_WRONLY | O_TRUNC, 0644 ) ) == -1 ) {
				fprintf( stderr, NAME ": opening file '%s' failed: %s\n", ua.comppath, strerror( errno ) );
				return 1;
			}

			// Write
			if ( write( fd, archive, asize ) == -1 ) {
				fprintf( stderr, NAME ": writing file '%s' failed: %s\n", ua.comppath, strerror( errno ) );
				return 1;
			}
#endif
			// Depending on location, output the package
			fprintf( stdout, "Total size of %s = %ld bytes\n", ua.srcdir, dirsize );
			free( archive );
		}
	}
#endif

	return 0;
}



