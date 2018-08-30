/* hypno.c */
#include "vendor/single.h"
#include "vendor/nw.h"
#include "vendor/http.h"
#include "bridge.h"
#include <dirent.h>

#define PROG "hypno"
#define DIRNAME_DEFAULT "example"
#define DATAFILE_NAME "data.lua"
#define ERR_SRVFORK 20
#define ERR_PIDFAIL 21
#define ERR_PIDWRFL 22 
#define ERR_SSOCKET 23 
#define ERR_INITCON 24 
#define ERR_SKLOOPS 25 
#define SUC_PARENT  27
#define SUC_CHILD   28

//Add timing functions
#define INCLUDE_TIMING_INFO_H

//Testing: See line numbers in error messages
//#define INCLUDE_LINE_NO_ERROR_H ERR_SUPERV                 

//Testing: I'm going to include an EXAMPLE define that uses the 'example' folder.
#define EXAMPLE_H

//Testing: Option module is not so wonderful yet, so use this to test out things having to do with it.
#define TESTOPTS_H                   

//This define controls whether or not line numbers will also be included in the error message.
#ifdef INCLUDE_LINE_NO_ERROR_H
 #define ERRL(...) \
	( snprintf( err, 24, "[ @%s(), %d ]                 ", __FUNCTION__, __LINE__ ) ? 1 : 1 \
	&& ( err += 24 ) && snprintf( err, errlen - 24, __VA_ARGS__ ) ) ? 0 : 0
#else
 #define ERRL(...) snprintf( err, errlen, __VA_ARGS__ ) ? 0 : 0
#endif

//This define should be a bit easier to use than fully calling http_err
#define ERR_500(...) http_err( r, h, 500, __VA_ARGS__ ) 
#define ERR_404(...) http_err( r, h, 404, __VA_ARGS__ ) 

//???
char default_dirname[] = DIRNAME_DEFAULT;

//Lua structure
typedef struct {
	char *name; 
	lua_CFunction func; 
	char *setname; 
	int sentinel;
} luaCF;

//Signature for handling a server.
_Bool http_run (Recvr *r, void *p, char *e);
_Bool http_send (Recvr *r, void *p, char *e);

//This is nw's data structure for handling protocols 
Executor etc[] = {
	[NW_AT_READ]        = { .exe = http_read, NW_NOTHING },
	[NW_AT_PROC]        = { .exe = http_run , NW_NOTHING },
	[NW_AT_WRITE]       = { .exe = http_send, NW_NOTHING },
	[NW_COMPLETED]      = { .exe = http_fin , NW_NOTHING }
};

//Lua functions
luaCF lua_functions[] =
{
	{ .setname = "set1" },
		{ "abc", abc },
		{ "val", abc },
		{ .sentinel = 1 },

	{ .setname = "set2" },
		{ "xyz", abc },
		{ "def", abc },
		{ .sentinel = 1 },
#if 0
	/*Database module*/	
	{ .setname = "db" },
		{ "exec",   exec_db },
		{ "schema", schema_db },
		{ "check",  check_table },
		{ .sentinel = 1 },
#endif
	/*Render module*/	
	{ .setname = "render" },
		{ "file",   abc },
		{ .sentinel = 1 },

	/*End this*/
	{ .sentinel = -1 },
};

//Useful variables for main()
static const char pidfile[] = "hypno.pid";
static char err[ 4096 ] = { 0 };
static const int errlen = 4096; 
static luaCF *rg = lua_functions;

//Serve a list of files
void srvListOfFiles() {
#if 0
		if (S_ISREG((sb.st_mode))) 
			fprintf( stderr, "%10s", "file:" );
		else if (S_ISDIR((sb.st_mode)))
			fprintf( stderr, "%10s", "dir:" );
		else if (S_ISCHR((sb.st_mode))) 
			fprintf( stderr, "%10s", "char:" );
		else if (S_ISBLK((sb.st_mode))) 
			fprintf( stderr, "%10s", "block:" );
		else if (S_ISFIFO((sb.st_mode))) 
			fprintf( stderr, "%10s", "felse ifo:" );
		else if (S_ISLNK((sb.st_mode))) 
			fprintf( stderr, "%10s", "link:" );
		else if (S_ISSOCK((sb.st_mode))) {
			fprintf( stderr, "%10s", "socket:" );
		}
#endif
}


