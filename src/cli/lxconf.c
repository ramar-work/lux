/**
 * lxconf.c
 * -------
 * Create configuration files for lux server and/or local instances.
 *
 *
 * Usage
 * -----
 * See HELP below.
 *
 *
 * LICENSE
 * -------
 * Copyright 2020-2024 Antonio R. Collins II (ramar@ramar.work)
 *
 * See LICENSE in the top-level directory for more information.
 *
 *
 * TODO
 * ----
 * - Come up with a clean way to update hosts when adding new sites.
 * - -H name=x,dir=y,filter=z
 *
 */

#include "lxcommon.h"

#define NAME "lxconf"

#define HELP \
	"-c, --create             Create a new instance.\n" \
	"-o, --output <path>      Put the generated file here.\n" \
	"-V, --version            Show version information and quit.\n" \
	"-v, --verbose            Tell me everything.\n" \
	"-h, --help               Show the help menu.\n" \
	"-t, --type <arg>         Generate a configuration file for either a server or instance.\n" \
	"    --server             Alias for server configuration file generation.\n" \
	"    --instance           Alias for instance configuration file generation.\n" \
	"\n" \
  "Server specific:\n" \
	"\n" \
	"-w, --wwwroot <arg>      Define a wwwroot to use.\n" \
	"-f, --filter <arg>       Define a DEFAULT filter to use with all instances\n" \
	"                         associated with this configuration.\n" \
	"-H, --host <arg>         Define a host to use with this instance.\n" \
	"\n" \
  "Instance specific:\n" \
	"-b, --database <arg>     Define a database connection to use with this instance.\n" \
	"-n, --fqdn <arg>         Define a fully qualified domain name for this instance.\n" \
	"-T, --title <arg>        Define an HTML title for this instance.\n" \
	"-S, --static <arg>       Define static paths that the instance should serve. (Use \n"


