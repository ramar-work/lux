/**
 * lxcli.c
 * -------
 * The primary CLI tool for lux.  Creates and manages new instances.
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
 *
 * ------------------------------------------- */
#include "lxcommon.h"

#define NAME "lxcli"

#define PORT 22

#define HOST "192.168.56.12"

#define HELP \
	"-c, --create             Create a new instance.\n" \
	"-d, --directory <arg>    Define where to create the new instance.\n" \
	"-n, --fqdn <arg>         Define a fully qualified domain name for this instance.\n" \
	"-t, --title <arg>        Define an HTML title for this instance.\n" \
	"-s, --static <arg>       Define static paths that the instance should serve. (Use \n" \
	"                         multiple --static flags to specify multiple paths).\n" \
	"-b, --database <arg>     Define a database connection to use with this instance.\n" \
	"-V, --version            Show version information and quit.\n" \
	"-v, --verbose            Tell me everything.\n" \
	"-h, --help               Show the help menu.\n"


struct repository {
	char *type;
	char *address;
};


// Default folder structure for new instances
const dir_t inst_def[] = {
	{ "/app/", H_DIR, NULL },
	{ "/assets/", H_DIR, NULL },
	{ "/db/", H_DIR, NULL },
	{ "/misc/", H_DIR, NULL },
	{ "/misc/certs/", H_DIR, NULL },
	{ "/lib/", H_DIR, NULL },
	{ "/lib/dependencies/", H_DIR, NULL },
	{ "/private/", H_DIR, NULL },
	{ "/private/setup/", H_DIR, NULL },
	{ "/routes/", H_DIR, NULL },
	{ "/tests/", H_DIR, NULL },
	{ "/sql/", H_DIR, NULL },
	{ "/src/", H_DIR, NULL },
	{ "/views/", H_DIR, NULL },
	{ "/app/hello.lua", H_FILE, SHAREDIR "app.hello.lua" },
	{ "/views/hello.tpl", H_FILE, SHAREDIR "views.hello.tpl" },
	{ "/config.lua", H_FILE, SHAREDIR "config.lua" },
	{ "/config.example.lua", H_FILE, SHAREDIR "config.example.lua" },
	{ "/robots.txt", H_FILE, SHAREDIR "robots.txt" },
	{ "/favicon.ico", H_BINFILE, SHAREDIR "favicon.ico" },
#if 0
	{ "/lib/README.md", H_FILE, SHAREDIR "lib.README.md" },
	{ "/app/README.md", H_FILE, SHAREDIR "app.README.md" },
	{ "/assets/README.md", H_FILE, SHAREDIR "assets.README.md" },
	{ "/db/README.md", H_FILE, SHAREDIR "db.README.md" },
	{ "/lib/README.md", H_FILE, SHAREDIR "lib.README.md" },
	{ "/misc/README.md", H_FILE, SHAREDIR "misc.README.md" },
	{ "/private/README.md", H_FILE, SHAREDIR "private.README.md" },
	{ "/routes/README.md", H_FILE, SHAREDIR "routes.README.md" },
	{ "/sql/README.md", H_FILE, SHAREDIR "sql.README.md" },
	{ "/src/README.md", H_FILE, SHAREDIR "src.README.md" },
	{ "/tests/README.md", H_FILE, SHAREDIR "tests.README.md" },
	{ "/views/README.md", H_FILE, SHAREDIR "views.README.md" },
#endif
	{ NULL }
};




/**
 * typedef struct config_t
 *
 * List of user arguments for `lxcli`
 *
 */
typedef struct config_t {

	/* Create a new instance */
	int create;

	/* Update an existing instance */
	int update;

	/* Delete an existing instance */
	int delete;

	/* You MAY need this, but this will change how package argument evaluation is done. */
	//char *repository;

	/* Define a name for the instance in question */
	char *instance;

	/* Define a fully qualified domain name for this new instance */
	char *domain;

	/* Define a title for this new instance */
	char *title;

	/* Define a default database for use with this backend */
	char *database;

	/* Define a list of static files */
	strings_t statics;

#if 0
	/* Specifying packages on the command line will install them to new instances */
	strings_t packages;
#endif

	/* Be verbose and say everyhting */
	int verbose;

} config_t;