//Serve binary files
void srvBinaryFiles() {
	#if 0
	//Binary data using server stuff
	#include "tests/why_tif.c"
	http_set_status( h, 200 );
	http_set_content( h, "image/tiff", ( uint8_t * )why_tif, why_tif_len );
	http_pack_response( h );
	#endif
}


//Serve a basic canned response
void srvBasicCannedResponse() {
  #if 0 
	//Static response, with no help from the server.
	const char resp[] = 
		"HTTP/2.0 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 23\r\n\r\n"
		"<h2>Hello, world!</h2>\n";
	http_set_status( h, 200 );
	http_set_content( h, "text/html", ( uint8_t * )resp, strlen( resp ) );
	#endif
	#if 0
	//Static response leaning on the server
	const char resp[] = 
		"<h2>Hello, world!</h2>\n";
	http_set_status( h, 200 );
	http_set_content( h, "text/html", ( uint8_t * )resp, strlen( resp ) );
	http_pack_response( h );
  #endif
}


typedef struct Passthru {

	//struct stat sb;
	char *webroot;
	char *activeDir;
	char *datafile ;
	DIR *ds; 
	struct dirent *de;
	short singleDir;

#if 0
	Table  routes, request, model;
	Loader ld[ 10 ], *l = NULL;
	Render ren;
	uint8_t *renbuf = NULL;
	int renbuflen = 0;
	HTTP *h = (HTTP *)r->userdata;
	HTTP_Request *req = &h->request;
	lua_State *L  = luaL_newstate(); 
#endif
	
} Passthru;


int glean_extension( char *pathname, Passthru *pt ) {
	return 1;
}


int hssent = 0;
#include "tests/char-char.c"
static uint8_t *tccf = NULL;
static int tccflen = 0, tccfpos = 0;

typedef struct {
	char *filename; //should be utf-8
	int fd;
	//really should do something with mmap() here, but...
	int pos;		
	int size;		
} HttpStreamer;

//This is what handles streaming in case it's needed.
_Bool http_send (Recvr *r, void *p, char *e) {
	HTTP *h = (HTTP *)r->userdata;
	HTTP_Response *y = &h->response;
	int ret = memstrat( y->msg, "\r\n\r\n", y->mlen );

#if 1
	//Dump Recvr and response body for debugging purposes
	fprintf( stderr, "conn #%d; %s, %d: %d (%p)\n", r->connNo, __func__, __LINE__, *r->bypass, r->bypass );
	print_recvr( r );
	http_print_response( h ) ; 
	write( 2, y->msg, y->mlen );

	//Search for the end of the message thing
	fprintf (stderr, "http header stops at: %d\n", ret ); 
	write( 2, &y->msg[ ret + 4 ], y->mlen - ( ret + 4 ) );
#endif
	
	//Stop here, because the message was never prepared correctly. 
	if ( ret == -1 )
		ERR_500( "Can't read packaged HTTP message." );	
	else {
		//add carriage return
		ret += 4;
	}

	//Set the current byte position and sent bytes
#if 1
	if ( !tccf ) {
		//this is the first time I've sent the message
		tccflen = tests_char_char_jpeg_len; 
		tccf = tests_char_char_jpeg; 
		//wipe the rest of r (so wget should only get a header or like carriage return as a message
		memset( &y->msg[ ret ], 0, y->mlen - ret );

nsprintf( "3|6 Mafia" );
npprintf( h->userdata ); 
		
		//write both recvr->msg or response.buffer and http message
		y->mlen = ret;
		fprintf( stderr, "New message:\n=================\n" );
		write( 2, y->msg, y->mlen );
		//write( 2, r->_response.buffer, y->len );
		fprintf( stderr, "Lengths:\n============\n" );
		niprintf( y->mlen );
		niprintf( bf_written( &r->_response ) );
		fprintf( stderr, "Same buffer?:\n============\n" );
		write( 2, bf_data( &r->_response ), bf_written( &r->_response ) );

		//bf_written is not being pointed to by anything
		//bf_data is pointed to by y->msg
		//how to modify directly?
			
		//write data to r	
		//bf_append( h->response, "200\r\n", strlen("200\r\n") );	
		//bf_append( h->response, tccf, 200 );	
		//tccfpos += 200;
		r->stage = NW_AT_WRITE;
	}
	else {
		fprintf( stderr, "conn #%d; %s, %d: %d (%p)\n", r->connNo, __func__, __LINE__, *r->bypass, r->bypass );
		exit( 0 );
		//write more data, unless i'm at the end...
	}
#else
	r->stage = NW_COMPLETED;
#endif

	//Open a database and start writing requests to it...
	if ( r->stage == NW_COMPLETED ) {	//r->stage = NW_AT_WRITE;
		//memset(h, 0, sizeof(HTTP));
	}
	else {

	}

	return 1;
} 


