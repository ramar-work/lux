/**
 * lxsrv.c
 * =======
 *
 * Manages the lux web server and supplementary daemons.
 *
 *
 * Usage
 * -----
 * -s, --start               Start the server
 * -k, --kill                Kill a running server
 * -c, --configuration <arg> Use this Lua file for configuration
 * -p, --port <arg>          Start using a different port 
 *     --pidfile <arg>       Define a PID file
 * -u, --user <arg>          Choose an alternate user to run as
 * -g, --group <arg>         Choose an alternate group to run as
 * -x, --dump                Dump configuration at startup
 * -L, --logfile <arg>       Define an alternate log file location
 * -V, --version             Show version information and quit.
 * -h, --help                Show the help menu.
 *
 *
 * LICENSE
 * -------
 * See LICENSE in the top-level directory for more information.
 *
 *
 */


#include "lxcommon.h"
#include <pthread.h>
#include <pwd.h>
#include <dlfcn.h>

#include "../config.h"
#include "../logging/log.h"
#include "../server/server.h"
#if 0
#include "../filters/filter-dirent.h"
#include "../filters/filter-redirect.h"
#endif
#include "../filters/filter-static.h"
#include "../filters/filter-echo.h"
#include "../filters/filter-lua.h"
#include "../ctx/ctx-http.h"

// New server types
#include "../server/multithread.h"
#include "../server/single.h"

#ifndef DISABLE_TLS
 #include "../ctx/ctx-https.h"
#endif

#ifndef PIDDIR
 #define PIDDIR "/var/run/"
#endif

#define NAME "lxsrv"

#define LIBDIR "/var/lib/" NAME

#define PIDFILE PIDDIR NAME ".pid"

#define REAPING_THREADS

#define HELP \
	"-s, --start                 Start the server\n" \
	"-k, --kill                  Kill a running server\n" \
	"-c, --configuration <arg>   Use this Lua file for configuration\n" \
	"-p, --port <arg>            Start using a different port \n" \
	"    --pidfile <arg>         Define a PID file\n" \
	"-u, --user <arg>            Choose an alternate user to run as\n" \
	"-g, --group <arg>           Choose an alternate group to run as\n" \
	"-x, --dump                  Dump configuration at startup\n" \
	"-L, --logfile <arg>         Define an alternate log file location\n" \
	"-V, --version               Show version information and quit.\n" \
	"-W, --wbuffer <arg>         Set a write buffer.\n" \
	"-M, --max-connections <arg> Set maximum connections (per context).\n" \
	"-T, --max-threads <arg>     Set maximum threads (per context).\n" \
	"-B, --backlog <arg>         Set the socket backlog.\n" \
	"-r, --wwwroot <arg>         Define where to create a new application.\n"\
	"-F, --default-filter <arg>  Use filter <arg> as the default.\n" \
	"-t, --hostname <arg>        Respond to requests with this hostname.\n" \
	"-I, --ip-address <arg>      Respond to requests received at this IP address.\n" \
	"-h, --help                  Show the help menu."

#if 0
	"-d, --dir <arg>          Define where to create a new application.\n"\
	"-n, --domain-name <arg>  Define a specific domain for the app.\n"\
	"    --title <arg>        Define a <title> for the app.\n"\
	"-s, --static <arg>       Define a static path. (Use multiple -s's to\n"\
	"                         specify multiple paths).\n"\
	"-b, --database <arg>     Define a specific database connection.\n"\
	"-x, --dump-args          Dump passed arguments.\n" \
	"    --no-fork            Do not fork\n" \
	"    --debug              set debug rules\n"
#endif

const int defport = DEFAULT_WWW_PORT;

char pidbuf[128] = {0};

FILE * logfd = NULL;

protocol_t *ctx = NULL;

int procpid = 0;

int fdset[ 10 ] = { -1 };


typedef enum model_t {
	SERVER_ONESHOT = 0,
	SERVER_MULTITHREAD,
} model_t;



typedef enum userprotocol_t {
	UP_HTTP = 0,
	UP_HTTPS
} userprotocol_t;



typedef struct config_t {
	char *config;
	char *group;
	char *libdir;
	char *logfile;
	char *pidfile;
	char *user;
#if 1
	char *idir;
	char *deffilter;
	char *hostname;
	char *ipaddr;
	char *wwwroot;
	char *rootdef;
#endif
	int dump;
	int fork;
	int gid;
	int kill;
	int port;
	int start;
	int uid;
	int verbose;
	int wbuffer;
	model_t model;
	pid_t pid;
	short int backlog;
	short int maxper;
	short int maxthr;
	userprotocol_t userprotocol;
#ifdef DEBUG_H
	int tapout;
#endif
} config_t; 



