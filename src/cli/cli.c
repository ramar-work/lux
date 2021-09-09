/* ------------------------------------------- * 
 * cli.c
 * =====
 *
 * Summary 
 * -------
 * Command line tooling to help administer 
 * new sites.
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
 * ------------------------------------------- */
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <zwalker.h>
#include <ztable.h>
#include "../util.h"

#define NAME "hypno-cli"

#define CONFDIR "/etc/hypno/"

#define SHAREDIR "/usr/local/share/hypno/"

#define ERRPRINTF(...) \
	fprintf( stderr, "%s: ", NAME ); \
	fprintf( stderr, __VA_ARGS__ ); \
	fprintf( stderr, "%s", "\n" );

#define HELP \
	"-d, --dir <arg>          Define where to create a new application.\n"\
	"-n, --domain-name <arg>  Define a specific domain for the app.\n"\
	"    --title <arg>        Define a <title> for the app.\n"\
	"-s, --static <arg>       Define a static path. (Use multiple -s's to\n"\
	"                         specify multiple paths).\n"\
	"-b, --database <arg>     Define a specific database connection.\n"\
	"-x, --dump-args          Dump passed arguments.\n"
	//'t', "template-engine", "Define an alternate template engine" 

char *arg_srcdir = NULL;
char *arg_domain = NULL;
char *arg_title = NULL;
char *arg_database = NULL;
char arg_verbose = 0;
char arg_dump = 0;
char *arg_static[10] = { NULL };
const char ch = '@';


struct kv { 
	int size; 
	unsigned char *value; 
}	point[ 64 ] = {0};


struct app {
	const char *name;
	enum {
		H_DIR = 1
	,	H_FILE
	,	H_BINFILE
	} type;
	const char *path;
	const unsigned char *content;
};


struct app defaults[] = {
	{ "/app/", H_DIR, NULL },
	{ "/assets/", H_DIR, NULL },
	{ "/db/", H_DIR, NULL },
	{ "/misc/", H_DIR, NULL },
	{ "/lib/", H_DIR, NULL },
	{ "/private/", H_DIR, NULL },
	{ "/sql/", H_DIR, NULL },
	{ "/src/", H_DIR, NULL },
	{ "/views/", H_DIR, NULL },
	{ "/app/hello.lua", H_FILE, SHAREDIR "app.hello.lua" },
	{ "/views/hello.tpl", H_FILE, SHAREDIR "views.hello.tpl" },
	{ "/config.lua", H_FILE, SHAREDIR "config.lua" },
	{ "/config.example.lua", H_FILE, SHAREDIR "config.example.lua" },
	{ "/ROBOTS.TXT", H_FILE, SHAREDIR "ROBOTS.TXT" },
	{ "/favicon.ico", H_BINFILE, SHAREDIR "favicon.ico" },
	{ NULL }
};


struct kset {
	const char *key;
	char **ptr;
	int len;
} kset[] = {
	{ "db", &arg_database, 2 },
	{ "fqdn", &arg_domain, 4 },
	{ "title", &arg_title, 5 },
	{ NULL }
};



//Return the basename
char *basename ( char *path ) {
	int len = strlen( path );
	char *p = path + len; 
	for ( ; len > 0 && *p != '/'; p--, --len );
	return ( *p == '/' ) ? ++p : p;
}


//Return an unsigned char with find and replace activated 
struct kv * replace ( unsigned char *f ) {
	unsigned char *f1 = f;
	int size = 0;
	struct kv *p = point;
	memset( p, 0, sizeof( point ) /	sizeof( struct kv ) ); 

	for ( int inner = 0; *f; f++ ) {
		if ( *f == ch && ( *f = '"' ) ) {
			if ( !inner )
				p->size = ++size, p->value = f1;
			else {
				f1++;
				for ( struct kset *k = kset; k->key; k++ ) {
					if ( !memcmp( f1, k->key, k->len ) ) {
						p->size = strlen( *k->ptr ), p->value = (unsigned char *)*k->ptr;
					}
				}
			}
			inner = !inner, f1 = f, size = 0, p++;
		}
		size++;
	}
	p->size = size, p->value = f1, ++p, p->size = -1, p->value = NULL;
	return point;
}