//This is the single-threaded HTTP run function
_Bool http_run ( Recvr *r, void *p, char *err ) { 
 #ifdef INCLUDE_TIMING_INFO_H
	char tbuf[1024]={0};
	Timer t;
	sprintf( tbuf,  "http_run at %s:%d\n", __FUNCTION__, __LINE__ ); 
	timer_use_us( &t );
	timer_start( &t );
 #endif	
 #if 0
	struct stat sb;
	char *ag->activeDir = NULL;
	char *datafile = NULL;
	char *fSetName = NULL;
	DIR *ds = NULL;
	struct dirent *de;
	const char *ag->webroot = "example";
 #endif

	//This is a hell of a lot of data.
	char *fSetName;
	char *datafile;
	struct stat sb;
	Table  routes, request, model;
	Loader ld[ 10 ], *l = NULL;
	Render ren;
	uint8_t *renbuf = NULL;
	int renbuflen = 0;

	//Anything that should be set or initialized is here
	Passthru *ag = (Passthru *)p;
	HTTP *h = (HTTP *)r->userdata;
	HTTP_Request *req = &h->request;
	lua_State *L = luaL_newstate(); 

	//Set the message length
	req->mlen = r->recvd;

	//Set up the "loader" structure
	memset( ld, 0, sizeof( Loader ) * 10 );

	//This should receive the entire request
	if ( !http_get_remaining( h, r->request, r->recvd ) )
		return ERR_500("Error processing request." );

	//Try to open the web root
	if ( !( ag->ds = opendir( ag->webroot ) ) )
		return ERR_500("Couldn't access web root: %s.", strerror(errno) );

#if 0
	//If the webroot is the web application directory, set things accordingly
	if ( ag->singleDir ) {
		//Make a fully qualified path from the filename
		char *fd = strcmbd( "/", ag->webroot, ag->de->d_name );

		//Check the name of the folder and see if the hostname matches
		if ( S_ISDIR(sb.st_mode) || S_ISLNK((sb.st_mode)) ) {
			if ( strcmp( ag->de->d_name, h->hostname ) == 0 ) {
				ag->activeDir = fd;
			}	
		}
	}
	else {
	}
#endif
	//fprintf( stderr, "Directory '%s' contains:\n", ag->webroot );
	while ( (ag->de = readdir( ag->ds )) ) {
		//Make a fully qualified path from the filename
		char *fd = strcmbd( "/", ag->webroot, ag->de->d_name );

		//Check that the child inode is accessible
		if ( lstat( fd, &sb ) == -1 )
			return ERR_500("Can't access directory %s: %s.", fd, strerror(errno));

		//Check the name of the folder and see if the hostname matches
		if ( S_ISDIR(sb.st_mode) || S_ISLNK((sb.st_mode)) ) {
			if ( strcmp( ag->de->d_name, h->hostname ) == 0 ) {
				ag->activeDir = fd;
				break;
			}	
		}
		free(fd);
	}

	//Default responses get handled here 
	if ( !ag->activeDir )
		return ERR_404( "No site matching hostname '%s' found.", h->hostname );

	//Lastly, check that the client isn't asking for actual files.
	int rplen = strlen( h->request.path );
	if ( memchr( h->request.path, '.', rplen ) ) {
		//Check for a known extension
		char *mt = NULL, *extck = &h->request.path[ rplen ];
		while ( extck-- && *extck != '.' ) ; 	
		extck++;

		//Now check for a valid mimetype	
		mt = (char *)mtfref( extck );
	
		//If the mimetype is not supported, that's technically a 404
		//but right now, use application/octet-stream to check for validity
		if ( strcmp( mt, "application/octet-stream" ) > 0 ) {
			//Check for the path name relative to the currently chosen directory
			uint8_t *fb = NULL;
			char *fn = strcmbd( "/", ag->activeDir, &h->request.path[ 1 ] );
			int fd = 0;

			//File not found, should return a 404
			if ( stat( fn, &sb ) == -1 ) {	
				return ERR_404( "File not found: %s", fn );
			}

			//Don't allow zero-length files...?
			if ( sb.st_size == 0 ) {
				return ERR_500( "Requested file: %s is zero length.", fn );
			}

/*
			//Allocate...
			if ( !(fb = malloc( sb.st_size )) || !memset( fb, 0, sb.st_size ) ) {
				return ERR_500( "Memory error: %s\n", strerror( errno ) );
			}
*/
			
			//Couldn't open	
			if ( (fd = open( fn, O_RDONLY )) == -1 || read( fd, fb, sb.st_size ) == -1 ) {	
				return ERR_500( "Error occurred while trying to open file: %s. %s", fn, strerror( errno ));
			}

			//Prepare the actual reponse
			http_set_status( h, 200 );
			http_set_content( h, mt, fb, sb.st_size );
			http_pack_response( h );
			free( fn );

			//a certain size needs to put the server in stream mode...
			//http_print_response( h );
			fprintf( stderr, "conn #%d; %s, %d: %d (%p)\n", r->connNo, __func__, __LINE__, *r->bypass, r->bypass );
			*r->bypass = 1;
			fprintf( stderr, "conn #%d; %s, %d: %d (%p)\n", r->connNo, __func__, __LINE__, *r->bypass, r->bypass );

			r->stage = NW_AT_WRITE;
			return 1;
		}
	}

	//Check that Lua initialized here
	if ( !L ) {
		return ERR_500("Failed to create new Lua state?" );
	}

	//Open Lua libraries.
	luaL_openlibs( L );
	lua_newtable( L );

	//Read the data file for whatever "site" is gonna be run.
	if ( !(datafile = strcmbd( "/", ag->activeDir, DATAFILE_NAME )) ) {
		return ERR_500("Low mem" );
	}

	//Always waste some time looking for the file
	if ( stat( datafile, &sb ) == -1 ) {
		return ERR_500("Couldn't find file containing site data: %s.", datafile );
	}

	//Load the route file.
	if ( !lua_load_file( L, datafile, &err ) ) {
		return ERR_500("Loading routes failed at file '%s': %s", datafile, err );
	}

	//If there is no data, then I shouldn't move forward
	char *dfbuf = NULL; 
	int dfd = open( datafile, O_RDONLY );
	dfbuf = malloc( sb.st_size );
	memset( dfbuf, 0, sb.st_size );

	if ( !dfbuf || dfd == -1 || read( dfd, dfbuf, sb.st_size ) == -1)  {
		free( dfbuf );
		return ERR_500( "Issues reading %s to buffer: %s", datafile, strerror( errno ) );
	}

	//Then check that there is something besides blank space in the file
	if ( sb.st_size == 0 )
		return ERR_500( "Datafile at %s is zero length, can't continue...\n", datafile );
	else {
		int fnd_oc = 0;
		while ( dfbuf++ ) {
			if ( *dfbuf != '\0' && *dfbuf != '\t' && *dfbuf != ' ' ) {
				fnd_oc = 1;
				break;
			}
		}
		if ( fnd_oc == 0 ) {
			return ERR_500( "Datafile at %s is blank, can't continue...\n", datafile );
		}
	}

	//Convert this to an actual table so C can work with it...
	if ( !lt_init( &routes, NULL, 666 ) || !lua_to_table( L, 2, &routes) ) {
		return ERR_500("Converting routes from file '%s' failed.", datafile );
	}

	//Clear stack to get rid of what came from routes
	lua_settop( L, 0 );

	//Add HTTP to the global space.
	lua_newtable( L );
	table_to_lua( L, 1, &h->request.table );
	lua_setglobal( L, "env" );

	//Register each of the Lua functions (TODO: every time... not good for perf)
	while ( rg->sentinel != -1 ) {
		//Set the top table
		if ( rg->sentinel == 1 ) {
			lua_setglobal( L, fSetName );
		}
		else if ( !rg->name && rg->setname ) {
			fSetName = rg->setname;
			lua_newtable( L );
		}
		else if ( rg->name ) {
			lua_pushstring( L, rg->name );
			lua_pushcfunction( L, rg->func );
			lua_settable( L, 1 );
		}
		rg++;
	}

	//Parse the routes that come off of this file
	if ( !parse_route( ld, sizeof(ld) / sizeof(Loader), h, &routes ) )
		return ERR_500("Finding the model and view for the current route failed." );

	//Reset Loader pointer and clear the stack
	l = &ld[0];
	lua_settop( L, 0 );

#if 0
	//What's in this weird little data structure thing that never works?
	while ( l->content ) {
		fprintf( stderr, "%s: %s\n", (l->type==CC_MODEL)?"model":"view",l->content );
		l++;
	}
exit( 0 );
#endif

	//Loop through all of this content	
	//l = &ld[0];
	while ( l->content ) {
		//Load each model file (which is just running via Lua)
		if ( l->type == CC_MODEL ) { 
			//Somehow have to get the root directory of the site in question...
			char *mfile = strcmbd( "/", ag->activeDir, "models", l->content, "lua" );
			mfile[ strlen(mfile) - 4 ] = '.';
			fprintf( stderr, "Attempting to load: %s\n", mfile );

			if ( stat( mfile, &sb ) == -1 )
				return ERR_500("Couldn't find model file: %s.", mfile );

			if ( luaL_dofile( L, mfile ) )
				return ERR_500("Could not load Lua file: %s. %s\n", mfile, lua_tostring(L, -1));
		}
		l++;
	}

	//Still gotta figure out the reason for that crash...
	lua_aggregate( L );
	lua_pushstring( L, "model" );
	lua_pushvalue( L, 1 );
	lua_newtable( L );
	lua_replace( L, 1 );
	lua_settable( L, 1 );
	
	//There is a thing called model now.
	if ( !lt_init( &model, NULL, 1027 ) || !lua_to_table( L, 1, &model ) )
		return ERR_500("Couldn't turn aggregate table into a C table.\n" );

	//Make a new "render buffer"
	if ( !(renbuf = malloc( 30000 )) || !memset( renbuf, 0, 30000 ) )
		return ERR_500("Couldn't allocate enough space for a render buffer.\n" );

	//Rewind Loader ptr and load each view's raw text
	l = &ld[0];
	lt_dump( &model );

	//Load each view into a single buffer (can be malloc'd uint8 for now)
	while ( l->content ) {
		if ( l->type == CC_VIEW ) {
			//Somehow have to get the root directory of the site in question...
			char *vfile = strcmbd( "/", ag->activeDir, "views", l->content, "html" );
			int fd = 0, bt = 0;
			vfile[ strlen(vfile) - 5 ] = '.';

			if ( stat( vfile, &sb ) == -1 )
				return ERR_500("Couldn't find view file: %s. Error: %s", vfile, strerror( errno ) );

			//TODO: Pull a realloc here and just keep adding to the same buffer.
			//if ( !(renbuf = realloc( sb.st_size )) )
			//	return ERR_500("Couldn't reallocate enough space for output buffer." );
			
			if ( (fd = open( vfile, O_RDONLY )) == -1 )
				return ERR_500("Couldn't open view file: %s. Error: %s", vfile, strerror( errno ) );
			
			if ( (bt += read(fd, &renbuf[renbuflen], sb.st_size )) == -1 )
				return ERR_500("Couldn't read view file into buffer: %s.  Error: %s", vfile, strerror( errno ) );
			
			renbuflen += bt;
		}
		l++;
	}

	//um
	if ( !render_init( &ren, &model ) )
		return ERR_500("Couldn't initialize rendering engine." );

	if ( !render_map( &ren, (uint8_t *)renbuf, strlen( (char *)renbuf ) ) )
		return ERR_500("Couldn't set up render mapping." );

	if ( !render_render( &ren ) )	
		return ERR_500("Failed to carry out templating on buffer." );

	//Prepare the actual reponse
	http_set_status( h, 200 );
	http_set_content( h, "text/html", ( uint8_t * )
		bf_data(render_rendered(&ren)), bf_written(render_rendered(&ren)) );

	//Set the end of the response preparation step
	http_pack_response( h );
 #ifdef INCLUDE_TIMING_INFO_H
	timer_end( &t );
	timer_print( &t );
 #endif	

	fprintf( stderr, "RECVR @ http_run\n==================\n" );
	print_recvr( r );

	r->stage = NW_AT_WRITE;
	//free( renbuf );
	return 1;
}


