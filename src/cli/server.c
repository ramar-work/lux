/* -------------------------------------------------------- *
 * server.c
 * ========
 * 
 * Summary 
 * -------
 * hypno's web server.
 * 
 * Usage
 * -----
 * These are the options:
 *   --start            Start new servers             
 *   --debug            Set debug rules               
 *   --kill             Test killing a server         
 *   --fork             Daemonize the server          
 *   --configuration <arg>     Use this file for configuration
 *   --port <arg>       Set a differnt port           
 *   --user <arg>       Choose a user to start as     
 * 
 * LICENSE
 * -------
 * Copyright 2020-2021 Tubular Modular Inc. dba Collins Design
 * 
 * See LICENSE in the top-level directory for more information.
 *
 * CHANGELOG 
 * ---------
 * - 
 *  
 * -------------------------------------------------------- */
#include <zwalker.h>
#include <ztable.h>
#include <dlfcn.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <pthread.h>
#include "../config.h"
#include "../logging/log.h"
#include "../server/server.h"
#if 0
#include "../filters/filter-static.h"
#include "../filters/filter-dirent.h"
#include "../filters/filter-redirect.h"
#endif
#include "../filters/filter-echo.h"
#include "../filters/filter-lua.h"
#include "../ctx/ctx-http.h"
#include "cliutils.h"

// New server types
#include "../server/multithread.h"
#include "../server/single.h"


#ifndef DISABLE_TLS
 #include "../ctx/ctx-https.h"
#endif

#ifndef NO_DNS_SUPPORT
 //#include "../ctx/ctx-dns.h"
#endif

#ifndef PIDDIR
 #define PIDDIR "/var/run/"
#endif

#define NAME "hypno"

#define LIBDIR "/var/lib/" NAME

#define PIDFILE PIDDIR NAME ".pid"

#define REAPING_THREADS

#define HELP \
	"-s, --start               Start the server\n" \
	"-k, --kill                Kill a running server\n" \
	"-c, --configuration <arg> Use this Lua file for configuration\n" \
	"-p, --port <arg>          Start using a different port \n" \
	"    --pid-file <arg>      Define a PID file\n" \
	"-u, --user <arg>          Choose an alternate user to run as\n" \
	"-g, --group <arg>         Choose an alternate group to run as\n" \
	"-x, --dump                Dump configuration at startup\n" \
	"-l, --log-file <arg>      Define an alternate log file location\n" \
	"-a, --access-file <arg>   Define an alternate access file location\n" \
	"-V, --version             Show version information and quit.\n" \
	"-h, --help                Show the help menu.\n"

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

#define eprintf(...) \
	( \
	fprintf( stderr, "%s: ", NAME "-server" ) && \
	fprintf( stderr, __VA_ARGS__ ) && \
	fprintf( stderr, "\n" ) \
	) ? 0 : 0

const int defport = DEFAULT_WWW_PORT;

char pidbuf[128] = {0};

FILE * logfd = NULL, * accessfd = NULL;

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



struct values {
	int port;
	pid_t pid;
	int start;
	int kill;
	int fork;
	int dump;
	int uid, gid;
	char user[ 128 ];
	char group[ 128 ];
	char config[ PATH_MAX ];
	char logfile[ PATH_MAX ];
	char accessfile[ PATH_MAX ];
	char libdir[ PATH_MAX ];
	char pidfile[ PATH_MAX ];
	model_t model;
	userprotocol_t userprotocol;
	short int maxper;
#ifdef DEBUG_H
	int tapout;
#endif
} v = {
	.port = 80
,	.pid = 0
,	.start = 0
,	.kill = 0
,	.fork = 0
,	.dump = 0
,	.uid = -1
,	.gid = -1
,	.model = SERVER_MULTITHREAD
,	.userprotocol = UP_HTTP
,	.maxper = 0
#ifdef DEBUG_H
,	.tapout = 32
#endif
};



filter_t http_filters[] = { 
#if 0
	{ "static", filter_static }
,	{ "dirent", filter_dirent }
,	{ "redirect", filter_redirect }
#endif
  { "lua", filter_lua }
, { "echo", filter_echo }
, { NULL }
#if 0
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
, { NULL }
, { NULL }
#endif
};



protocol_t sr[] = {
	{ "http", read_notls, write_notls, create_notls, free_notls, pre_notls, post_notls },
	{ "https", read_gnutls, write_gnutls, create_gnutls, free_gnutls, pre_gnutls, post_gnutls },
#if 0
	{ "dns", read_dns, write_dns, create_dns, NULL, pre_dns, post_dns },
	{ "rtmp", read_rtmp, write_rtmp, create_rtmp, NULL, pre_rtmp, post_rtmp },
#endif
	{ NULL }
};