//Create a new directory
int dir_cmd( struct app *layout, char *err, int errlen ) {
	while ( layout->name ) { 	
		char rname[ 2048 ] = { 0 };
		snprintf( rname, sizeof( rname ) - 1, "%s%s", arg_srcdir, layout->name );
		//fprintf( stdout, "resource name: %s\n", rname );
		if ( layout->type == H_DIR ) {	
			if ( mkdir( rname, 0755 ) == -1 ) {
				snprintf( err, errlen, "Directory creation failure at %s: %s", rname, strerror( errno ));
				return 0;
			}
		}
		else { 
			int fd = 0, len = 0;
			unsigned char *content = NULL; 
			//TODO: Work on my dereferencing, there's no reason I should need this
			char lerr[ 1024 ] = { 0 };

			if ( !( content = read_file( layout->path, &len, lerr, sizeof( lerr ) ) ) ) {
				memcpy( err, lerr, strlen( lerr ) );
				return 0;
			}

			if ( ( fd = open( rname, O_CREAT | O_RDWR, 0644 ) ) == -1 ) {
				snprintf( err, errlen, "Failed to open file at %s: %s", rname, strerror( errno ));
				return 0;
			} 

			if ( layout->type == H_BINFILE && write( fd, content, len ) == -1 ) {
				snprintf( err, errlen, "Failed to write to file at %s: %s", rname, strerror( errno ));
				return 0;
			}
			else {
				for ( struct kv *c = replace( content ); c->size > -1; c++ ) {
					write( fd, c->value, c->size );
				}
			}

			if ( close( fd ) == -1 ) {
				snprintf( err, errlen, "Could not close file at %s: %s", rname, strerror( errno ));
				return 0;
			}
	
			free( content );
		}
		layout++;
	}	
	return 1;
}


//Dump all command line arguments
void dump () {
	fprintf( stderr, "Directory: %s\n", arg_srcdir );
	fprintf( stderr, "Domain Name: %s\n", arg_domain );
	fprintf( stderr, "Title: %s\n", arg_title );
	fprintf( stderr, "Database: %s\n", arg_database );
	fprintf( stderr, "Static paths:\n" );
	for ( char **sp = arg_static; *sp; sp++ ) {
		fprintf( stderr, "%s\n", *sp );
	}
}


int main ( int argc, char *argv[] ) {
	char err[ 2048 ] = { 0 };
	struct stat sb;

	//Process all your options...
	if ( argc < 2 ) {
		fprintf( stderr, "No options received:\n" );
		fprintf( stderr, "%s", HELP );
		return 1;
	}
 
	while ( *argv ) {
		if ( strcmp( *argv, "-v" ) == 0 || strcmp( *argv, "--verbose" ) == 0 ) 
			arg_verbose = 1;
		else if ( strcmp( *argv, "-x" ) == 0 || strcmp( *argv, "--dump-args" ) == 0 ) 
			arg_dump = 1;
		else if ( strcmp( *argv, "-h" ) == 0 || strcmp( *argv, "--help" ) == 0 ) {
			fprintf( stderr, "%s", HELP );
			return 1;
		}
		else if ( strcmp( *argv, "-d" ) == 0 || strcmp( *argv, "--dir" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --dir!" );
				return 1;
			} 
			arg_srcdir = *argv;
		}
		else if ( strcmp( *argv, "-n" ) == 0 || strcmp( *argv, "--domain-name" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --domain-name!" );
				return 1;
			} 
			arg_domain = *argv;
		}
		else if ( strcmp( *argv, "--title" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --title!" );
				return 1;
			} 
			arg_title = *argv;
		}
		else if ( strcmp( *argv, "-b" ) == 0 || strcmp( *argv, "--database" ) == 0 ) {
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --database!" );
				return 1;
			} 
			arg_database = *argv;
		}
		else if ( strcmp( *argv, "-s" ) == 0 || strcmp( *argv, "--static" ) == 0 ) {
			static char **as = arg_static;
			argv++;
			if ( !*argv ) {
				ERRPRINTF( "Expected argument for --static!" );
				return 1;
			} 
			*(as++) = *argv;
		}
		argv++;
	}
	
	if ( !arg_srcdir ) {
		ERRPRINTF( "No source directory specified." );
		return 1;
	}

	if ( stat( arg_srcdir, &sb ) != -1 ) {
		fprintf( stderr, "Source directory '%s' already exists.\n", arg_srcdir );
		return 1;	
	} 
	else {
		( arg_verbose ) ? fprintf( stderr, "Source directory '%s' does not exist.\n", arg_srcdir ) : 0;
		if ( mkdir( arg_srcdir, 0755 ) == -1 ) {
			ERRPRINTF( "Failed to create source directory '%s': %s.", arg_srcdir, strerror(errno) );
			return 1;
		}
	}

	//Fill in argument defaults?
	arg_database = arg_database ? arg_database : "none" ;
	arg_domain = arg_domain ? arg_domain : basename( arg_srcdir );
	arg_title = arg_title ? arg_title : basename( arg_srcdir );

	if ( arg_dump ) {
		dump();
	}

	if ( !dir_cmd( defaults, err, sizeof(err) ) ) {
		ERRPRINTF( "%s.", err );
		return 1;
	}

	return 0;
}
