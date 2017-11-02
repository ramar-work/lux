#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <libgen.h>
#include "bridge.h"

#include "vendor/single.h"
#include "vendor/nw.h"
#include "vendor/http.h"

#define PROG "luas"

#if 1
 //I did this because there will eventually be defines that control backends
 #include "vendor/sqlite3.h"
#endif

#ifndef LUA_53
 #define lua_rotate( a, b, c ) 
 #define lua_geti( a, b, c ) 
 #define lua_seti( a, b, c ) 
#endif

//Define some headers here
_Bool http_run (Recvr *r, void *p, char *e);

//Specify a pidfile
const char pidfile[] = "hypno.pid";

//Error buffer
char err[ 4096 ] = { 0 };

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

int abc ( lua_State *L ) { return 0; } 

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


//Set Lua functions
int set_lua_functions ( lua_State *L, luaCF *rg )
{
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

	return 1;
}


//This is the single-threaded HTTP run function
_Bool http_run (Recvr *r, void *p, char *e) 
{
	HTTP *h            = (HTTP *)r->userdata;
	HTTP_Request *req  = &h->request;
	req->mlen          = r->recvd;
	char buf[ 1024 ]   = { 0 };

	//This should receive the entire request
	SHOWDATA( "streaming request" );
	if ( !http_get_remaining( h, r->request, r->recvd ) )
		return http_err( h, 500, "Error processing request." );

 #if 0
	//This is a quick test to check that things work as they should.
	const char resp[] = 
		"HTTP/2.0 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 23\r\n\r\n"
		"<h2>Hello, world!</h2>\n";

	http_set_status( h, 200 );
	http_set_content( h, "text/html", ( uint8_t * )resp, strlen( resp ) );
	http_print_response( h );
 #else
	//This is what I'll usually use and the majority of work should happen here.
	const char *file = "b.lua"; // or "example/data.lua";
	lua_State *L  = luaL_newstate(); 
	luaCF     *rg = lua_functions;
	Buffer    *rr = h->resb;
	Table      t;     
	Render     R; 

	//Check that Lua initialized here
	if ( !L )
		return 0;
	else {
		luaL_openlibs( L );
		lua_newtable( L );
		lua_loop( L );
		set_lua_functions( L, rg );
		//Convert HTTP structure to something Lua can read
		table_to_lua( L, 1, &h->request.table );
	}

	//Load the first data.lua here
	if ( !lua_load_file( L, file, err ) ) 
		return http_err( h, 500, "Loading routes failed at file '%s': %s", file, err );
	else 
	{
		//Negotiate the route here.
		Loader ld[ 10 ];
		memset( ld, 0, sizeof( Loader ) * 10 );
		Loader *l = ld;
		char *ptr = NULL;

		//Turn on table
		if ( !lt_init( &t, NULL, 666 ) )
			return http_err( h, 500, "table did not initialize...\n" );

		//Convert the Lua table to a C table
		if ( !lua_to_table( L, 2, &t ) )
			return http_err( h, 500, "could not convert Lua table ...\n" );

		//Return an error here and tell what happened (if it comes to that)
		if ( !parse_route( ld, sizeof(ld) / sizeof(Loader), &h->request.table, &t ) )
			return http_err( h, 500, "Parsing routes failed." );

		//Cycle through all of the elements comprising the route
		while ( l->type ) 
		{
  	 #if 0
			else if ( l->type == CC_QUERY )
				return http_err( h, 500, "Lua query payloads don't work yet." );
			else if ( l->type == CC_FUNCT )
				return http_err( h, 500, "Lua function payloads don't work yet." );
		 #endif
			if ( l->type == CC_STR ) 
				bf_append( rr, (uint8_t *)l->content, strlen( l->content ) );	
			else if ( l->type == CC_MODEL || l->type == CC_VIEW ) 
			{
				//Check that the file exists and permissions are correct
				const char *dir = "example1";
				struct stat sb;
				memset( &sb, 0, sizeof( struct stat ) );
				char *file = NULL;
				char *type = (l->type == CC_MODEL ) ? "lua" : "html";
				char *sdir = (l->type == CC_MODEL ) ? "app" : "views";

				file = strcmbd( "/", dir, sdir, l->content, type );
				file[ (strlen( file ) - strlen( type )) - 1 ] = '.';
				fprintf( stderr, "Loading %s file: %s\n", sdir, file );

				if ( stat( file, &sb ) == -1 ) {
					free( file );
					return http_err( h, 500, "%s", strerror( errno ) );
				}

				//Execute referenced file name
				if ( l->type == CC_MODEL )
				{
					//When model is successfully loaded, a key made of the FILENAME 
					//will contain a table with the rendering data
					if ( !lua_load_file( L, file, err ) )
					{
						free( file );
						return http_err( h, 500, "%s", err );
					}
				}
				else if ( l->type == CC_VIEW )
				{
					//Open and display it for now...	
					char buf[ 8000 ] = {0};
					int fd = open( file, O_RDONLY, S_IRUSR | S_IRGRP );
					int r = read( fd, buf, sizeof(buf));
					bf_append( rr, (uint8_t *)buf, r );	
				}

				free( file );
			}
			else {
				return http_err( h, 500, "Some unknown payload was received." );
			}
			l++;
		}
	}

	//bypassing should make the headers work right...
	//fprintf( stderr, "Written so far %d...", bf_written( rr ));
	http_set_status( h, 200 );
	http_set_version( h, 1.0 );
	//http_set_content( h, "text/html", bf_data( &rb ), bf_written( &rb ) );
	http_set_content_type( h, "text/html" );
	http_set_content_length( h, bf_written( rr ) );
	http_pack_response( h );
	lt_free( &t );
	//destroy Lua environment?
 #endif	

	//This has to be set if you don't fail...
	http_print_response( h );
	r->stage = NW_AT_WRITE;
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


//Starts up a new server
/*
int startServer ( int port, int connLimit, int daemonize )

Returns a few different status depending on what happened:
ERR_SRVFORK - Failed to daemonize
ERR_PIDFAIL - Failed to open PID file, fatal b/c a zombie would exist
ERR_PIDWRFL - Failed to write to PID file, fatal b/c a zombie would exist 
SUC_PARENT  - Parent successfully started a daemon
SUC_CHILD   - Child successfully exited the function 
 */
#define ERR_SRVFORK 20
#define ERR_PIDFAIL 21
#define ERR_PIDWRFL 22 
#define ERR_SSOCKET 23 
#define ERR_INITCON 24 
#define ERR_SKLOOPS 25 
#define SUC_PARENT  27
#define SUC_CHILD   28
int startServer ( int port, int connLimit, int daemonize )
{
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
		if ( pid == -1 )
			return ERR_SRVFORK;//(fprintf(stderr, "Failed to daemonize.\n") ? 1 : 1);
		else if ( pid ) {
			int len, fd = 0;
			char buf[64] = { 0 };

			if ( (fd = open( pidfile, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR )) == -1 )
				return ERR_PIDFAIL;//(fprintf( stderr, "pid logging failed...\n" ) ? 1 : 1);

			len = snprintf( buf, 63, "%d", pid );

			//Write the pid down
			if (write( fd, buf, len ) == -1)
				return ERR_PIDWRFL;//(fprintf( stderr, "open of file failed miserably...\n" ) ? 1 : 1);
		
			//The parent exited successfully.
			close(fd);
			return SUC_PARENT;
		}
	}

	//Open the socket
	if (!socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock))
		return ERR_SSOCKET;//(fprintf(stderr, "Socket init error.\n") ? 1 : 1);

	//Initialize details for a non-blocking server loop
	if (!initialize_selector(&sel, &sock)) //&l, local_index))
		return ERR_INITCON;//nw_err(0, "Selector init error.\n"); 

	//Dump some data
	obprintf(stderr, "Listening at %s:%d\n", sock.hostname, sock.port);

	//Start the non-blocking server loop
	if (!activate_selector(&sel))
		return ERR_SKLOOPS;//(fprintf(stderr, "Something went wrong inside the select loop.\n") ? 1 : 1);
	
	//Clean up and tear down.
	free_selector(&sel);
	obprintf(stderr, "HTTP server done...\n");
	return SUC_CHILD;
}


