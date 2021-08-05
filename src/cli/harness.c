/* ------------------------------------------- * 
 * harness.c
 * =========
 *
 * Summary 
 * -------
 * Command line tooling to test hypno sites.
 *
 * Usage
 * -----
 * -
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * See LICENSE in the top-level directory for more information.
 *
 * Changelog
 * ---------
 * 
 * ------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <dlfcn.h>
#include <errno.h>
#include <zwalker.h>
#include <zhttp.h>
#include "../util.h"
#include "../server.h"
#include "../filters/filter-static.h"
#include "../filters/filter-echo.h"
#include "../filters/filter-dirent.h"
#include "../filters/filter-redirect.h"
#include "../filters/filter-lua.h"

#define PP "hypno-harness"

#define FSYMBOL "filter"

#define NSYMBOL "libname"

#define HELP \
	"-f, --filter <arg>       Specify a filter for testing (required).\n" \
	"-l, --library <arg>      Specify path to library.\n" \
	"-d, --directory <arg>    Specify path to web app directory (required).\n" \
	"-u, --uri <arg>          Specify a URI (required).\n" \
	"-c, --content-type <arg> Specify a content-type for testing.\n" \
	"-n, --host <arg>         Specify a hostname for use w/ the request.\n" \
	"-a, --alias <arg>        Specify an alternate hostname for use w/ the request.\n" \
	"-m, --method <arg>       Specify an HTTP method to be used when making\n" \
	"                         a request. (GET is default)\n" \
	"-p, --protocol <arg>     Specify alternate protocols (HTTP/1.0, 2.0, etc)\n" \
	"-F, --form <arg>         Specify a body to use when making requests.\n" \
	"-b, --binary <arg>       Specify a body to use when making requests. (assumes multipart)\n" \
	"                         (Use multiple invocations for additional arguments)\n" \
	"-e, --header <arg>       Specify a header to use when making requests.\n" \
	"                         (Use multiple invocations for additional arguments)\n" \
	"-M, --multipart          Use a multipart request when using POST or PUT\n" \
	"-S, --msg-only           Show only the message, no header info\n" \
	"-B, --body-to <arg>      Output body to file at <arg>\n" \
	"-H, --headers-to <arg>   Output headers to file at <arg>\n" \
	"-v, --verbose            Be wordy.\n" \
	"-h, --help               Show help and quit.\n"

//Define a list of filters
struct filter filters[16] = { 
	{ "static", filter_static }
,	{ "echo", filter_echo }
,	{ "dirent", filter_dirent }
,	{ "redirect", filter_redirect }
,	{ "lua", filter_lua }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
, { NULL }
};

struct arg {
	char *lib;
	char *path;
	char *ctype;
	char *host;
	char *alias;
	char *method;
	char *protocol;
	char *uri;
	char *filter;
	int verbose;
	int randomize;
	int multipart;
	int blen;
	int hlen;
	int msgonly;
	char *headerf;
	char *bodyf;
	int dump;
	char **headers;
	char **body;
};


void dump_args( struct arg *arg ) {
	fprintf( stderr, "lib:       %s\n", arg->lib );
	fprintf( stderr, "path:      %s\n", arg->path );
	fprintf( stderr, "ctype:     %s\n", arg->ctype );
	fprintf( stderr, "host:      %s\n", arg->host );
	fprintf( stderr, "alias:     %s\n", arg->alias );
	fprintf( stderr, "method:    %s\n", arg->method );
	fprintf( stderr, "protocol:  %s\n", arg->protocol );
	fprintf( stderr, "uri:       %s\n", arg->uri );
	fprintf( stderr, "headers:   %p\n", arg->headers );
	fprintf( stderr, "body:      %p\n", arg->body );

	fprintf( stderr, "verbose:   %s\n", arg->verbose ? "Y" : "N" );
	fprintf( stderr, "randomize: %s\n", arg->randomize ? "Y" : "N" ); 
	fprintf( stderr, "multipart: %s\n", arg->multipart ? "Y" : "N" );
	fprintf( stderr, "msg-only:  %s\n", arg->msgonly ? "Y" : "N" );
	fprintf( stderr, "blen:      %d\n", arg->blen );
	fprintf( stderr, "hlen:      %d\n", arg->hlen );
}


int method_expects_body ( char *mstr ) {
	const char *mth[] = { 
		"POST", "PUT", "DELETE"
	};

	for ( int i=0; i<sizeof(mth)/sizeof(const char *); i++ ) {
		if ( !strcasecmp( mth[i], mstr ) ) return 1;
	}

	return 0;	
}



int method_is_valid ( char *mstr ) {
	const char *mth[] = { 
		"HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS", "TRACE" 
	};

	for ( int i=0; i<sizeof(mth)/sizeof(const char *); i++ ) {
		if ( !strcasecmp( mth[i], mstr ) ) return 1;
	}

	return 0;
}



int main ( int argc, char * argv[] ) {
	//Make this
	struct arg arg = {0};
	int blen = 0;
	void *app = NULL;
	const int (*filter)( int, struct HTTPBody *, struct HTTPBody *, struct cdata * );
	struct HTTPBody req = {0}, res = {0};
	char *fname = NULL, err[ 2048 ] = { 0 };
	struct cdata conn;
	struct lconfig sconf;
	int header_fd=1, body_fd=1;

	if ( argc < 2 ) {
		fprintf( stderr, PP ":\n%s\n", HELP );
		return 1;
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "-l" ) || !strcmp( *argv, "--library" ) )
			arg.lib = *( ++argv );
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--directory" ) )
			arg.path = *( ++argv );
		else if ( !strcmp( *argv, "-f" ) || !strcmp( *argv, "--filter" ) )
			arg.filter = *( ++argv );
		else if ( !strcmp( *argv, "-u" ) || !strcmp( *argv, "--uri" ) )
			arg.uri = *( ++argv );
		else if ( !strcmp( *argv, "-n" ) || !strcmp( *argv, "--host" ) )
			arg.host = *( ++argv );
		else if ( !strcmp( *argv, "-c" ) || !strcmp( *argv, "--content-type" ) )
			arg.ctype = *( ++argv );
		else if ( !strcmp( *argv, "-m" ) || !strcmp( *argv, "--method" ) )
			arg.method = *( ++argv );
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--protocol" ) )
			arg.protocol = *( ++argv );
		else if ( !strcmp( *argv, "-M" ) || !strcmp( *argv, "--multipart" ) )
			arg.multipart = 1;
		else if ( !strcmp( *argv, "-S" ) || !strcmp( *argv, "--msg-only" ) )
			arg.msgonly = 1;
		else if ( !strcmp( *argv, "-H" ) || !strcmp( *argv, "--headers-to" ) )
			arg.headerf = *( ++argv );
		else if ( !strcmp( *argv, "-B" ) || !strcmp( *argv, "--body-to" ) )
			arg.bodyf = *( ++argv );
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--random" ) )
			arg.randomize = 1;
		else if ( !strcmp( *argv, "-v" ) || !strcmp( *argv, "--verbose" ) )
			arg.verbose = 1;
		else if ( !strcmp( *argv, "-D" ) || !strcmp( *argv, "--dump" ) )
			arg.dump = 1;
		else if ( !strcmp( *argv, "-E" ) || !strcmp( *argv, "--headers" ) ) {
			char * a = *( ++argv );
			add_item( &arg.headers, a, unsigned char *, &arg.hlen );
		}
		else if ( !strcmp( *argv, "-F" ) || !strcmp( *argv, "--form" ) ) {
			char * a = *( ++argv );
			add_item( &arg.body, a, unsigned char *, &arg.blen );
		}
		else if ( !strcmp( *argv, "-b" ) || !strcmp( *argv, "--binary" ) ) {
			char * a = *( ++argv );
			add_item( &arg.body, a, unsigned char *, &arg.blen ); 
			arg.multipart = 1;
		}
		else if ( !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
			fprintf( stderr, "%s\n", HELP );
			return 0;
		}	
		argv++;
	}

	//Dump the arguments if you need to
	if ( arg.dump ) {
		dump_args( &arg );
	}

	//Catch any problems
	if ( !arg.method )
		arg.method = ( arg.body ) ? "POST" : "GET";
	else if ( !method_is_valid( arg.method ) ) {
		fprintf( stderr, PP ": Wrong method requested: %s\n", arg.method );
		return 1;
	}

	if ( !arg.path ) {
		fprintf( stderr, PP ": Directory to application not specified.\n" );
		return 1;
	}

	if ( !arg.uri )
		arg.uri = "/";
	else if ( *arg.uri != '/' ) {
		fprintf( stderr, PP ": URI is unspecified (only specify what comes after the domain).\n" );
		return 1;
	}

#if 1
	if ( !arg.filter ) {
		fprintf( stderr, PP ": No filter (or library) specified.\n" );
		return 1;
	} 

	if ( arg.filter ) {
		filter = NULL;
		for ( struct filter *f = filters; f->name; f++ ) {
			if ( strcmp( f->name, arg.filter ) == 0 ) {
				filter = f->filter, fname = (char *)f->name;
				break;
			}
		}
		if ( !filter ) {
			fprintf( stderr, PP ": Filter '%s' not supported.\n", arg.filter );
			return 1;
		}
	}
#else
	if ( !arg.lib ) {
		fprintf( stderr, PP ": No library specified.\n" );
		return 1;
	} 

	//Load the app, find the symbol and run the code...
	if ( !( app = dlopen( arg.lib, RTLD_LAZY ) ) ) {
		fprintf( stderr, PP ": Could not open application at %s: %s.\n", arg.lib, dlerror() );
		return 1;
	}

	//Find the name, since that will cause problems if not specified
	if ( !( fname = dlsym( app, NSYMBOL ) ) ) {
		dlclose( app );
		fprintf( stderr, PP ": symbol '%s' not found in filter: %s\n", NSYMBOL, strerror(errno) );
		return 1;
	} 

	//Find the main function symbol
	if ( !( filter = dlsym( app, FSYMBOL )) ) {
		dlclose( app );
		fprintf( stderr, PP ": symbol '%s' not found in filter: %s\n", FSYMBOL, strerror(errno) );
		return 1;
	}
#endif

	//Populate the request structure.  Normally, one will never populate this from scratch
	req.path = zhttp_dupstr( arg.uri );
	req.ctype = zhttp_dupstr( "text/html" );
	req.host = !arg.host ? zhttp_dupstr( "example.com" ) : zhttp_dupstr( arg.host );
	req.method = zhttp_dupstr( arg.method );
	req.protocol = !arg.protocol ? zhttp_dupstr( "HTTP/1.1" ) : zhttp_dupstr( arg.protocol );

fprintf( stderr, "%s\n", req.host );
getchar();
#if 0
	req.alias = !arg.alias ? zhttp_dupstr( "example.com" ) : zhttp_dupstr( arg.alias );
#endif

	//TODO: This is damned ugly...
	//Add any extra headers
	if ( arg.headers ) { 
		for ( char **headers = arg.headers; *headers; headers++ ) {
			if ( index( *headers, '=' ) ) { 
				int a = 0, b = 0, eq = 0;
				char aa[ 1024 ] = { 0 }, bb[ 1024 ] = { 0 };
				for ( char *header = *headers; *header; header++ ) {
					if ( !( eq += ( *header == '=' ) ) ) 
						aa[ a++ ] = *header;
					else {
						*header != '=' ? bb[ b++ ] = *header : 0;
					}
				}
				http_copy_header( &req, aa, bb );
			}
		}
		free( arg.headers ); 
	}

	//Also need to add GET vars from URI
	//TODO: Single char values do not work... strangely enough
	for ( char *ww = index( arg.uri, '?' ); ww ; ) {
		char aa[ 1024 ] = {0}, bb[ 1024 ] = {0};
		zWalker w = {0};
		ww++;

		for ( int len; strwalk( &w, ww, "&=" ); ) {
			len = ( w.chr == '=' || w.chr == '&' ) ? w.size	- 1 : w.size;
			if ( w.chr == '=' ) 
				memcpy( aa, &ww[ w.pos ], len );
			else {
				memcpy( bb, &ww[ w.pos ], len );
				http_copy_uripart( &req, aa, bb );
				memset( aa, 0, sizeof( aa ) );
				memset( bb, 0, sizeof( bb ) );
			}
		}

		break;
	}  

	//POST, PUT & DELETE typically expect a body...
	if ( method_expects_body( req.method ) && arg.body ) {
		//Make it multipart if requested
		if ( arg.multipart ) {
			//Need to free the original ctype
			if ( req.ctype ) {
				free( req.ctype );
			}
			req.ctype = zhttp_dupstr( "multipart/form-data" );
		}

		//Save all bodies 
		for ( char **body = arg.body; *body; body++ ) {
			if ( index( *body, '=' ) ) { 
				int a = 0, b = 0, eq = 0, bblen = 0;
				char aa[ 1024 ] = {0};
				unsigned char *bb = NULL;
				for ( char *value = *body; *value; value++ ) {
					if ( !( eq += ( *value == '=' ) ) ) 
						aa[ a++ ] = *value;
					else if ( *value != '=' ) {
						if ( *value != '@' ) {
							bb = (unsigned char *)value, bblen = strlen( value );	
							http_copy_formvalue( &req, aa, bb, bblen );
						}
						else { 
							if ( !( bb = read_file( ++value, &bblen, err, sizeof( err ) ) ) ) {
								fprintf( stderr, PP ": couldn't open test file: %s\n", value );	
								//TODO: Things should be freed here...
								return 1;
							}
							http_copy_formvalue( &req, aa, bb, bblen );
							free( bb );
						}
						break;
					}
				}
			}
		}
		free( arg.body ); 
	}

	//Assemble a message from here...
	if ( !http_finalize_request( &req, err, sizeof( err ) ) ) {
		fprintf( stderr, "%s\n", err );
		return 1; 
	}

	//Mock the site config data (or populate from file)
	sconf.name = req.host;
	sconf.alias = "";
	sconf.dir = arg.path;
	sconf.filter = fname;
	sconf.root_default = "";

	//Mock the connection data
	conn.count = 0;
	conn.status = 0;
	conn.ctx = NULL;
	conn.config = NULL;
	conn.hconfig = &sconf;
	conn.ipv4 = "192.168.0.1";
	//conn.ipv6 = '192.168.0.1';

	//Open any needed files (dying if you fail to do so)
	if ( arg.headerf ) {
		if ( *arg.headerf	== '-' )
			header_fd = 1;	
		else if ( ( header_fd = open( arg.headerf, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ) == -1 ) {
			fprintf( stderr, PP ": Opening header file %s - %s", arg.headerf, strerror( errno ) ); 
			http_free_request( &req );
			http_free_response( &res );
			return 1;
		}
	}

	if ( arg.bodyf ) {
		if ( *arg.bodyf	== '-' )
			body_fd = 1;	
		else if ( ( body_fd = open( arg.bodyf, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ) == -1 ) {
			fprintf( stderr, PP ": Opening body file %s - %s", arg.bodyf, strerror( errno ) ); 
			http_free_request( &req );
			http_free_response( &res );
			return 1;
		}
	}

	if ( !filter( 0, &req, &res, &conn ) ) {
		fprintf( stderr, PP ": HTTP funct '%s' failed to execute\n", FSYMBOL );
		write( 1, res.msg, res.mlen );
		fflush( stdout );
		http_free_request( &req );
		http_free_response( &res );
		return 1;
	}

	//Show whatever message should have come out
	if ( arg.headerf ) {
		write( header_fd, res.msg, res.mlen - res.clen );
	}

	if ( arg.msgonly || arg.bodyf ) {
		int sp = res.mlen - res.clen;	
		write( body_fd, &res.msg[ sp ], res.mlen - sp );
	}
	else if ( !arg.headerf ) {
		write( body_fd, res.msg, res.mlen );
		fflush( stdout );
	}

#if 0
	if ( arg.lib && dlclose( app ) == -1 ) {
		http_free_request( &req );
		http_free_response( &res );
		fprintf( stderr, PP ": Failed to close application: %s\n", strerror( errno ) );
		return 1;
	}
#endif

	//Destroy res, req and anything else allocated
	//print_httpbody( &req );
	//print_httpbody( &res );

	http_free_request( &req );
	http_free_response( &res );

	//Close files
	if ( header_fd > 2 ) {
		close( header_fd );
	}

	if ( body_fd > 2 ) {
		close( body_fd );
	}

	//After we're done, look at the response
	return 0;
}