/**
 * filter_t http_filters[] = 
 *
 * Supported filters are listed here.
 * TODO: These may move to a shared object.
 *
 */
filter_t http_filters[] = {
#if 0
,	{ "dirent", filter_dirent }
,	{ "redirect", filter_redirect }
#endif
	{ "static", filter_static }
, { "lua", filter_lua }
, { "echo", filter_echo }
, { NULL }
};



/**
 * protocol_t sr[] = 
 *
 * Supported protocols are listed here.
 * TODO: These may move to a shared object.
 *
 */
protocol_t sr[] = {
	{ "http", read_notls, write_notls, create_notls, free_notls, pre_notls, post_notls },
#ifndef DISABLE_TLS
	{ "https", read_gnutls, write_gnutls, create_gnutls, free_gnutls, pre_gnutls, post_gnutls },
#endif
#if 0
	{ "dns", read_dns, write_dns, create_dns, NULL, pre_dns, post_dns },
	{ "rtmp", read_rtmp, write_rtmp, create_rtmp, NULL, pre_rtmp, post_rtmp },
#endif
	{ NULL }
};



/**
 * struct log loggers[] = 
 *
 * Supported logger types are listed here.
 *
 */
struct log loggers[] = {
	{ f_open, f_close, f_write, NULL },
 	{ sqlite3_log_open, sqlite3_log_close, sqlite3_log_write, NULL }
};