#ifdef INCLUDE_KILL 
#ifdef WIN32
 #error "Kill is not gonna work here, son... Sorry to burst yer bublet."
#endif
//Kills a currently running server
int killServer () 
{
	pid_t pid;
	int fd, len;
	struct stat sb;
	char buf[64] = {0};

	//For right now, we can just naively assume that the server has not started yet.
	if ( stat(pidfile, &sb) == -1 )	
		return (fprintf( stderr, "No process running...\n" )? 1 : 1);	
	
	//Make sure size isn't bigger than buffer
	if ( sb.st_size >= sizeof(buf) )
		return ( fprintf( stderr, "pid initialization error..." ) ? 1 : 1 );
 
	//Get the pid otherwise
	if ((fd = open( pidfile, O_RDONLY )) == -1 || (len = read( fd, buf, sb.st_size )) == -1) 
	{
		close( fd );
		fprintf( stderr, "pid find error..." );
		return 1;
	}

	//More checking...
	close( fd );
	for (int i=0; i<len; i++) {
		if ( !isdigit(buf[i]) )  {
			fprintf( stderr, "pid is not really a number..." );
			return 1;
		}
	}

	//Close the process
	pid = atoi( buf );
	fprintf( stderr, "attempting to kill server process...\n");

	if ( kill(pid, SIGTERM) == -1 )
		return (fprintf(stderr, "Failed to kill proc\n") ? 1 : 1);	

	fprintf( stderr, "server dead...\n");
	return 0;
}
#endif



