//cli.c - A way to quickly generate new sites to test Hypno...
#include "../vendor/single.h"

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

#define ERRPRINTF(...) \
	fprintf( stderr, "%s: ", "hcli" ); \
	fprintf( stderr, __VA_ARGS__ ); \
	fprintf( stderr, "%s", "\n" );

struct app {
	const char *name;
	const char  type;  //DIR, FILE
	const char *content;
};

char *arg_srcdir = NULL;
char arg_verbose = 0;
const int H_DIR = 31;
const int H_FILE = 32;

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
	{ NULL }
};


void help (void) {
	fprintf( stderr, "%s: No options received.\n", __FILE__ );
	const char *fmt = "-%c, --%-10s       %-30s\n";
	fprintf( stderr, fmt, 'd', "dir", "Define where to create a new application" );
	exit( 0 );
	return;
}


int dir_cmd( struct app *layout, char *err, int errlen ) {


	while ( layout->name ) { 	
		char rname[ 2048 ] = { 0 };
		snprintf( rname, sizeof( rname ) - 1, "%s%s", arg_srcdir, layout->name );
		//fprintf( stdout, "resource name: %s\n", rname );
#if 1
		if ( layout->type == H_DIR ) {	
			if ( mkdir( rname, S_IRWXU ) == -1 ) {
				snprintf( err, errlen, "Directory creation failure at %s: %s", rname, strerror( errno ));
				return 0;
			}
		}
		else if ( layout->type == H_FILE ) {
			int fd = 0;
			const char *content = layout->content;
			if ( ( fd = open( rname, S_IRWXU | O_RDWR ) ) == -1 ) {
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
	return 1;
}


int main (int argc, char *argv[]) {
	//Process all your options...
	( argc < 2 ) ? help() : 0;
	while ( *argv ) {
		if ( strcmp( *argv, "--help" ) == 0 ) 
			help();	
		else if ( strcmp( *argv, "--verbose" ) == 0 ) 
			arg_verbose = 1;
		else if ( strcmp( *argv, "--port" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --port!" );
				return 0;
			} 
		}
		else if ( strcmp( *argv, "--dir" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --dir!" );
				return 0;
			} 
			arg_srcdir = *argv;
		}
		argv++;
	}
	
	char err[ 2048 ] = { 0 };
	if ( !arg_srcdir ) {
		ERRPRINTF( "No source directory specified." );
		return 0;
	}

	struct stat sb;
	if ( stat( arg_srcdir, &sb ) == -1 ) {
		fprintf( stderr, "Source directory '%s' does not exist.\n", arg_srcdir );
		if ( mkdir( arg_srcdir, S_IRWXO ) == -1 ) {
			ERRPRINTF( "Failed to create source directory '%s': %s.", arg_srcdir, strerror(errno) );
			return 0;
		}
	}

	if ( !dir_cmd( defaults, err, sizeof(err) ) ) {
		ERRPRINTF( err );
		return 0;
	}
	return 0;
}