#if 0
// We can drop privileges permanently
int revoke_priv ( config_t *v, char *err, int errlen ) {
	//You're root, but you need to drop to v->user, v->group
	//This can fail in a number of ways:
	//- you're not root,
	//- the user or group specified does not exist
	//- completely different thing could go wrong
	//Privilege seperation should be done here.
	struct passwd *p = getpwnam( v->user );
	gid_t ogid = v->gid, ngid;
	uid_t ouid = v->uid, nuid;

	//uid and gid should be blank if a user was specified
	if ( ouid == -1 ) {
		ogid = getegid(), ouid = geteuid();	
	}

	//Die if we can't find the user that we're supposed to run as
	if ( !p ) {
		snprintf( err, errlen, "user %s not found.\n", v->user );
		return 0;
	}

	//This is the user to switch to
	ngid = p->pw_gid, nuid = p->pw_uid;

	//Finally, if the two aren't the same, switch to the new one
	if ( ngid != ogid ) {
		char *gname = getpwuid( ngid )->pw_name;
	#if 1
		if ( setegid( ngid ) == -1 || setgid( ngid ) == -1 ) {
	#else
		if ( setreuid( ngid, ngid ) == -1 ) {
	#endif
			snprintf( err, errlen, "Failed to set run-as group '%s': %s\n", gname, strerror( errno ) );
			return 0;
		}
	}

	//seteuid does not work, why?
	if ( nuid != ouid ) {
	#if 1
		if ( /*seteuid( nuid ) == -1 || */ setuid( nuid ) == -1 ) {
	#else
		if ( setreuid( nuid, nuid ) == -1 ) {
	#endif
			snprintf( err, errlen, "Failed to set run-as user '%s': %s\n", p->pw_name, strerror( errno ) );
			return 0;
		}
	}
	return 1;
}
#endif



/**
 * int write_pid( int pid, char *pidfile, char *err, int errlen )
 *
 * Write the process ID to file.
 *
 */
int write_pid( int pid, char *pidfile, char *err, int errlen ) {

	char buf[ 64 ] = { 0 };
	int fd = -1, len = snprintf( buf, 63, "%d", pid );

	if ( !pidfile ) {
		snprintf( err, errlen, "No PID file specified." );
		return 0;
	}

	if ( ( fd = open( pidfile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR ) ) == -1 ) {
		snprintf( err, errlen, "Failed to access PID file: %s.", strerror(errno) );
		return 0;
	}

	//Write the pid down
	if ( write( fd, buf, len ) == -1 ) {
		snprintf( err, errlen, "Failed to log PID: %s.", strerror(errno) );
		return 0;
	}

	//The parent exited successfully.
	if ( close(fd) == -1 ) {
		snprintf( err, errlen, "Could not close PID file: %s", strerror(errno) );
		return 0;
	}

	return 1;
}



/**
 * void sigkill( int signum ) 
 *
 * Handle kill signals
 *
 */
void sigkill( int signum ) {
	fprintf( stderr, "Received SIGKILL - Killing the server...\n" );
	char err[ 2048 ] = {0};

	//Kill any open sockets.  This is the only time we'll see this type of looping
	for ( int i = 0; i < sizeof( fdset ) / sizeof( int ); i++ )	{
		if ( fdset[ i ] < 3 ) {
			break;
		}
		#if 0
		for ( count = 0; count >= 64; count++ ) {
			int s = pthread_join( ta_set[ count ].id, NULL );
			FPRINTF( "Joining thread at pos %d, status = %d....\n", count, s );
			if ( s != 0 ) {
				FPRINTF( "Pthread join at pos %d failed...\n", count );
				continue;
			}
		}
		#endif

		//Should be reaping all of the open threads...
		if ( close( fdset[ i ] ) == -1 ) {
			fprintf( logfd, "Failed to close fd '%d': %s", fdset[ i ], strerror( errno ) );
		}
	}

#if 0
	cmd_kill( NULL, err, sizeof( err ) );
#endif
}



/**
 * int cmd_kill ( config_t *v, char *err, int errlen )
 *
 * Kills a running server.
 *
 */
int cmd_kill ( config_t *v, char *err, int errlen ) {
	//Open a file
	struct stat sb;
	DIR *dir = NULL;
	const char *dname = PIDDIR;

	if ( !( dir = opendir( dname ) ) ) {
		snprintf( err, errlen, "Failed to open PID directory: %s\n", strerror( errno ) );
		return 0;	
	}

	for ( struct dirent *d; ( d = readdir( dir ) ); ) {
		if ( *d->d_name	== '.' ) {
			continue;
		}

	#ifdef DEBUG_H
		fprintf( stderr, "Checking %s/%s\n", dname, d->d_name );
	#endif

		if ( memstrat( d->d_name, NAME "-", strlen( d->d_name ) ) > -1 ) {
			fprintf( stderr, "I found a PID file at: %s/%s\n", dname, d->d_name );
			//Read the contents in and kill from here?
			char fpid[ 64 ] = {0}, fname[ 2048 ] = {0};
			int pid, fd = 0;
			snprintf( fname, sizeof( fname ), "%s/%s", dname, d->d_name );
			if ( ( fd = open( fname, O_RDONLY ) ) == -1 ) {
				snprintf( err, errlen, "Failed to open PID file: %s\n", strerror( errno ) );
				return 0;
			}

			if ( read( fd, fpid, sizeof( fpid ) ) == -1 ) {
				snprintf( err, errlen, "Failed to read PID file: %s\n", strerror( errno ) );
				return 0;
			}

			if ( ( pid = safeatoi( fpid ) ) < 2 ) {
				snprintf( err, errlen, "Server process ID is invalid.\n" );
				return 0;
			}

			//Do we go until it's dead?
			if ( kill( pid, SIGKILL ) == -1 ) {
				snprintf( err, errlen, "Could not kill process %d: %s", pid, strerror( errno ) );
				return 0;
			}

			if ( close( fd ) == -1 ) {
				snprintf( err, errlen, "Could not close file %s: %s", fname, strerror( errno ) );
				return 0;
			}

			if ( remove( fname ) == -1 ) {
				snprintf( err, errlen, "Could not remove file %s: %s", fname, strerror( errno ) );
				return 0;
			}

			closedir( dir );	
			return 1;
		}
	}	
	
	closedir( dir );	
	snprintf( err, errlen, "No server appears to be running right now." );
	return 0;
}



/**
 * int cmd_server ( config_t *v, char *err, int errlen )
 *
 * Start a new server.
 *
 */
int cmd_server ( config_t *v, char *err, int errlen ) {

	// Define all we need
	struct sockaddr_in sa, *si = &sa;
	//short unsigned int port = v->port, *pport = &port;
	//short unsigned int *pport = NULL;
	//int server.fd = 0;
	//int backlog = BACKLOG;
	int on = 1;

	// Initialize the server structure. 
	server_t server;
	memset( &server, 0, sizeof( server_t ) );
	server.interrupt = 0;
	server.max_per = !v->maxper ? LUX_MAX_CONN_COUNT : v->maxper;
	server.timeout = 30;
	server.ttimeout = 60;
	server.rtimeout = 30;
	server.wtimeout = 30;
	server.fd = -1;
	server.data = NULL;
	server.fdset = NULL;
	server.port = v->port;
	server.backlog = LUX_BACKLOG;
	server.wbuffer = !v->wbuffer ? 1024 : v->wbuffer;
	server.filters = http_filters;
	short unsigned int port = server.port, *pport = &port;
	#ifdef DEBUG_H
	server.tapout = v->tapout;
	#endif

	// TODO: The logger should open differently based on the URI given
	server.logger = &loggers[ 1 ];

	// Check that the protocol choice is valid and set a reference
	if ( v->userprotocol == UP_HTTP )
		server.ctx = &sr[ (int)UP_HTTP ];
	else if ( v->userprotocol == UP_HTTPS )
		server.ctx = &sr[ (int)UP_HTTPS ];
	else {
		snprintf( err, errlen, "Invalid protocol specified...\n" );
		return 0;
	}

	//Die if config is null or file not there
	if ( !( server.conffile = v->config ) ) {
		snprintf( err, errlen, "No server configuration file specified...\n" );
		return 0;
	}

	//Build the server configuration if possible
	//TODO: move 'build_server_config' to server.c
	if ( !( server.config = build_server_config( server.conffile, err, errlen ) ) ) {
		return 0;
	}

	//Initialize server protocol
	if ( !server.ctx->init( &server ) ) {
		free_server_config( server.config );
		snprintf( err, errlen, "Initializing protocol '%s' failed: %s\n", server.ctx->name, server.err );
		return 0;
	}

	// Open the logging agent
	if ( !server.logger->open( v->logfile, (void *)&server.logger->data, server.err, sizeof( server.err ) ) ) {
		free_server_config( server.config );
		snprintf( err, errlen, "Failed to open log file: %s", server.err );
		return 0;
	}
	
	// Setup and open a TCP socket
	si->sin_family = PF_INET;
	si->sin_port = htons( *pport );
	(&si->sin_addr)->s_addr = htonl( INADDR_ANY );

	if (( fdset[0] = server.fd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP )) == -1 ) {
		snprintf( err, errlen, "Couldn't open socket! Error: %s\n", strerror( errno ) );
		fprintf( logfd, "%s", err );
		return 0;
	}

	if ( setsockopt( server.fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on) ) == -1 ) {
		snprintf( err, errlen, "Couldn't set socket to reusable! Error: %s\n", strerror( errno ) );
		fprintf( logfd, "%s", err );
		return 0;
	}

  #if 0
	//This may only be valid via BSD
	if ( setsockopt( server.fd, SOL_SOCKET, SO_NOSIGPIPE, (char *)&on, sizeof(on) ) == -1 ) {
		snprintf( err, errlen, "Couldn't set socket sigpipe behavior! Error: %s\n", strerror( errno ) );
		fprintf( logfd, "%s", err );
		return 0;
	}
  #endif

  #if 1
	if ( fcntl( server.fd, F_SETFD, O_NONBLOCK ) == -1 ) {
		snprintf( err, errlen, "fcntl error: %s\n", strerror(errno) );
		fprintf( logfd, "%s", err );
		return 0;
	}
  #else
	// One of these two should set non blocking functionality
	if ( ioctl( server.fd, FIONBIO, (char *)&on ) == -1 ) {
		snprintf( err, errlen, "fcntl error: %s\n", strerror(errno) );
		fprintf( logfd, "%s", err );
		return 0;
	}
  #endif

	if ( bind( server.fd, (struct sockaddr *)si, sizeof(struct sockaddr_in)) == -1 ) {
		snprintf( err, errlen, "Couldn't bind socket to address! Error: %s\n", strerror( errno ) );
		fprintf( logfd, "%s", err );
		return 0;
	}

	if ( listen( server.fd, server.backlog ) == -1 ) {
		snprintf( err, errlen, "Couldn't listen for connections! Error: %s\n", strerror( errno ) );
		fprintf( logfd, "%s", err );
		return 0;
	}

#if 0
	//Drop privileges
	if ( !revoke_priv( v, err, errlen ) ) {
		return 0;
	}

	//Write a PID file
	if ( !v->fork ) {
		//Record the PID somewhere
		int len, fd = 0;
		char buf[64] = { 0 };

		//Would this ever return zero?
		v->pid = getpid();

		if ( ( fd = open( v->pidfile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR ) ) == -1 ) {
			eprintf( "Failed to access PID file: %s.", strerror(errno));
			return 0;
		}

		len = snprintf( buf, 63, "%d", v->pid );

		//Write the pid down
		if ( write( fd, buf, len ) == -1 ) {
			eprintf( "Failed to log PID: %s.", strerror(errno));
			return 0;
		}
	
		//The parent exited successfully.
		if ( close(fd) == -1 ) {
			eprintf( "Could not close parent socket: %s", strerror(errno));
			return 0;
		}
	}
#endif
	
	//TODO: Using threads may make this easier... https://www.geeksforgeeks.org/zombie-processes-prevention/
	if ( signal( SIGCHLD, SIG_IGN ) == SIG_ERR ) {
		snprintf( err, errlen, "Failed to set SIGCHLD\n" );
		fprintf( logfd, "%s", err );
		return 0;
	}

	//Needed for lots of send() activity
	if ( signal( SIGPIPE, SIG_IGN ) == SIG_ERR ) {
		snprintf( err, errlen, "Failed to set SIGPIPE\n" );
		fprintf( logfd, "%s", err );
		return 0;
	}

	#if 0
	if ( signal( SIGSEGV, SIG_IGN ) == SIG_ERR ) {
		snprintf( err, errlen, "Failed to set SIGSEGV\n" );
		fprintf( logfd, "%s", err );
		return 0;
	}
	#endif

	// Evaluate server mode
	if ( v->model == SERVER_ONESHOT )
		srv_single( &server );
	else if ( v->model == SERVER_MULTITHREAD ) {
		srv_multithread( &server );
	}

	if ( close( server.fd ) == -1 ) {
		FPRINTF( "FAILURE: Couldn't close parent socket. Error: %s\n", err );
		return 0;
	}

	//TODO: Free whatever was allocated at ctx->init()
	server.ctx->free( &server );
	free_server_config( server.config );
	return 1;
}



