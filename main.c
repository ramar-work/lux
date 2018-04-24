#include "vendor/single.h"
#include "vendor/nw.h"
#include "vendor/http.h"
#include "bridge.h"
#include <dirent.h>

#define PROG "hypno"
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

//Skip adding Lua functions
//#define DONT_ADD_LUA_FUNCTIONS_H

//Skip adding ENV to Lua space
//#define DONT_ADD_HTTP_TO_LUA_H

//Testing: See specifically where certain functions are crapping out.
//#define ERR_SUPERV

//Testing: I'm going to include an EXAMPLE define that uses the 'example' folder.
#define EXAMPLE_H

//Testing: Option module is not so wonderful yet, so use this to test out things having to do with it.
#define TESTOPTS_H

#ifndef LUA_53
 #define lua_rotate( a, b, c ) 
 #define lua_geti( a, b, c ) 
 #define lua_seti( a, b, c ) 
#endif

//This define controls whether or not line numbers will also be included in the error message.
#ifdef ERR_SUPERV
 #define ERRL(...) \
	( snprintf( err, 24, "[ @%s(), %d ]                 ", __FUNCTION__, __LINE__ ) ? 1 : 1 \
	&& ( err += 24 ) && snprintf( err, errlen - 24, __VA_ARGS__ ) ) ? 0 : 0
#else
 #define ERRL(...) snprintf( err, errlen, __VA_ARGS__ ) ? 0 : 0
#endif

//This define should be a bit easier to use than fully calling http_err
#define ERRH(...) http_err( r, h, 500, __VA_ARGS__ ) 

//Define some headers here
_Bool http_run (Recvr *r, void *p, char *e);

//Specify a pidfile
const char pidfile[] = "hypno.pid";

//Error buffer
char err[ 4096 ] = { 0 };
const int errlen = 4096; 

//This is nw's data structure for handling protocols 
Executor runners[] = 
{
	[NW_AT_READ]        = { .exe = http_read, NW_NOTHING },
	[NW_AT_PROC]        = { .exe = http_run , NW_NOTHING },
	[NW_AT_WRITE]       = { .exe = http_fin , NW_NOTHING },
	[NW_COMPLETED]      = { .exe = http_fin , NW_NOTHING }
};


//Table of Lua functions
typedef struct 
{
	char *name; 
	lua_CFunction func; 
	char *setname; 
	int sentinel;
} luaCF;

int abc ( lua_State *L ) { fprintf( stderr, "chicken" ); return 0; } 

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