//Options
Option opts[] = 
{
	//Debugging and whatnot
	{ "-d", "--dir",       "Choose this directory for serving web apps.",'s' },
	{ "-c", "--config",    "Use an alternate file for configuration.",'s' },
	{ "-f", "--file",      "Try running a file and seeing its results.",'s' },

	//Server stuff
	{ "-s", "--start"    , "Start a server."                                },
	{ "-n", "--no-daemon", "Choose port to start server on."                },
#ifdef INCLUDE_KILL 
	{ "-k", "--kill",      "Kill a running server."                },
#endif
	//Parameters
	{ NULL, "--max-conn",  "How many connections to enable at a time.", 'n' },
	{ "-p", "--port"    ,  "Choose port to start server on."          , 'n' },
	{ "-c", "--chroot-dir","Choose a directory to change root to.",     's' },
	{ .sentinel = 1 }
};



//Server loop
int main (int argc, char *argv[])
{
	//Values
	(argc < 2) ? opt_usage(opts, argv[0], "nothing to do.", 0) : opt_eval(opts, argc, argv);

#ifdef INCLUDE_KILL 
	//Catch kill option first
	if ( opt_set(opts, "--kill") )
		killServer();
#endif

	//
	if ( opt_set(opts, "--file") )
	{
		lua_State *L = NULL;  
		char *f = opt_get( opts, "--file" ).s;
		Table t;

		if (!( L = luaL_newstate() ))
		{
			fprintf( stderr, "L is not initialized...\n" );
			return 0;
		}
		
		lt_init( &t, NULL, 127 );
		lua_load_file( L, f, err  );	
		lua_to_table( L, 1, &t );
		lt_dump( &t );
	}	

	//Start a server (and possibly fork it)
	if ( opt_set(opts, "--start") ) 
	{
		int stat, conn, port, daemonize;
		!(conn    = opt_get(opts, "--max-conn").n) ? conn = 1000 : 0;
		!(port    = opt_get(opts, "--port").n) ? port = 2000 : 0; 
		daemonize = !opt_set(opts, "--no-daemon");

		//Different statuses do different things
		stat = startServer( port, conn, daemonize ); 

	}
	return 0;
}