#if 1
// Previously was able to use shared objects, very unsure about this
int cmd_libs( config_t *v, char *err, int errlen ) {
	//Define
	DIR *dir;
	struct dirent *d;
	int findex = 0;
	const char *appn = "filter";
	const char *libn = "libname";

	//Find the last index
	for ( filter_t *f = http_filters; f->name; f++, findex++ );

	//Open directory
	if ( !( dir = opendir( v->libdir ) ) ) {
		snprintf( err, errlen, "lib fail: %s", strerror( errno ) );
		return 0;
	}

	//List whatever directory
	for ( ; ( d = readdir( dir ) );  ) {
		void *lib = NULL;
		filter_t *f = &http_filters[ findex ];
		char fpath[2048] = {0};

		//Skip '.' & '..', and stop if you can't open it...
		if ( *d->d_name == '.' ) {
			continue;
		}

		//Try to open the library
		snprintf( fpath, sizeof( fpath ) - 1, "%s/%s", v->libdir, d->d_name );
		if ( ( lib = dlopen( fpath, RTLD_NOW ) ) == NULL ) {
			fprintf( stderr, "dlopen error: %s\n", strerror( errno ) );
			continue;
		}

		//Look for the symbol 'libname'
		if ( !( f->name = (const char *)dlsym( lib, libn ) ) ) {
			fprintf( stderr, "dlsym libname error: %s\n", dlerror() );
			//Don't open, don't load, and close what's there...
			dlclose( lib );
			continue;
		}

		//Look for the symbol 'app'
		if ( !( f->filter = dlsym( lib, appn ) ) ) {
			fprintf( stderr, "dlsym app error: %s\n", dlerror() );
			dlclose( lib );
			continue;
		}

		//Move to the next
		findex++;
	}

	return 1;
}
#endif



