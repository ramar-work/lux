//harness.c - code needed to run test harness stuff
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <dlfcn.h>
#include <errno.h>
#include <zwalker.h>
#include <zhttp.h>
#include "util.h"

#if 0
char * strupper ( const char *str ) {
	return NULL;
}

char * strlower ( const char *str ) {
	return NULL;
}

char * mstrlower ( const char *str ) {
	return NULL;
}

char * mstrupper ( const char *str ) {
	return NULL;
}

char * strjoin ( const char *str, ... ) {
	return NULL;
}

char ** strbreak ( const char *str ) {
	return NULL;
}

char * strbefore ( const char *str, const char *delim ) {
	return NULL;
}

char * strafter ( const char *str, const char *delim ) {
	return NULL;
}
#endif


#define PP "harness"

#define BASIC_HTML \
	"<html>\n" \
	"	<head>\n" \
	"		<title>Hypno Site Test</title>\n" \
	"	</head>\n" \
	"	<body>\n" \
	"		<h2>Hypno Example Site</h2>\n" \
	"		<p>This is an example page to help users test out Hypno.</p>\n" \
	"	</body>\n" \
	"</html>\n"

#define DEFAULT_LUA \
	"return {\n" \
	"	db = \"roast.db\",\n" \
	"	title = \"da food snob\",\n" \
	"	fqdn = \"dafoodsnob.com\",\n" \
	"	template_engine = \"roast.db\",\n" \
	"	static = \"static\",\n" \
	"	routes = {\n" \
	"		default = { model=\"roast\",view=\"roast\" },\n" \
	"		turkey = { model=\"turkey\",view=\"roast\" },\n" \
	"		chicken = { model=\"chicken\",view=\"roast\" },\n" \
	"		beef = { model=\"beef\",view=\"roast\" },\n" \
	"		recipe = {\n" \
	"			[\":id=number\"] = { model=\"recipe\",view=\"recipe\" },\n" \
	"		},\n" \
	"	}	\n" \
	"}\n"

#define ERRPRINTF(...) \
	fprintf( stderr, "%s: ", PP ); \
	fprintf( stderr, __VA_ARGS__ ); \
	fprintf( stderr, "%s", "\n" );

#define HELP \
"-l, --library <arg>      Specify a library for testing (required).\n" \
"-u, --uri <arg>          Specify a URI for testing (required).\n" \
"-c, --content-type <arg> Specify a content-type for testing.\n" \
"-n, --host <arg>         Specify a hostname for use w/ the request.\n" \
"-s, --symbol <arg>       Specify a hostname for use w/ the request.\n" \
"-m, --method <arg>       Specify an HTTP method to be used when making\n" \
"                         a request. (GET is default)\n" \
"-p, --protocol <arg>     Specify alternate protocols (HTTP/1.0, 2.0, etc)\n" \
"-B, --body <arg>         Specify a body to use when making requests.\n" \
"                         (Use multiple invocations for additional arguments)\n" \
"-H, --header <arg>       Specify a header to use when making requests.\n" \
"                         (Use multiple invocations for additional arguments)\n" \
"-M, --multipart          Use a multipart request when using POST or PUT\n" \
"-v, --verbose            Be wordy.\n" \
"-h, --help               Show help and quit.\n"


const int H_DIR = 31;
const int H_FILE = 32;

struct arg {
	char *lib;
	char *path;
	char *ctype;
	char *host;
	char *method;
	char *symbol;
	char *protocol;
	char *uri;
	char *srcdir;
	int verbose;
	int randomize;
	int multipart;
	int blen;
	int hlen;
	char **headers;
	char **body;
};

struct app {
	const char *name;
	const char  type;  //DIR, FILE
	const char *content;
};

struct app defaults[] = {
	{ "/app/", H_DIR, NULL },
	{ "/views/", H_DIR, NULL },
	{ "/log/", H_DIR, NULL },
	{ "/middleware/", H_DIR, NULL },
	{ "/misc/", H_DIR, NULL },
	{ "/static/", H_DIR, NULL },
	{ "/static/index.html", H_FILE, BASIC_HTML },
	{ "/app/hello.lua", H_FILE, NULL },
	{ "/views/hello.tpl", H_FILE, NULL },
	{ "/config.lua", H_FILE, DEFAULT_LUA },
	{ NULL }
};