/**
 * void print_config ( config_t *)
 *
 * Dump all in config_t.
 *
 */
static void print_config ( config_t *ua ) {
	fprintf( stderr, "Create:      %d\n", ua->create );
	fprintf( stderr, "Instance:    %s\n", ua->instance );
	fprintf( stderr, "Domain Name: %s\n", ua->domain  );
	fprintf( stderr, "Title:       %s\n", ua->title );
	fprintf( stderr, "Database:    %s\n", ua->database );
	fprintf( stderr, "Static paths:\n" );
	for ( const char **sp = ua->statics.strings; sp && *sp; sp++ ) {
		fprintf( stderr, "%s\n", *sp );
	}
#if 0
	fprintf( stderr, "Package:     %s\n", ua->package );
#endif
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
	config_t ua = {
		.create = 0,
		.instance = "",
		.domain = "",
		.title = "",
		.database = "",
		.verbose = 0,
		.statics = {
			.len = 0,
			.strings = NULL
		},
#if 0
		.package = NULL,
#endif
	};

	// Skip the first argument...
	for ( argv++; *argv; ) {
	
		// We got a non argument option	
		if ( *( *argv ) != '-' )
			return EXITPRINTF( 1, NAME ": Got unknown argument %s!\n", *argv );
		else if ( EVALARG( *argv, "-h", "--help" ) )
			return EXITPRINTF( 0, "%s\n", HELP );
		else if ( EVALARG( *argv, "-v", "--verbose" ) )
			ua.verbose = 1;
		else if ( EVALARG( *argv, "-c", "--create" ) )
			ua.create = 1;
		else if ( EVALARG( *argv, "-b", "--database" ) && !SAVEARG( argv, ua.database ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --database." );	
		else if ( EVALARG( *argv, "-d", "--directory" ) && !SAVEARG( argv, ua.instance ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --instance." );	
		else if ( EVALARG( *argv, "-n", "--fqdn" ) && !SAVEARG( argv, ua.domain ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --fqdn." );	
		else if ( EVALARG( *argv, "-t", "--title" ) && !SAVEARG( argv, ua.title ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --title." );	
		else if ( EVALARG( *argv, "-s", "--static" ) ) {
			if ( !*( ++argv ) ) {
				return EXITPRINTF( 1, "Expected argument for --static!" );
			}
			strings_t *s = &ua.statics;
			add_item( *argv, s->strings, char *, &s->len );
		}
#if 0
		else if ( EVALARG( *argv, "-p", "--package" ) && !SAVEARG( argv, ua.package ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --package." );	
#endif
		else if ( EVALARG( *argv, "-V", "--version" ) ) {
			fprintf( stdout, "%s\n", PACKAGE_VERSION );
			return 0;
		}
		else if ( !memcmp( *argv, "-", 1  ) ) {
			return EXITPRINTF( 1, NAME ": Got unknown argument %s!\n", *argv );
		}
		argv++;
	}

	//print_config( &ua );

	// Die if no instance is specified...
	if ( !ua.instance ) {
		fprintf( stderr, NAME ": No instance name specified!\n" );
		return 1;
	}

	// Create stuff
	if ( ua.create ) {

		// Define finds and replacements with this weird little structure
		kset_t reps[] = {
			{ "db", &ua.database, 2 },
			{ "fqdn", &ua.domain, 4 },
			{ "title", &ua.title, 5 },
			{ NULL }
		};	

		// Create all of the directories.
		if ( !create_dirs( ua.instance, (dir_t *)inst_def, reps, err, sizeof( err ) ) ) {
			fprintf( stderr, NAME ": Creating instance <instance-name> failed: %s\n", err );
			return 1;
		}
	}
	else {
		fprintf( stderr, NAME ": No actions specified.\n" );
		return 1;
	}


	return 0;
}