/**
 * int cmd_dump( config_t *v, char *err, int errlen )
 *
 * Dump the user's command line.
 *
 */
int cmd_dump( config_t *v, char *err, int errlen ) {
	int isLuaEnabled = 0;
	fprintf( stderr, "%s is running with the following settings.\n", NAME );
	fprintf( stderr, "===============\n" );
	fprintf( stderr, "Port:                %d\n", v->port );
	fprintf( stderr, "User:                %s (%d)\n", v->user, v->uid );
	fprintf( stderr, "Group:               %s (%d)\n", v->group, v->gid );
	fprintf( stderr, "Server config File:  %s\n", v->config );
	fprintf( stderr, "PID file:            %s\n", v->pidfile );
	fprintf( stderr, "Config Directory:    %s\n", CONFDIR );
	fprintf( stderr, "Share Directory:     %s\n", SHAREDIR );
	fprintf( stderr, "Session DB:          %s\n", SESSION_DB_PATH );
	fprintf( stderr, "Max threads per:     %d\n", LUX_MAX_THREADS );
	fprintf( stderr, "Max conn per:        %d\n", !v->maxper ? v->maxper : LUX_MAX_CONN_COUNT );
	fprintf( stderr, "Backlog allowance:   %d\n", LUX_BACKLOG );
	fprintf( stderr, "Model:               %s\n",
		v->model == SERVER_ONESHOT ? "oneshot" : "multithread" );
	fprintf( stderr, "Default Protocol:    %s\n",
		v->userprotocol == UP_HTTP ? "http" : "https" );
	//fprintf( stderr, "Daemonized:          %s\n", v->fork ? "T" : "F" );
	//fprintf( stderr, "Library Directory:   %s\n", v->libdir );

	fprintf( stderr, "Filters enabled:     " );
	for ( filter_t *f = http_filters; f->name; f++ ) {
		fprintf( stderr, "%s (%p), ", f->name, f->filter );
	}
	fprintf( stderr, "\n" );

	for ( filter_t *f = http_filters; f->name; f++ ) {
		if ( strcmp( f->name, "lua" ) == 0 ) {
			fprintf( stderr, "Lua modules enabled: " );
			for ( struct lua_fset *f = functions; f->namespace; f++ ) {
				f->functions ? fprintf( stderr, "%s, ", f->namespace ) : 0;
			}
			fprintf( stderr, "\n" );
			break;
		}
	}

#ifdef DEBUG_H
	if ( 0 ) {
#ifndef DISABLE_TLS
		fprintf( stderr, "GnuTLS supported ciphersuites\n" );
		fprintf( stderr, "=============================\n" );
		// TODO: output to buffer so that we can control formatting and print to one line
		for ( const gnutls_cipher_algorithm_t *cip = gnutls_cipher_list(); cip && *cip; cip++ ) {
			const char *cname = gnutls_cipher_get_name( *cip );
			fprintf( stderr, "%s\n", cname );
		}
#endif
	}
#endif
	return 1;
}