//Options
Option opts[] = {
	{ "-s", "--start"    , "Start a server." },
	{ "-k", "--kill",      "Kill a running server." },
#if 0
	{ "-c", "--create",    "Create a new directory for hypno site.",'s' },	
	{ "-l", "--list",      "List all hypno sites on the system.",'s' },	
	//...
	{ "-d", "--dir",       "Choose this directory for serving web apps.",'s' },
	{ "-c", "--config",    "Use an alternate file for configuration.",'s' },	
	//I'm just thinking out loud here.
	{ "-u", "--user",      "Choose who to run as.",'s' },
	{ "-m", "--mode",      "Choose how server should evaluate hostnames.",'s' },
	{ NULL, "--chroot-dir","Choose a directory to change root to.",     's' },
#endif
	{ "-d", "--dir",       "Serve just one specific directory.",'s' },
	{ "-f", "--file",      "Try running a file and seeing its results.",'s' },
	{ "-m", "--max-conn",  "How many connections to enable at a time.", 'n' },
	{ "-n", "--no-daemon", "Do not daemonize the server when starting."  },
	{ "-p", "--port"    ,  "Choose port to start server on."          , 'n' },

	{ .sentinel = 1 }
};


//Kill the server
int kill_cmd( Option *opts, char *err, Passthru *pt ) {
#ifdef WIN32
 #error "Kill (as written here anyway) does not work on Windows."
#endif
	pid_t pid;
	int fd, len;
	struct stat sb;
	char buf[64] = {0};

	//For right now, we can just naively assume that the server has not started yet.
	if ( stat(pidfile, &sb) == -1 )	
		return ERRL( "Could not locate server process: %s", strerror(errno)  );
	
	//Make sure size isn't bigger than buffer
	if ( sb.st_size >= sizeof(buf) )
		return ERRL( "Error initializing process id."  );
 
	//Get the pid otherwise
	if ((fd = open( pidfile, O_RDONLY )) == -1 || (len = read( fd, buf, sb.st_size )) == -1) { 
		close( fd );
		return ERRL( "Cannot access my PID file; %s", strerror(errno)  );
	}

	//More checking...
	close( fd );
	for (int i=0; i<len; i++) {
		if ( !isdigit(buf[i]) ) {
			return ERRL( "PID is not a number."  );
		}
	}

	//Close the process
	pid = atoi( buf );
	if ( kill(pid, SIGTERM) == -1 )
		return ERRL( "Failed to kill server process: %s", strerror( errno )  );

	return 1;
}