//This is the single-threaded HTTP run function
_Bool http_run ( Recvr *r, void *p, char *err ) 
{ 
	#ifdef INCLUDE_TIMING_INFO_H
	Timer t;
	char tbuf[1024]={0};
	sprintf( tbuf,  "http_run at %s:%d\n", __FUNCTION__, __LINE__ ); 
	timer_use_us( &t );
	timer_start( &t );
	#endif	
	//This is a hell of a lot of data.
	Table  routes, request, model;
	struct stat sb;
	Loader ld[ 10 ], *l = NULL;
	Render ren;
	uint8_t *renbuf = NULL;
	int renbuflen = 0;
	HTTP *h = (HTTP *)r->userdata;
	HTTP_Request *req = &h->request;
	lua_State *L  = luaL_newstate(); 
	char *activeDir = NULL;
	char *datafile = NULL;
	char *fSetName = NULL;
	DIR *ds = NULL;
	struct dirent *de;
	const char *dirname = "example";

	//Set the message length
	req->mlen = r->recvd;

	//Set up the "loader" structure
	memset( ld, 0, sizeof( Loader ) * 10 );

	//This should receive the entire request
	if ( !http_get_remaining( h, r->request, r->recvd ) )
		return ERRH("Error processing request." );

	//For now, I assume the root is at 'example'.
	if ( !( ds = opendir( dirname ) ) )
		return ERRH("Couldn't access web root: %s.", strerror(errno) );

	//fprintf( stderr, "Directory '%s' contains:\n", dirname );
	while ((de = readdir( ds ))) {
		char *fd = strcmbd( "/", dirname, de->d_name );

		//Check that the child inode is accessible
		if ( lstat( fd, &sb ) == -1 )
			return ERRH("Can't access directory %s: %s.", fd, strerror(errno));

		//Check the name of the folder and see if the hostname matches
		if (S_ISDIR(sb.st_mode) || S_ISLNK((sb.st_mode))) {
			if ( strcmp( de->d_name, h->hostname ) == 0 ) {
				activeDir = fd;
				break;
			}	
		}
		free(fd);
	}

	//Default responses get handled here 
	if ( !activeDir ) {
		return ERRH( "No site matching hostname '%s' found.", h->hostname );
	}
	else {
		//Seems like the mime chain would go here.
		//If I were another server, I would configure mime types on a per site basis and evaluate them here.
		0;
	}

	//If the client is asking for 'favicon.ico', just stop and die...
	if ( strcmp( h->request.path, "/favicon.ico" ) == 0 ) {
		return ERRH( "Favicons aren't handled yet, sorry..." );
	}

	//Check that Lua initialized here
	if ( !L )
		return ERRH("Failed to create new Lua state?" );

	//Open Lua libraries.
	luaL_openlibs( L );
	lua_newtable( L );

	//Read the data file for whatever "site" is gonna be run.
	datafile = strcmbd( "/", activeDir, "data.lua" );

	//Always waste some time looking for the file
	if ( stat( datafile, &sb ) == -1 )
		return ERRH("Couldn't find file containing site data: %s.", datafile );

	//Load the route file.
	if ( !lua_load_file( L, datafile, &err ) )
		return ERRH("Loading routes failed at file '%s': %s", datafile, err );

	//Convert this to an actual table so C can work with it...
	if ( !lt_init( &routes, NULL, 666 ) || !lua_to_table( L, 2, &routes) )
		return ERRH("Converting routes from file '%s' failed.", datafile );

	//Clear stack to get rid of what came from routes
	lua_settop( L, 0 );

	//Add HTTP to the global space.
	lua_newtable( L );
	table_to_lua( L, 1, &h->request.table );
	lua_setglobal( L, "env" ); /*This needs to be readonly*/

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
		return ERRH("Finding the model and view for the current route failed." );

	//Reset Loader pointer and clear the stack
	l = &ld[0];
	lua_settop( L, 0 );
	
	while ( l->content ) {
		//Load each model file (which is just running via Lua)
		if ( l->type == CC_MODEL ) { 
			//Somehow have to get the root directory of the site in question...
			char *mfile = strcmbd( "/", activeDir, "models", l->content, "lua" );
			int mfilelen = strlen( mfile );
			mfile[ mfilelen - 4 ] = '.';
			fprintf( stderr, "Attempting to load: %s\n", mfile );

			if ( stat( mfile, &sb ) == -1 )
				return ERRH("Couldn't find model file: %s.", mfile );

			if ( luaL_dofile( L, mfile ) )
			{
				const char *msg = lua_tostring(L, -1);
				lua_settop( L, 0 );
				return ERRH("Could not load Lua document at %s. Error message: %s\n", mfile, msg );
			}
		}

		l++;
	}

	//This is a working solution.  Still gotta figure out the reason for that crash...
	lua_aggregate( L ); //1
	lua_pushstring(L,"model"); //2
	lua_pushvalue(L,1); //3
	lua_newtable(L); //4
	lua_replace(L,1); //3
	lua_settable(L,1); //1
	
	//There is a thing called model now.
	if ( !lt_init( &model, NULL, 1027 ) || !lua_to_table( L, 1, &model ) )
		return ERRH("Couldn't turn aggregate table into a C table.\n" );

	//Make a new "render buffer"
	if ( !(renbuf = malloc( 30000 )) || !memset( renbuf, 0, 30000 ) )
		return ERRH("Couldn't allocate enough space for a render buffer.\n" );

	//Rewind Loader ptr and load each view's raw text
	l = &ld[0];

	//Load each view into a single buffer (can be malloc'd uint8 for now)
	while ( l->content ) {
		if ( l->type == CC_VIEW ) {
			//Somehow have to get the root directory of the site in question...
			char *vfile = strcmbd( "/", activeDir, "views", l->content, "html" );
			int fd, bt = 0, vfilelen = strlen( vfile );
			vfile[ vfilelen - 5 ] = '.';

			if ( stat( vfile, &sb ) == -1 )
				return ERRH("Couldn't find view file: %s. Error: %s", vfile, strerror( errno ) );

			//TODO: Pull a realloc here and just keep adding to the same buffer.
			//if ( !(renbuf = realloc( sb.st_size )) )
			//	return ERRH("Couldn't reallocate enough space for output buffer." );
			
			if ( (fd = open( vfile, O_RDONLY )) == -1 )
				return ERRH("Couldn't open view file: %s. Error: %s", vfile, strerror( errno ) );
			
			if ( (bt += read(fd, &renbuf[renbuflen], sb.st_size )) == -1 )
				return ERRH("Couldn't read view file into buffer: %s.  Error: %s", vfile, strerror( errno ) );
			
			renbuflen += bt;
		}
		l++;
	}

	if ( !render_init( &ren, &model ) )
		return ERRH("Couldn't initialize rendering engine." );

	if ( !render_map( &ren, (uint8_t *)renbuf, strlen( (char *)renbuf ) ) )
		return ERRH("Couldn't set up render mapping." );

	if ( !render_render( &ren ) )	
		return ERRH("Failed to carry out templating on buffer." );

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
	r->stage = NW_AT_WRITE;
	free( renbuf );
	return 1;
}


