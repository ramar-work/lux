//test.c

//Using your own client to test web servers is stupid.  
//Invoke curl, wget and chromium to test that things work as they should.
#include "vendor/single.h"

const char *cmds[] = {
	//Simple GETs
	"curl --form name=John --form shoesize=11 http://localhost:2000"
//, "wget http://localhost:2000"
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
	//
	char **cmd = (char **)cmds;	
	while ( *cmd ) {
		FILE *p = popen( *cmd, "r" );
		if ( !p )
			fprintf( stderr, "Couldn't execute sequence: '%s' - %s\n", *cmd, strerror(errno) );
		else {
			char buf[ 120000 ];
			memset( buf, 0, sizeof(buf) );
			//read and extract from the fifo?
			//fread( p );
			//close the fifo
			pclose( p );
		}
		cmd++;
	}
	return 0;
}	