//Load a different data.lua file from main()
int file_cmd( Option *opts, char *err, Passthru *pt ) {
	lua_State *L = NULL;  
	char *f = opt_get( opts, "--file" ).s;
	struct stat sb;
	Table t;

	if (!( L = luaL_newstate() ))
		return ERRL( "L is not initialized..."  );

	if ( stat( f, &sb ) == -1 )
		return ERRL( "File %s inaccessbile. %s", f, strerror( errno ) );
	
	if ( !lt_init( &t, NULL, 127 ) ) 
		return ERRL( "Couldn't initialize table for file reading."  );

	if ( !lua_load_file( L, f, &err ) ) {
		//TODO: Why does ERRL( str, f, err ) throw a compiler error? 
		char err2[4096] = { 0 };
		snprintf( err2, 4095, "%s", err );
		return ERRL( "Couldn't run file %s.  Error: %s", f, err2 );
	}

	lua_to_table( L, 1, &t );
	lt_dump( &t );
	return 1;
}


//Start the server from main()
int start_cmd( Option *opts, char *err, Passthru *pt ) {
	int stat, conn, port, daemonize;
	daemonize = !opt_set(opts, "--no-daemon");
	!(conn = opt_get(opts, "--max-conn").n) ? conn = 1000 : 0;
	!(port = opt_get(opts, "--port").n) ? port = 2000 : 0; 
	fprintf( stderr, "starting server on %d\n", port );
	//stat   = startServer( port, conn, daemonize ); 

	//I start a loop above, so... how to handle that when I need to jump out of it?
	//Define stuff
	Socket    sock = { 1/*Start a server*/, "localhost", "tcp", .port = port };
	Selector  sel  = {
		.read_min   = 12, 
		.write_min  = 12, 
		.max_events = 1000, 
		.global_ud  = pt,
		.lsize      = sizeof(HTTP),
		.recv_retry = 10, 
		.send_retry = 10, 
		.errors     = _nw_errors,
		.runners    = etc, 
		.run_limit  = 3, /*No more than 3 seconds per client*/
	};

	//Fork the children
	if ( daemonize ) {
		pid_t pid = fork();
		if ( pid == -1 ) {
			return ERRL( "Failed to daemonize server process: %s", strerror(errno) );
		}
		else if ( pid ) {
			int len, fd = 0;
			char buf[64] = { 0 };

			if ( (fd = open( pidfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR )) == -1 )
				return ERRL( "Failed to access PID file: %s.", strerror(errno));

			len = snprintf( buf, 63, "%d", pid );

			//Write the pid down
			if (write( fd, buf, len ) == -1)
				return ERRL( "Failed to log PID: %s.", strerror(errno));
		
			//The parent exited successfully.
			if ( close(fd) == -1 )
				return ERRL( "Could not close parent socket: %s", strerror(errno));
			return SUC_PARENT;
		}
	}

	//Open the socket
	if ( !socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock) )
		return ERRL("Failed to initialize a socket for port %d", port );

	//Initialize details for a non-blocking server loop
	if ( !initialize_selector(&sel, &sock) ) //&l, local_index))
		return ERRL("Failed to initialize server settings" );

	//Dump some data
	obprintf(stderr, "Listening at %s:%d\n", sock.hostname, sock.port);

	//Start the non-blocking server loop
	if ( !activate_selector(&sel) )
		return ERRL("Failure to properly initialize server select loop" );
	
	//Clean up and tear down.
	free_selector(&sel);
	return 1; //SUC_CHILD
}