//Handle http requests via Lua
int lua_http_handler (HTTP *h, Table *p)
{
	//Initialize Lua's environment and set up everything else
	char renderblock[ 60000 ] = { 0 };
	lua_State *L = luaL_newstate(); 
	luaCF *rg = lua_functions;
	LiteBlob *b  = NULL;
	Table t;     
	Render R; 
	Buffer *rr = NULL;
	char *modelfile = NULL, 
       *viewfile = NULL;

	//Set up a table
	lt_init( &t, NULL, 127 );
	
	//Check that Lua initialized here
	if ( !L )
		return 0;

	//Now create two tables: 1 for env, and another for 
	//user defined functions 
	luaL_openlibs( L );
	lua_newtable( L );
	int at=2;
	lua_loop( L );

	//Loop through and add each UDF
	while ( rg->sentinel != -1 )
	{
		//Set the top table
		if ( rg->sentinel == 1 ) 
		{
			lua_settable( L, 1 );
			lua_loop( L );
		}

		else if ( !rg->name && rg->setname )
		{
			obprintf( stderr, "Registering new table: %s\n", rg->setname );
			lua_pushstring( L, rg->setname );
			lua_newtable( L );
			lua_loop( L );
		}

		else if ( rg->name )
		{	
			obprintf( stderr, "Registering funct: %s\n", rg->name );
			lua_pushstring( L, rg->name );
			lua_pushcfunction( L, rg->func );
			lua_settable( L, 3 );
		}
		rg++;
	}

	//Loop through all of the http structure
	table_to_lua( L, 1, &h->request.table );

	//Each one of these needs to be in a table
	obprintf( stderr," Finished converting HTTP data into Lua... " );
	lua_setglobal( L, "env" ); /*This needs to be readonly*/

	//Reverse lookup of host
	char hh[ 2048 ] = { 0 };
	char *dir = NULL;
	int ad=0;
	memcpy( &hh[ ad ], "sites.", 6 ); ad += 6;
	memcpy( &hh[ ad ], h->hostname, strlen( h->hostname ) ); ad+=strlen(h->hostname);
	memcpy( &hh[ ad ], ".dir", 4 );ad+=4;
	dir = lt_text( p, hh );

	//Reuse buffer for model file
	ad = 0;
	memcpy( &hh [ ad ], dir, strlen( dir ) );
	ad += strlen( dir );
	memcpy( &hh [ ad ], "/index.lua", 10 );
	ad += 10;
	hh[ ad ] = '\0';	

	//Get data.lua if it's available and load routes
	fprintf( stderr, "about to execute: %s\n", hh );

	if ( luaL_dofile( L, hh ) != 0 )
	{
		fprintf( stderr, "Error occurred!\n" );
		if ( lua_gettop( L ) > 0 ) {
			fprintf( stderr, "%s\n", lua_tostring( L, 1 ) );
		}
	}

	//Converts what came from the stack
	lua_loop( L );
	lua_to_table( L, 1, &t );
	lt_dump( &t );

	//Reuse buffer for view file
	ad = 0;
	memcpy( &hh [ ad ], dir, strlen( dir ) ); ad += strlen( dir );
	memcpy( &hh [ ad ], "/index.html", 11 ); ad += 11;
	hh[ ad ] = '\0';	


	//Initialize the rendering module	
	//TODO: Error handling is non-existent here...
	int fd = open( hh, O_RDONLY );
	read( fd, renderblock, sizeof( renderblock )); 
	close(fd);
	render_init( &R, &t );
	render_map( &R, (uint8_t *)renderblock, strlen( renderblock ));
	render_render( &R ); 
	rr = render_rendered( &R );
	//write( 2, bf_data( rr ), bf_written( rr ) );

#if 1
	http_set_status( h, 200 );
	http_set_content( h, "text/html", bf_data( rr ), bf_written( rr ));
	//http_set_content_length( h, );
	//http_set_content_type( h, "text/html" );
#endif

	render_free( &R );
	lt_free( &t );
	return 1;
}