const dir_t file_def[] = {
	{ "/app/hello.lua", H_FILE, SHAREDIR "app.hello.lua" },
	{ NULL },
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

	/* Choose whether this is for a server or an instance */
	int type;

	/* Define the path to write this file */
	char *path;

	/* Define the wwwroot for this server */
	char *wwwroot;

	/* Define the default filter for this server */
	char *filter;

	/* Define hosts for an instance */
	strings_t hosts;

	/* Define the fully qualified domain for this instance */
	char *domain;

	/* Define the path to a primary database for this configuration */
	char *database;

	/* Define an HTML title if it's an instance */
	char *title;

	/* Define static paths for an instance */
	strings_t statics;

	/* Be verbose and say everyhting */
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
	fprintf( stderr, "Type:        %c\n", ua->type );
	fprintf( stderr, "Path:        %s\n", ua->path );
	if ( ua->type == 's' ) {
		fprintf( stderr, "WWW Root:    %s\n", ua->wwwroot );
		//fprintf( stderr, "Default Filter:    %s\n", ua->database );
		//fprintf( stderr, "Default Port:    %s\n", ua->database );
		fprintf( stderr, "Hosts:\n" );
		for ( const char **sp = ua->statics.strings; sp && *sp; sp++ ) {
			fprintf( stderr, "%s\n", *sp );
		}
	}
	else {
		fprintf( stderr, "Domain Name: %s\n", ua->domain  );
		fprintf( stderr, "Title:       %s\n", ua->title );
		fprintf( stderr, "Database:    %s\n", ua->database );
		fprintf( stderr, "Static paths:\n" );
		for ( const char **sp = ua->statics.strings; sp && *sp; sp++ ) {
			fprintf( stderr, "%s\n", *sp );
		}
	}

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
		.type = 'i',
		.wwwroot = NULL,
		.filter = "lua",
		.domain = "",
		.title = "",
		.database = "",
		.verbose = 0,
		.hosts = {
			.len = 0,
			.strings = NULL
		},
		.statics = {
			.len = 0,
			.strings = NULL
		},
	};


	// Skip the first argument...
	for ( argv++; *argv; ) {
	
		// We got a non argument option	
		if ( *( *argv ) != '-' )
			return EXITPRINTF( 1, NAME ": Got unknown argument %s!\n", *argv );
		else if ( EVALARG( *argv, "-c", "--create" ) )
			ua.create = 1;
		else if ( EVALARG( *argv, "-v", "--verbose" ) )
			ua.verbose = 1;
		else if ( EVALARG( *argv, "-s", "--server" ) )
			ua.type = 's';	
		else if ( EVALARG( *argv, "-i", "--instance" ) )
			ua.type = 'i';	
		else if ( EVALARG( *argv, "-o", "--output" ) && !SAVEARG( argv, ua.path ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --output." );	
		else if ( EVALARG( *argv, "-w", "--wwwroot" ) && !SAVEARG( argv, ua.wwwroot ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --wwwroot." );	
		else if ( EVALARG( *argv, "-f", "--filter" ) && !SAVEARG( argv, ua.filter ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --filter." );	
		else if ( EVALARG( *argv, "-b", "--database" ) && !SAVEARG( argv, ua.database ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --database." );	
		else if ( EVALARG( *argv, "-n", "--fqdn" ) && !SAVEARG( argv, ua.domain ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --fqdn." );	
		else if ( EVALARG( *argv, "-T", "--title" ) && !SAVEARG( argv, ua.title ) )
			return EXITPRINTF( 1, "%s\n", "Argument required for --title." );	
		else if ( EVALARG( *argv, "-S", "--static" ) ) {
			if ( !*( ++argv ) ) {
				return EXITPRINTF( 1, "Expected argument for --static!" );
			}
			strings_t *s = &ua.statics;
			add_item( *argv, s->strings, char *, &s->len );
		}
		else if ( EVALARG( *argv, "-H", "--host" ) ) {
			if ( !*( ++argv ) ) {
				return EXITPRINTF( 1, "Expected argument for --host!" );
			}
			strings_t *s = &ua.statics;
			add_item( *argv, s->strings, char *, &s->len );
		}
		else if ( EVALARG( *argv, "-t", "--type" ) ) {
			if ( ! (*( ++argv )	) || *( *argv ) == '-' )
				return EXITPRINTF( 1, NAME ": No argument present for -t" );
			else if ( !strcasecmp( *argv, "server" ) )
				ua.type = 's';	
			else if ( !strcasecmp( *argv, "instance" ) )
				ua.type = 'i';	
			else {
				return EXITPRINTF( 1, NAME ": Got malformed argument %s for -t", *argv );
			}
		}
	#if 0
		else if ( EVALARG( *argv, "-u", "--update" ) ) {
			fprintf( stdout, "%s\n", PACKAGE_VERSION );
			return 0;
		}
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

print_config( &ua );

	// Create stuff
	if ( ua.create ) {

		const char *path = NULL;
		unsigned char *file = NULL;
		int fd = -1;
		int filelen = 0;
		kset_t kset[20] = { { NULL } };

		// Create an instance config file
		if  ( ua.type == 's' ) {
			path = SHAREDIR "config.server.lua.in";

			// wwwroot cannot be blank in this case
			if ( !ua.wwwroot ) {
				ua.wwwroot = ".";
			}

			// Define finds and replacements with this weird little structure
			kset[ 0 ].key = "wwwroot"; //, &ua.database, 2 };
			kset[ 0 ].ptr = &ua.wwwroot;
			kset[ 0 ].len = 7;

			// Define finds and replacements with this weird little structure
			kset[ 1 ].key = "default_filter"; //, &ua.database, 2 };
			kset[ 1 ].ptr = &ua.filter;
			kset[ 1 ].len = 14;
		}	

		// Create an instance config file
		else if ( ua.type == 'i' ) {
			path = SHAREDIR "config.instance.lua";

			// Define finds and replacements with this weird little structure
			kset[ 0 ].key = "db"; //, &ua.database, 2 };
			kset[ 0 ].ptr = &ua.database;
			kset[ 0 ].len = 2;
			kset[ 1 ].key = "fqdn"; //, &ua.database, 2 };
			kset[ 1 ].ptr = &ua.domain;
			kset[ 1 ].len = 4;
			kset[ 2 ].key = "title"; //, &ua.database, 2 };
			kset[ 2 ].ptr = &ua.title;
			kset[ 2 ].len = 5;

			// TODO: This should be preinitialized already
			kset[ 3 ].key = NULL;
		}

		// Open and read contents to memory
		if ( !( file = read_file( path, &filelen, err, sizeof( err ) ) ) ) {
			fprintf( stderr, NAME ": File read error: %s\n", err );
			return 0;
		}

		// Try opening whatever the user asked for
		if ( !ua.path || ( strlen( ua.path ) == 1 && *ua.path == '-' ) )
			fd = 1;
		// Blow the original away if you specified a path
		else if ( ( fd = open( ua.path, O_CREAT | O_RDWR | O_TRUNC, 0644 ) ) == -1 ) {
			fprintf( stderr, NAME ": File open error: %s\n", strerror( errno ) );
			return 0;
		}

		// Write and replace
		for ( struct kv *c = replace( file, kset ); c->size > -1; c++ ) {
			write( fd, c->value, c->size );
		}

		// Free
		free( file );

		// Close file if not 1
		if ( fd > 2 ) {
			close( fd );	
		}
	}
	else {
		fprintf( stderr, NAME ": No actions specified.\n" );
		return 1;
	}

	return 0;
}