//Command loop
struct Cmd
{ 
	const char *cmd;
	int (*exec)( Option *, char *, Passthru *pt );
} Cmds[] = {
	{ "--kill"     , kill_cmd  }
 ,{ "--file"     , file_cmd  }
 ,{ "--start"    , start_cmd }
 ,{ NULL         , NULL      }
};


//Server loop
int main (int argc, char *argv[])
{
	//Values
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

	//Allocate user data here
	Passthru *pt = malloc( sizeof(Passthru ) );
	memset( pt, 0, sizeof(Passthru) );

	//Set things
	if ( !opt_set( opts, "--dir" ) )
		pt->singleDir = 1, pt->webroot = default_dirname;
	else {
		//check that dir exists and can be touched
		struct stat check;
		pt->webroot = opt_get( opts, "--dir" ).s;

		if ( !pt->webroot || *pt->webroot == 0 || strlen( pt->webroot ) == 0 ) {
			fprintf( stderr, PROG ": %s\n", "Invalid directory specified." );
			return 1;
		}

		if ( stat( pt->webroot, &check ) == -1 ) { 
			fprintf( stderr, PROG ": %s\n", strerror( errno ) );
			return 1;
		}
	}

	//Evaluate all main stuff by looping through the above structure.
	struct Cmd *cmd = Cmds;	
	while ( cmd->cmd ) {
	#ifdef TESTOPTS_H
		fprintf( stderr, "Got option: %s? %s\n", cmd->cmd, opt_set(opts, cmd->cmd ) ? "YES" : "NO" );
	#endif
		if ( opt_set(opts, cmd->cmd ) && !cmd->exec( opts, err, pt ) ) {
			fprintf( stderr, PROG ": %s\n", err );
			return 1;
		}
		cmd++;
	}

	free( pt );
	return 0;
}