int main ( int argc, char *argv[] ) {

	// Define
	char err[ 2048 ] = { 0 };
	char v_groupname[ 128 ];
	char v_username[ 128 ];
	int *port = NULL;

	// Initialize
	logfd = stderr;

	if ( argc < 2 ) {
		fprintf( stderr, NAME ":\n%s\n", HELP );
		return 1;
	}
	
	config_t v = {
	  .dump = 0
	,	.fork = 0
	,	.gid = -1
	,	.kill = 0
	,	.maxper = 0
	,	.model = SERVER_MULTITHREAD
	,	.pid = 0
	,	.port = 80
	,	.start = 0
	,	.uid = -1
	,	.userprotocol = UP_HTTP
	,	.verbose = 0
	, .config = NULL
	, .group = "nobody"
	, .libdir = LIBDIR
	, .logfile = ERROR_LOGDB
	, .pidfile = NULL
	, .user = "nobody"
	#if 1
	, .deffilter = NULL
	, .hostname = NULL
	, .ipaddr = NULL
	, .wwwroot = NULL
	#endif
	#ifdef DEBUG_H
	,	.tapout = 32
	#endif
	};

	// Skip the first argument...
	for ( argv++; *argv; ) {

		// Stop on non-argument options
		if ( *( *argv ) != '-' )
			return EXITPRINTF( 1, NAME ": Got unknown argument %s!\n", *argv );
		else if ( EVALARG( *argv, "-v", "--verbose" ) )
			v.verbose = 1;
		else if ( EVALARG( *argv, "-s", "--start" ) )
			v.start = 1;
		else if ( EVALARG( *argv, "-k", "--kill" ) )
			v.kill = 1;
		else if ( EVALARG( *argv, "-x", "--dump" ) )
			v.dump = 1;
		else if ( EVALARG( *argv, "-c", "--configuration" ) && !SAVEARG( argv, v.config ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --configuration." );	
		else if ( !strcmp( *argv, "--pidfile" ) && !SAVEARG( argv, v.pidfile ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --pidfile." );	
		else if ( EVALARG( *argv, "-g", "--group" ) && !SAVEARG( argv, v.group ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --group." );	
		else if ( EVALARG( *argv, "-u", "--user" ) && !SAVEARG( argv, v.user ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --user." );	
		else if ( EVALARG( *argv, "-L", "--logfile" ) && !SAVEARG( argv, v.logfile ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --logfile." );	
	#if 1
		else if ( EVALARG( *argv, "-r", "--wwwroot" ) && !SAVEARG( argv, v.wwwroot ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --wwwroot." );	
		else if ( EVALARG( *argv, "-F", "--default-filter" ) && !SAVEARG( argv, v.deffilter ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --default-filter." );	
		else if ( EVALARG( *argv, "-t", "--hostname" ) && !SAVEARG( argv, v.hostname) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --hostname." );	
		else if ( EVALARG( *argv, "-I", "--ip-address" ) && !SAVEARG( argv, v.ipaddr ) )
			return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --ip-address." );	
	#endif
		else if ( !strcasecmp( *argv, "--model=threaded" ) )
			v.model = SERVER_MULTITHREAD;
		else if ( !strcasecmp( *argv, "--model=oneshot" ) )
			v.model = SERVER_ONESHOT;
		else if ( !strcasecmp( *argv, "--protocol=http" ) )
			v.userprotocol = UP_HTTP;
		else if ( !strcasecmp( *argv, "--protocol=https" ) )
			v.userprotocol = UP_HTTPS;
		else if ( EVALARG( *argv, "-p", "--port" ) ) {
			if ( ! *( ++argv ) )
				return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --port." );	
			for ( char *k = *argv; *k; k++ ) {
				if ( !memchr( "0123456789", *k, 10 ) ) {
					return EXITPRINTF( 1, NAME ": %s\n", "Argument given to --port must be numeric" );	
				}
			}
			v.port = atoi( *argv );
		}
		else if ( EVALARG( *argv, "-W", "--wbuffer" ) ) {
			if ( ! *( ++argv ) )
				return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --wbuffer." );	

			for ( char *k = *argv; *k; k++ ) {
				if ( !memchr( "0123456789", *k, 10 ) ) {
					return EXITPRINTF( 1, NAME ": %s\n", "Argument given to --wbuffer must be numeric" );	
				}
			}

			// TODO: Shouldn't hard code the upper limit (1GB max here... and that's a lot...)
			if ( ( v.wbuffer = atoi( *argv ) ) < 0 || v.wbuffer > 10485760 ) {
				return EXITPRINTF( 1, NAME ": Requested write buffer %d is invalid\n", v.wbuffer );	
			}
		}
		else if ( !strcmp( *argv, "-M" ) || !strcmp( *argv, "--max-connections" ) ) {
			if ( ! *( ++argv ) )
				return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --max-connections." );	
			for ( char *k = *argv; *k; k++ ) {
				if ( !memchr( "0123456789", *k, 10 ) ) {
					return EXITPRINTF( 1, NAME ": %s\n", "Argument given to --max-connections must be numeric" );	
				}
			}
			v.maxper = atoi( *argv );
		}
		else if ( !strcmp( *argv, "-T" ) || !strcmp( *argv, "--max-threads" ) ) {
			if ( ! *( ++argv ) )
				return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --max-threads." );	
			for ( char *k = *argv; *k; k++ ) {
				if ( !memchr( "0123456789", *k, 10 ) ) {
					return EXITPRINTF( 1, NAME ": %s\n", "Argument given to --max-threads must be numeric" );	
				}
			}
			v.maxthr = atoi( *argv );
		}
		else if ( !strcmp( *argv, "-B" ) || !strcmp( *argv, "--backlog" ) ) {
			if ( ! *( ++argv ) )
				return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --backlog." );	
			for ( char *k = *argv; *k; k++ ) {
				if ( !memchr( "0123456789", *k, 10 ) ) {
					return EXITPRINTF( 1, NAME ": %s\n", "Argument given to --backlog must be numeric" );	
				}
			}
			v.backlog = atoi( *argv );
		}
	#ifdef DEBUG_H
		else if ( !strcmp( *argv, "--tapout" ) ) {
			if ( ! *( ++argv ) )
				return EXITPRINTF( 1, NAME ": %s\n", "Argument required for --tapout." );	
			for ( char *k = *argv; *k; k++ ) {
				if ( !memchr( "0123456789", *k, 10 ) ) {
					return EXITPRINTF( 1, NAME ": %s\n", "Argument given to --tapout must be numeric" );	
				}
			}
			v.tapout = atoi( *argv );
		}
	#endif
	#if 0
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--daemonize" ) )
			v.fork = 1;
		else if ( !strcmp( *argv, "-l" ) || !strcmp( *argv, "--libs" ) ) {
			argv++;
			if ( !*argv ) {
				eprintf( "Expected argument for --libs!" );
				return 0;
			}
			snprintf( v.libdir, sizeof( v.libdir ) - 1, "%s", *argv );	
		}
		else if ( ac > argc ) {
			return eprintf( "Got unexpected argument: '%s'\n", *argv );
		}
	#endif
		else if ( EVALARG( *argv, "-V", "--version" ) ) {
			fprintf( stdout, NAME ": %s\n", PACKAGE_VERSION );
			return 0;
		}

		argv++;
	}

	// Register SIGINT
	signal( SIGINT, sigkill );

	// Set all of the socket stuff
	if ( !v.port ) {
		v.port = defport;
	}

	// Set a default user and group
	if ( !v.user ) {
		snprintf( v_username, sizeof( v_username ) - 1, "%s", getpwuid( getuid() )->pw_name );
		v.user = v_username;
		v.uid = getuid();
	}
	else if ( strchr( v.user, ':' ) ) {
		char *u = strchr( v.user, ':' );
		*u = '\0', v.user = v.user, v.group = ++u;
	}

	if ( !v.group ) {
		//v.group = getpwuid( getuid() )->pw_gid ;
		v.gid = getgid();
		v.group = v_groupname;
		snprintf( v.group, sizeof( v.group ) - 1, "%s", getpwuid( getuid() )->pw_name );
	}

#if 0
	// Load shared libraries
	if ( !cmd_libs( &v, err, sizeof( err ) ) ) {
		eprintf( "%s", err );
		return 1;
	}
#endif

	// Dump the configuration if necessary
	if ( v.dump ) {
		cmd_dump( &v, err, sizeof( err ) );		
	}

	// Start a server
	if ( v.start ) {

		// Pull in a configuration
		if ( v.config ) {
			//fprintf( stderr, NAME ": No configuration specified.\n" );
			fprintf( stderr, NAME ": config specified, stop.\n" );
			
			// If this was specified and you can't find or access the file, stop
			if ( access( v.config, R_OK ) == -1 ) {
				fprintf( stderr, NAME ": configuration file error: %s.\n", strerror( errno ) );
				return 1;
			}	
		}
		else {
			fprintf( stderr, NAME ": config not specified, stop.\n" );

		#if 0
			// sconfig and ONE lconfig have to be filled in
			// struct lconfig
			char *name;	
			char *alias;
			char *dir;	
			char *filter;	
			char *root_default;	
			char *cert_file;
			char *key_file;
			int *tlserror;
			int tlsready;

			// struct sconfig
			char *wwwroot;
			zTable *src;
			struct lconfig **hosts;
		#endif

			// If the instance directory is not specified, die immediately
			if ( !v.idir ) {
				fprintf( stderr, NAME ": directory not specified!  (Try using the --directory flag).\n" ); 
				return 1;
			}

			// If the filter is not specified, die immediately
			if ( !v.deffilter ) {
				fprintf( stderr, NAME ": filter not specified!  (Try using the -F option with an available filter name ).\n" ); 
				return 1;
			}

			// If name isn't specified, also die immediately
			if ( !v.hostname ) {
				fprintf( stderr, NAME ": domai nname not specified!  (Try using the --name flag).\n" ); 
				return 1;
			}

			// If root default isn't specified and filter is static, die immediately.
			if ( !v.rootdef && strcmp( v.deffilter, "static" ) ) {
				fprintf( stderr, NAME ": root default not specified! (Try --root-default <dir>).\n" ); 
				return 1;
			}

			// If the root isn't specified, then the root should be the current directory
			if ( !v.wwwroot ) {
				char cwd[ PATH_MAX ];
				if ( !getcwd( cwd, PATH_MAX - 1 ) ) {
					fprintf( stderr, NAME ": failed to fetch working directory: %s.\n", strerror( errno ) ); 
					return 1;
				}
				v.wwwroot = strdup( cwd );
			}

			// Finally, check if TLS is needed
		}

//exit(0);

		#if 0
		//TODO: Set pid file if one is not set.  Will also need a --no-pid option.
		if ( !v.pidfile ) {
			struct timespec t;
			clock_gettime( CLOCK_REALTIME, &t );
			unsigned long time = t.tv_nsec % 3333;
			snprintf( v.pidfile, sizeof( v.pidfile ) - 1, "%s/%s-%ld", PIDDIR, NAME, time );
		}
		#endif

		// Set the process ID
		v.pid = getpid();

		// Since daemonization is not enabled right now, just write the PID file (and exit if you can't)
		if ( v.pidfile && !write_pid( v.pid, v.pidfile, err, sizeof( err ) ) ) {
			fprintf( stderr, NAME ": pid access error: %s", err );
			return 0;
		}

		if ( !cmd_server( &v, err, sizeof(err) ) ) {
			fprintf( stderr, NAME ": server failure: %s", err );
			return 1;
		}
	}


	// Take down a running server
	if ( v.kill ) {
		if ( !cmd_kill( &v, err, sizeof( err ) ) ) {
			fprintf( stderr, NAME ": server takedown failure: %s", err );
			return 1;
		}
	}

	return 0;
}