//Options
Option opts[] = 
{
	{ "-s", "--start"    , "Start a server."                                },
	{ "-k", "--kill",      "Kill a running server."                },

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
	{ "-f", "--file",      "Try running a file and seeing its results.",'s' },
	{ "-m", "--max-conn",  "How many connections to enable at a time.", 'n' },
	{ "-n", "--no-daemon", "Do not daemonize the server when starting."  },
	{ "-p", "--port"    ,  "Choose port to start server on."          , 'n' },

	{ .sentinel = 1 }
};


//Kill the server
int kill_cmd( Option *opts, char *err ) 
{
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
int file_cmd( Option *opts, char *err ) 
{
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

	if ( !lua_load_file( L, f, &err ) )
		return ERRL( "Couldn't run file %s.  Error: %s", f, err  );

	lua_to_table( L, 1, &t );
	lt_dump( &t );
	return 1;
}


//Start the server from main()
int start_cmd( Option *opts, char *err ) 
{
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
		.global_ud  = NULL, //(void *)&obsidian,
		.lsize      = sizeof(HTTP),
		.recv_retry = 10, 
		.send_retry = 10, 
		.errors     = _nw_errors,
		.runners    = runners, 
		.run_limit  = 3, /*No more than 3 seconds per client*/
	};

	//Fork the children
	if ( daemonize )
	{
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
	int (*exec)( Option *, char *);
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

	//Evaluate all main stuff by looping through the above structure.
	struct Cmd *cmd = Cmds;	
	while ( cmd->cmd ) {
	#ifdef TESTOPTS_H
		fprintf( stderr, "Got option: %s? %s\n", cmd->cmd, opt_set(opts, cmd->cmd ) ? "YES" : "NO" );
	#endif
		if ( opt_set(opts, cmd->cmd ) && !cmd->exec( opts, err ) ) {
			fprintf( stderr, PROG ": %s\n", err );
			return 1;
		}
		cmd++;
	}

	return 0;
}