struct log loggers[] = {
	{ f_open, f_close, f_write, f_handler }
,	{ sqlite3_log_open, sqlite3_log_close, sqlite3_log_write, sqlite3_handler }
};



// We can drop privileges permanently
int revoke_priv ( struct values *v, char *err, int errlen ) {
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



// Write the process ID
int write_pid( int pid, char *pidfile ) {

	char buf[ 64 ] = { 0 };
	int fd = -1, len = snprintf( buf, 63, "%d", pid );

	if ( !pidfile ) {
		return eprintf( "No PID file specified." );
	}

	if ( ( fd = open( pidfile, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR ) ) == -1 ) {
		return eprintf( "Failed to access PID file: %s.", strerror(errno));
	}

	//Write the pid down
	if ( write( fd, buf, len ) == -1 ) {
		return eprintf( "Failed to log PID: %s.", strerror(errno));
	}

	//The parent exited successfully.
	if ( close(fd) == -1 ) {
		return eprintf( "Could not close PID file: %s", strerror(errno));
	}

	return 1;
}



// A sigkill handler
void sigkill( int signum ) {
	fprintf( stderr, "Received SIGKILL - Killing the server...\n" );
	char err[ 2048 ] = {0};

#if 0
	//TODO: Join (and kill) all the open threads (could take a while)
	for ( ; ; ) {
		//
	}
#endif

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

	//TODO: Add in write detection versus just closing arbitrarily
	if ( accessfd ) {
		fclose( accessfd ); 
		accessfd = stderr;
	}

	if ( logfd ) {
		fclose( logfd );
		logfd = stderr;
	}

	//cmd_kill( NULL, err, sizeof( err ) );
}






// Use the command line to kill a running server
int cmd_kill ( struct values *v, char *err, int errlen ) {
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

		if ( memstrat( d->d_name, "hypno-", strlen( d->d_name ) ) > -1 ) {
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



// Run a server
int cmd_server ( struct values *v, char *err, int errlen ) {

	// Define all we need
	struct sockaddr_in sa, *si = &sa;
	//short unsigned int port = v->port, *pport = &port;
	//short unsigned int *pport = NULL;
	//int server.fd = 0;
	//int backlog = BACKLOG;
	int on = 1;

	// Prep this
	server_t server; 
	server.interrupt = 0;
	server.max_per = !v->maxper ? 64 : v->maxper;
	server.timeout = 30;
	server.ttimeout = 60;
	server.rtimeout = 30;
	server.wtimeout = 30;
	server.fd = -1;
	server.data = NULL;
	server.fdset = NULL;
	server.port = v->port; 
	server.backlog = BACKLOG;
	server.filters = http_filters;
	short unsigned int port = server.port, *pport = &port;
	#ifdef DEBUG_H
	server.tapout = v->tapout;
	#endif

	// Check that the protocol choice is valid and set a reference
	if ( v->userprotocol == UP_HTTP )
		server.ctx = &sr[ (int)UP_HTTP ];
	else if ( v->userprotocol == UP_HTTPS )
		server.ctx = &sr[ (int)UP_HTTPS ];
	else {
		snprintf( err, errlen, "Invalid protocol specified...\n" );
		return 0;
	}
	 

	// Logging functions are now a context of their own.
	// But locking will most likely be needed, so either
	// set those functions here, or only let the parent have
	// control. 
	// server.log = -1;
	// server.access = -1;

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

  #if 0
	//Init logging and access structures too
	struct log *al = &loggers[ 0 ], *el = &loggers[ 0 ];
	
	if ( !el->open( v->logfile, &el->data ) ) {
		snprintf( err, errlen, "Could not open error log handle at %s - %s\n", v->logfile, el->handler() );
		return 0;
	}
  #else
	// Open a log file here
	if ( !( logfd = fopen( v->logfile, "a" ) ) ) {
		return eprintf( "Couldn't open error log file at: %s...\n", v->logfile );
	}

	// Set reference
	server.log_fd = logfd;
  #endif

  #if 0
	if ( !al->open( v->accessfile, &al->data ) ) {
		snprintf( err, errlen, "Could not open access log handle at %s - %s\n", v->accessfile, al->handler() );
		return 0;
	}
  #else
	// Open an access file too 
	if ( !( accessfd = fopen( v->accessfile, "a" ) ) ) {
		return eprintf( "Couldn't open access log file at: %s...\n", v->accessfile );
	}

	// Set reference
	server.access_fd = accessfd;
  #endif

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
	
	//todo: uSing threads may make this easier... https://www.geeksforgeeks.org/zombie-processes-prevention/
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

	if ( accessfd ) {
		fclose( accessfd ); 
		accessfd = NULL;
	}

	if ( logfd ) {
		fclose( logfd );
		logfd = NULL;
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



#if 0
// Previously was able to use shared objects, very unsure about this
int cmd_libs( struct values *v, char *err, int errlen ) {
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



// Dump the user's command line 
int cmd_dump( struct values *v, char *err, int errlen ) {
	int isLuaEnabled = 0;
	fprintf( stderr, "Hypno is running with the following settings.\n" );
	fprintf( stderr, "===============\n" );
	fprintf( stderr, "Port:                %d\n", v->port );
	fprintf( stderr, "User:                %s (%d)\n", v->user, v->uid );
	fprintf( stderr, "Group:               %s (%d)\n", v->group, v->gid );
	fprintf( stderr, "Server config File:  %s\n", v->config );
	fprintf( stderr, "PID file:            %s\n", v->pidfile );
	fprintf( stderr, "Config Directory:    %s\n", CONFDIR );
	fprintf( stderr, "Share Directory:     %s\n", SHAREDIR );
	fprintf( stderr, "Session DB:          %s\n", SESSION_DB_PATH );
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
		fprintf( stderr, "GnuTLS supported ciphersuites\n" );
		fprintf( stderr, "=============================\n" );
		// TODO: output to buffer so that we can control formatting and print to one line
		for ( const gnutls_cipher_algorithm_t *cip = gnutls_cipher_list(); cip && *cip; cip++ ) {
			const char *cname = gnutls_cipher_get_name( *cip );
			fprintf( stderr, "%s\n", cname );
		}
	}
#endif
	return 1;
}



// The main program
int main ( int argc, char *argv[] ) {
	char err[ 2048 ] = { 0 };
	int *port = NULL; 

#if 0
	struct values v = { 0 };
	v.port = 80;
	v.pid = 80;
	v.start = 0; 
	v.kill = 0 ;
	v.fork = 0 ;
	v.dump = 0 ;
	v.uid = -1 ;
	v.gid = -1 ;
#endif

	//
	snprintf( v.logfile, sizeof( v.logfile ), "%s", ERROR_LOGFILE );
	snprintf( v.accessfile, sizeof( v.accessfile ), "%s", ACCESS_LOGFILE );

	if ( argc < 2 ) {
		fprintf( stderr, HELP );
		return 1;	
	}
	
	//while ( *argv ) {
	for ( int ac = argc; *argv; argv++, argc-- ) {
		if ( strcmp( *argv, "--version" ) == 0 ) {
			fprintf( stdout, "%s\n", PACKAGE_VERSION );
			return 0;
		}

		if ( OPTEVAL( *argv, "-s", "--start" ) ) 
			v.start = 1;
		else if ( OPTEVAL( *argv, "-k", "--kill" ) ) 
			v.kill = 1;
		else if ( OPTEVAL( *argv, "-x", "--dump" ) ) 
			v.dump = 1;
		else if ( OPTEVAL( *argv, "-c", "--configuration" ) ) {
			OPTARG( *argv, "--configuration" );
			snprintf( v.config, sizeof( v.config ) - 1, "%s", *argv );	
		}
		else if ( !strcmp( *argv, "--pid-file" ) ) {
			OPTARG( *argv, "--pid-file" );
			snprintf( v.pidfile, sizeof( v.pidfile ) - 1, "%s", *argv );	
		}
		else if ( OPTEVAL( *argv, "-p", "--port" ) ) {
			OPTARG( *argv, "--port" );
			//TODO: This should be safeatoi 
			v.port = atoi( *argv );
		}
		else if ( OPTEVAL( *argv, "-g", "--group" ) ) {
			OPTARG( *argv, "--group" );
			snprintf( v.group, sizeof( v.group ) - 1, "%s", *argv );	
		}
		else if ( OPTEVAL( *argv, "-u", "--user" ) ) {
			OPTARG( *argv, "--user" );
			snprintf( v.user, sizeof( v.user ) - 1, "%s", *argv );	
		}
		else if ( OPTEVAL( *argv, "-l", "--log-file" ) ) {
			OPTARG( *argv, "--log-file" );
			memset( v.logfile, 0, sizeof( v.logfile ) );
			snprintf( v.logfile, sizeof( v.logfile) - 1, "%s", *argv );	
		}
		else if ( OPTEVAL( *argv, "-a", "--access-file" ) ) {
			OPTARG( *argv, "--access-file" );
			memset( v.accessfile, 0, sizeof( v.accessfile ) );
			snprintf( v.accessfile, sizeof( v.accessfile) - 1, "%s", *argv );
		}
		else if ( !strcmp( *argv, "--max-per" ) ) {
			OPTARG( *argv, "--max-per" );
			//TODO: This should be safeatoi 
			v.maxper = atoi( *argv );
		}
		else if ( !strcmp( *argv, "--model=threaded" ) ) 
			v.model = SERVER_MULTITHREAD;
		else if ( !strcmp( *argv, "--model=oneshot" ) ) 
			v.model = SERVER_ONESHOT;
		else if ( !strcmp( *argv, "--protocol=http" ) ) 
			v.userprotocol = UP_HTTP;
		else if ( !strcmp( *argv, "--protocol=https" ) ) 
			v.userprotocol = UP_HTTPS;
	#ifdef DEBUG_H
		else if ( !strcmp( *argv, "--tapout" ) ) {
			OPTARG( *argv, "--tapout" );
			//TODO: This should be safeatoi 
			v.tapout = atoi( *argv );
		}
	#endif
	#if 0
		else if ( OPTEVAL( *argv, "-d", "--daemonize" ) ) 
			v.fork = 1;
		else if ( OPTEVAL( *argv, "-l", "--libs" ) ) {
			argv++;
			if ( !*argv ) {
				eprintf( "Expected argument for --libs!" );
				return 0;
			}
			snprintf( v.libdir, sizeof( v.libdir ) - 1, "%s", *argv );	
		}
	#endif
		else if ( ac > argc ) {
			return eprintf( "Got unexpected argument: '%s'\n", *argv );
		}
	}

	//Register SIGINT
	signal( SIGINT, sigkill );

	//Set all of the socket stuff
	if ( !v.port ) {
		v.port = defport;
	}

	//Set a default user and group
	if ( ! *v.user ) {
		snprintf( v.user, sizeof( v.user ) - 1, "%s", getpwuid( getuid() )->pw_name );
		v.uid = getuid();
	}

	if ( ! *v.group ) {
		//v.group = getpwuid( getuid() )->pw_gid ;
		v.gid = getgid();
		snprintf( v.group, sizeof( v.group ) - 1, "%s", getpwuid( getuid() )->pw_name );
	}

	//Open the libraries (in addition to stuff)
	if ( ! *v.libdir ) {
		snprintf( v.libdir, sizeof( v.libdir ), "%s", LIBDIR );
	}

#if 0
	//Load shared libraries
	if ( !cmd_libs( &v, err, sizeof( err ) ) ) {
		eprintf( "%s", err );
		return 1;
	}
#endif

	//Dump the configuration if necessary
	if ( v.dump ) {
		cmd_dump( &v, err, sizeof( err ) );		
	}

	//Start a server
	if ( v.start ) {
		//Pull in a configuration
		if ( ! *v.config ) {
			eprintf( "No configuration specified." );
			return 1;
		}

		#if 0
		//TODO: Set pid file if one is not set.  Will also need a --no-pid option.
		if ( ! *v.pidfile ) {
			struct timespec t;
			clock_gettime( CLOCK_REALTIME, &t );
			unsigned long time = t.tv_nsec % 3333;
			snprintf( v.pidfile, sizeof( v.pidfile ) - 1, "%s/%s-%ld", PIDDIR, NAME, time );
		}
		#endif

		// Set the process ID
		v.pid = getpid();

		// Since daemonization is not enabled right now, just write the PID file (and exit if you can't)
		if ( *v.pidfile && !write_pid( v.pid, v.pidfile ) ) {
			return 0;
		}

		if ( !cmd_server( &v, err, sizeof(err) ) ) {
			eprintf( "%s", err );
			return 1;
		}
	}

	if ( v.kill ) {
		//eprintf ( "%s\n", "--kill not yet implemented." );
		if ( !cmd_kill( &v, err, sizeof( err ) ) ) {
			eprintf( "%s", err );
			return 1;
		}
	}

	FPRINTF( "I am a child that reached the end...\n" );
	return 0;
}