static int method_expects_body ( char *mstr ) {
	const char *mth[] = { 
		"POST", "PUT", "DELETE"
	};

	for ( int i=0; i<sizeof(mth)/sizeof(const char *); i++ ) {
		if ( !strcasecmp( mth[i], mstr ) ) return 1;
	}

	return 0;	
}



static int method_is_valid ( char *mstr ) {
	const char *mth[] = { 
		"HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS", "TRACE" 
	};

	for ( int i=0; i<sizeof(mth)/sizeof(const char *); i++ ) {
		if ( !strcasecmp( mth[i], mstr ) ) return 1;
	}

	return 0;
}



//Dir cmd should create a new thing
#if 0
int dir_cmd ( struct arg *arg ) {
	//Define
	struct stat sb;

	if ( !arg->srcdir ) {
		ERRPRINTF( "No source directory specified." );
		return 0;
	}

	if ( stat( arg->srcdir, &sb ) == -1 ) {
		fprintf( stderr, "Source directory '%s' does not exist.\n", arg->srcdir );
		if ( mkdir( arg->srcdir, 0755 ) == -1 ) {
			ERRPRINTF( "Failed to create source directory '%s': %s.", arg->srcdir, strerror(errno) );
			return 0;
		}
	}

	for ( struct app *layout = defaults; layout->name; layout++ ) {
		char rname[ 2048 ] = { 0 };
		snprintf( rname, sizeof( rname ) - 1, "%s%s", arg_srcdir, layout->name );
		//fprintf( stdout, "resource name: %s\n", rname );
#if 1
		if ( layout->type == H_DIR ) {	
			if ( mkdir( rname, 0755 ) == -1 ) {
				snprintf( err, errlen, "Directory creation failure at %s: %s", rname, strerror( errno ));
				return 0;
			}
		}
		else if ( layout->type == H_FILE ) {
			int fd = 0;
			const char *content = layout->content;
			if ( ( fd = open( rname, O_CREAT | O_RDWR, 0755 ) ) == -1 ) {
				snprintf( err, errlen, "Failed to open file at %s: %s", rname, strerror( errno ));
				return 0;
			} 

			if ( content ) {
				if ( write( fd, content, strlen( content ) ) == -1 ) {
					snprintf( err, errlen, "Failed to write to file at %s: %s", rname, strerror( errno ));
					return 0;
				}
			} 

			if ( close( fd ) == -1 ) {
				snprintf( err, errlen, "Could not close file at %s: %s", rname, strerror( errno ));
				return 0;
			} 
		}
#endif
		layout++;
	}	


}
#endif


