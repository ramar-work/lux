/*
test.c
======

This script is used to test out our new web server against a couple of 
different browsers.  They all act a little bit differently.  Need to 
always be able to invoke a bunch of stuff.  

Would this be easier in the shell?  Since I have to use popen anyway?

- consider using an actual thinkpad again or a dell (the keyboard is the concern)
- to make this fake pipe thing work, you'll need to break up the process invocation process 	
	I think its:
		fork
		open
		read
		close
		die
-

*/

#include "vendor/single.h"

const char *cmds[] = {
	//Simple GETs
//	"curl -i --form name=John --form shoesize=11 http://localhost:2000?yes=bounce&twelve=kittens"
  "wget -O- http://localhost:2000"
, "wget -O- --post-data 'name=John&shoesize=111' 'http://localhost:2000?yes=bounce&twelve=kittens'"
//, "chromium http://localhost:2000"

#if 0
	//Complex GETs
, "curl http://localhost:2000?id=1"
, "wget http://localhost:2000?id=234234&action=add"
//, "chromium http://localhost:2000?id=234234&action=add"

	//Stress testing GETs
	//...

	//Simple POSTs, files are embedded and written at the beginning
, "curl http://localhost:2000"
, "wget http://localhost:2000"
//, "chromium http://localhost:2000"
#endif
, NULL
};



struct TestCase {
	const char *addr;
	char *content;
	int status;
};



int main (int argc, char *argv[]) {

	struct values {
		int port;
		int ssl;
		int start;
		int kill;
		int fork;
		char *user;
	} values = { 0 };
#if 0
	//Process all your options...
	if ( argc < 2 ) {
		fprintf( stderr, "No options received.\n" );
		const char *fmt = "  --%-10s       %-30s\n";
		fprintf( stderr, fmt, "start", "start new servers" );
		fprintf( stderr, fmt, "kill", "test killing a server" );
		return 0;	
	}	
	else {
		while ( *argv ) {
			if ( strcmp( *argv, "--start" ) == 0 ) 
				values.start = 1;
			else if ( strcmp( *argv, "--user" ) == 0 ) {
				argv++;
				if ( !*argv ) {
					fprintf( stderr, "Expected argument for --user!" );
					return 0;
				} 
				values.user = strdup( *argv );
			}
			argv++;
		}
	}
#endif

	const char testdir[] = "/tmp/hypnotests";
	int index = 0;
	char **cmd = (char **)cmds;	

#if 1
	//get rid of the folder if there was one...
	FILE *n = popen( "rm -rf /tmp/hypnotests", "r" );
	if ( !n ) 
		fprintf( stderr, "Couldn't remove %s: '%s'\n", testdir, strerror(errno) );
	else {
		pclose( n );
	}

	//mkdir a test directory of some sort
	if ( mkdir( testdir, S_IRWXU ) == -1 ) {
		fprintf( stderr, "mkdir error: %s\n", strerror (errno) );
		return 1;
	}
#endif


#if 0
#else
	while ( *cmd ) {
		//clone the command, add your own redirect rules
		char newcmd[ 1024 ];
		memset( newcmd, 0, sizeof( newcmd ) );
		snprintf( newcmd, sizeof( newcmd ) - 1, "%s 2>/dev/null >%s/%d", *cmd, testdir, index ); 

		//run the new process
		FILE *p = popen( newcmd, "r" );
		if ( !p )
			fprintf( stderr, "Couldn't execute sequence: '%s' - %s\n", *cmd, strerror(errno) );
		else {
			//char buf[ 120000 ];
			//memset( buf, 0, sizeof(buf) );
			//read and extract from the fifo?
			//fread( p );
			//close the fifo
			//wait until it's done?
			pclose( p );
		}
		cmd++;
	}

	//We need to wait until each of these is done before moving...
	//(but not too long...)

	//Now, open each file put the message in a buffer, and 
	//mark the httpstatus, command status and more
	//cmdstatus | httpstatus | message (expected) | message (recvd) | time to read complete message / early termination
	cmd = (char **)cmds;	
	while ( *cmd ) {
		fprintf( stderr, "*cmd was: %s\n", *cmd );
		//Couple ways of doing this
	#if 0
		//Save to SQLite, and read stuff off
	#endif	
	#if 0
		//Save to simple text delimited file (can pipe to markdown or something else)
	#endif	
		cmd++;
	}
#endif
	
	return 0;
}	