//harness cmd should test endpoints and whatnot
#if 0
int harness_cmd ( struct arg *arg ) {
	struct HTTPBody req = {0}, res = {0};
	void *app = NULL;
	int (*httpfunc)( struct HTTPBody *, struct HTTPBody * );

	//Catch any problems
	if ( !arg.method )
		arg.method = "GET";
	else if ( !method_is_valid( arg.method ) ) {
		fprintf( stderr, PP ": Wrong method requested: %s\n", arg.method );
		return 1;
	}

	if ( !arg.lib ) {
		fprintf( stderr, PP ": No library specified.\n" );
		return 1;
	} 

	if ( !arg.uri )
		arg.uri = "/";
	else if ( *arg.uri != '/' ) {
		fprintf( stderr, PP ": URI is unspecified (only specify what comes after the domain).\n" );
		return 1;
	}


	//Populate the request structure.  Normally, one will never populate this from scratch
	req.path = zhttp_dupstr( arg.uri );
	req.ctype = zhttp_dupstr( "text/html" );
	req.host = zhttp_dupstr( "example.com" );
#if 1
	req.method = zhttp_dupstr( "GET" );
	req.protocol = zhttp_dupstr( "HTTP/1.1" );
#else
	req.method = zhttp_dupstr( ( !arg.method ) ? "GET" : arg.method );
	req.protocol = zhttp_dupstr( !arg.protocol ? "HTTP/1.1" : arg.protocol );
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

	//POST, PUT & DELETE typically expect a body...
	if ( method_expects_body( req.method ) && arg.body ) {
		//Make it multipart if requested
		if ( arg.multipart ) {
			req.ctype = "multipart/form-data";
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

	//Load the app, find the symbol and run the code...
	if ( !( app = dlopen( arg.lib, RTLD_LAZY ) ) ) {
		fprintf( stderr, PP ": Could not open application at %s: %s.\n", arg.lib, dlerror() );
		return 1;
	}

	if ( !( httpfunc = dlsym( app, arg.symbol )) ) {
		dlclose( app );
		fprintf( stderr, PP ": '%s' not found in C app: %s\n", arg.symbol, strerror(errno) );
		return 1;
	}

	if ( !httpfunc( &req, &res ) ) {
		fprintf( stderr, PP ": HTTP funct '%s' failed to execute\n", arg.symbol );
		write( 2, res.msg, res.mlen );
		http_free_request( &req );
		http_free_response( &res );
		return 1;
	}

	//Show whatever message should have come out
	write( 1, res.msg, res.mlen );
	fflush( stdout );

	if ( dlclose( app ) == -1 ) {
		http_free_request( &req );
		http_free_response( &res );
		fprintf( stderr, PP ": Failed to close application: %s\n", strerror( errno ) );
		return 1;
	}

	//Destroy res, req and anything else allocated
	http_free_request( &req );
	http_free_response( &res );


}
#endif



int main ( int argc, char * argv[] ) {
	//Make this
	struct arg arg = {0};
	int blen = 0;
	char err[ 2048 ] = { 0 };

	//check for options
	if ( argc < 2 ) {
		fprintf( stderr, PP ":\n%s\n", HELP );
		return 1;
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "-l" ) || !strcmp( *argv, "--library" ) )
			arg.lib = *( ++argv );
		else if ( !strcmp( *argv, "-u" ) || !strcmp( *argv, "--uri" ) )
			arg.uri = *( ++argv );
		else if ( !strcmp( *argv, "-c" ) || !strcmp( *argv, "--content-type" ) )
			arg.ctype = *( ++argv );
		else if ( !strcmp( *argv, "-m" ) || !strcmp( *argv, "--method" ) )
			arg.method = *( ++argv );
		else if ( !strcmp( *argv, "-s" ) || !strcmp( *argv, "--symbol" ) )
			arg.symbol = *( ++argv );
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--protocol" ) )
			arg.protocol = *( ++argv );
		else if ( !strcmp( *argv, "-M" ) || !strcmp( *argv, "--multipart" ) )
			arg.multipart = 1;
		else if ( !strcmp( *argv, "-F" ) || !strcmp( *argv, "--form" ) )
			add_item( &arg.body, *( ++argv ), unsigned char *, &arg.blen );
		else if ( !strcmp( *argv, "-B" ) || !strcmp( *argv, "--binary" ) )
			add_item( &arg.body, *( ++argv ), unsigned char *, &arg.blen ), arg.multipart = 1;
		else if ( !strcmp( *argv, "-H" ) || !strcmp( *argv, "--headers" ) )
			add_item( &arg.headers, *( ++argv ), unsigned char *, &arg.hlen );
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--random" ) )
			arg.randomize = 1;
		else if ( !strcmp( *argv, "-v" ) || !strcmp( *argv, "--verbose" ) )
			arg.verbose = 1;
		else if ( !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
			fprintf( stderr, "%s\n", HELP );
			return 0;
		}	
		else if ( strcmp( *argv, "--dir" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --dir!" );
				return 0;
			} 
			arg.srcdir = *argv;
		}
		argv++;
	}

#if 0
	harness_cmd( &arg );

	dir_cmd( &arg );
#endif

	//After we're done, look at the response
	return 0;
}

