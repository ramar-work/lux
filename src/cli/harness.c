/* ------------------------------------------- * 
 * harness.c
 * =========
 *
 * Summary 
 * -------
 * Command line tooling to test hypno sites.
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
#include <stdlib.h>
#include <strings.h>
#include <dlfcn.h>
#include <errno.h>
#include <zwalker.h>
#include <zhttp.h>
#include <zjson.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "../util.h"
#include "../server/server.h"
#include "../filters/filter-static.h"
#include "../filters/filter-echo.h"
#include "../filters/filter-dirent.h"
#include "../filters/filter-redirect.h"
#include "../filters/filter-lua.h"
#include "../lua.h"
#include "../lua.h"

#define PP "hypno-harness"

#define FSYMBOL "filter"

#define NSYMBOL "libname"

#define CTYPE_JSON "application/json"

#define CTYPE_XML "application/xml"

#define HELP \
	"-f, --filter <arg>       Specify a filter for testing (required).\n" \
	"-l, --library <arg>      Specify path to library.\n" \
	"-d, --directory <arg>    Specify path to web app directory (required).\n" \
	"-u, --uri <arg>          Specify a URI (required).\n" \
	"-c, --content-type <arg> Specify a content-type for testing.\n" \
	"-n, --host <arg>         Specify a hostname for use w/ the request.\n" \
	"-a, --alias <arg>        Specify an alternate hostname for use w/ the request.\n" \
	"-m, --method <arg>       Specify an HTTP method to be used when making\n" \
	"                         a request. (GET is default)\n" \
	"-p, --protocol <arg>     Specify alternate protocols (HTTP/1.0, 2.0, etc)\n" \
	"-F, --form <arg>         Specify a body to use when making requests.\n" \
	"-b, --binary <arg>       Specify a body to use when making requests. (assumes multipart)\n" \
	"                         (Use multiple invocations for additional arguments)\n" \
	"-E, --header <arg>       Specify a header to use when making requests.\n" \
	"                         (Use multiple invocations for additional arguments)\n" \
	"-t, --test <arg>         Use <arg> to run a test.\n" \
	"-M, --multipart          Use a multipart request when using POST or PUT\n" \
	"-S, --msg-only           Show only the message, no header info\n" \
	"-B, --body-to <arg>      Output body to file at <arg>\n" \
	"-H, --headers-to <arg>   Output headers to file at <arg>\n" \
	"-X, --dump-args          Dump the supplied arguments.\n" \
	"-D, --dump-http          Dump the HTTP request that was created and stop.\n" \
	"-O, --dump-response      Dump the HTTP response when using a test file.\n" \
	"-v, --verbose            Be wordy.\n" \
	"-h, --help               Show help and quit.\n"

//Define a list of filters
filter_t filters[16] = { 
	{ "static", filter_static }
,	{ "echo", filter_echo }
,	{ "dirent", filter_dirent }
,	{ "redirect", filter_redirect }
,	{ "lua", filter_lua }
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
};


struct arg {
	char *lib;
	char *path;
	char *ctype;
	char *host;
	char *alias;
	char *method;
	char *protocol;
	char *uri;
	char *filter;
	char *luatest;
	int verbose;
	int randomize;
	int multipart;
	int blen;
	int hlen;
	int msgonly;
	char *headerf;
	char *bodyf;
	int dumpArgs;
	int dumpHttp;
	int dumpResp;
	char **headers;
	char **body;
};


struct test {
	ztable_t *table;
	char *ctype;
	char *host;
	char *uri;
	char *method;
	char *protocol;

	struct expected {
		int status;
		int clength;
		char *ctype;
		char **headers;
		unsigned char *payload;
	} expected;
};


void dump_args( struct arg *arg ) {
	fprintf( stderr, "lib:       %s\n", arg->lib );
	fprintf( stderr, "path:      %s\n", arg->path );
	fprintf( stderr, "ctype:     %s\n", arg->ctype );
	fprintf( stderr, "host:      %s\n", arg->host );
	fprintf( stderr, "alias:     %s\n", arg->alias );
	fprintf( stderr, "method:    %s\n", arg->method );
	fprintf( stderr, "protocol:  %s\n", arg->protocol );
	fprintf( stderr, "uri:       %s\n", arg->uri );
	fprintf( stderr, "headers:   %p\n", arg->headers );
	fprintf( stderr, "body:      %p\n", arg->body );

	fprintf( stderr, "verbose:   %s\n", arg->verbose ? "Y" : "N" );
	fprintf( stderr, "randomize: %s\n", arg->randomize ? "Y" : "N" ); 
	fprintf( stderr, "multipart: %s\n", arg->multipart ? "Y" : "N" );
	fprintf( stderr, "msg-only:  %s\n", arg->msgonly ? "Y" : "N" );
	fprintf( stderr, "blen:      %d\n", arg->blen );
	fprintf( stderr, "hlen:      %d\n", arg->hlen );
}


int method_expects_body ( char *mstr ) {
	const char *mth[] = { 
		"POST", "PUT", "DELETE"
	};

	for ( int i=0; i<sizeof(mth)/sizeof(const char *); i++ ) {
		if ( !strcasecmp( mth[i], mstr ) ) return 1;
	}

	return 0;	
}



int method_is_valid ( char *mstr ) {
	const char *mth[] = { 
		"HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS", "TRACE" 
	};

	for ( int i=0; i<sizeof(mth)/sizeof(const char *); i++ ) {
		if ( !strcasecmp( mth[i], mstr ) ) return 1;
	}

	return 0;
}



int main ( int argc, char * argv[] ) {
	//Make this
	struct arg arg = {0};
	struct test test = {0};
	int blen = 0;
	void *app = NULL;
	const int (*filter)( int, zhttp_t *, zhttp_t *, struct cdata * );
	zhttp_t req = {0}, res = {0};
	char *fname = NULL, err[ 2048 ] = { 0 };
	struct cdata conn;
	struct lconfig sconf;
	int header_fd=1, body_fd=1;
	ztable_t *lt = NULL;

	if ( argc < 2 ) {
		fprintf( stderr, PP ":\n%s\n", HELP );
		return 1;
	}

	while ( *argv ) {
		if ( !strcmp( *argv, "-l" ) || !strcmp( *argv, "--library" ) )
			arg.lib = *( ++argv );
		else if ( !strcmp( *argv, "-d" ) || !strcmp( *argv, "--directory" ) )
			arg.path = *( ++argv );
		else if ( !strcmp( *argv, "-f" ) || !strcmp( *argv, "--filter" ) )
			arg.filter = *( ++argv );
		else if ( !strcmp( *argv, "-u" ) || !strcmp( *argv, "--uri" ) )
			arg.uri = *( ++argv );
		else if ( !strcmp( *argv, "-n" ) || !strcmp( *argv, "--host" ) )
			arg.host = *( ++argv );
		else if ( !strcmp( *argv, "-c" ) || !strcmp( *argv, "--content-type" ) )
			arg.ctype = *( ++argv );
		else if ( !strcmp( *argv, "-m" ) || !strcmp( *argv, "--method" ) )
			arg.method = *( ++argv );
		else if ( !strcmp( *argv, "-p" ) || !strcmp( *argv, "--protocol" ) )
			arg.protocol = *( ++argv );
		else if ( !strcmp( *argv, "-M" ) || !strcmp( *argv, "--multipart" ) )
			arg.multipart = 1;
		else if ( !strcmp( *argv, "-S" ) || !strcmp( *argv, "--msg-only" ) )
			arg.msgonly = 1;
		else if ( !strcmp( *argv, "-H" ) || !strcmp( *argv, "--headers-to" ) )
			arg.headerf = *( ++argv );
		else if ( !strcmp( *argv, "-B" ) || !strcmp( *argv, "--body-to" ) )
			arg.bodyf = *( ++argv );
		else if ( !strcmp( *argv, "-r" ) || !strcmp( *argv, "--random" ) )
			arg.randomize = 1;
		else if ( !strcmp( *argv, "-v" ) || !strcmp( *argv, "--verbose" ) )
			arg.verbose = 1;
		else if ( !strcmp( *argv, "-X" ) || !strcmp( *argv, "--dump-args" ) )
			arg.dumpArgs = 1;
		else if ( !strcmp( *argv, "-D" ) || !strcmp( *argv, "--dump-http" ) )
			arg.dumpHttp = 1;
		else if ( !strcmp( *argv, "-O" ) || !strcmp( *argv, "--dump-response" ) )
			arg.dumpResp = 1;
		else if ( !strcmp( *argv, "-t" ) || !strcmp( *argv, "--test" ) )
			arg.luatest = *( ++argv );
		else if ( !strcmp( *argv, "-E" ) || !strcmp( *argv, "--header" ) ) {
			char * a = *( ++argv );
			add_item( &arg.headers, a, unsigned char *, &arg.hlen );
		}
		else if ( !strcmp( *argv, "-F" ) || !strcmp( *argv, "--form" ) ) {
			char * a = *( ++argv );
			add_item( &arg.body, a, unsigned char *, &arg.blen );
		}
		else if ( !strcmp( *argv, "-b" ) || !strcmp( *argv, "--binary" ) ) {
			char * a = *( ++argv );
			add_item( &arg.body, a, unsigned char *, &arg.blen ); 
			arg.multipart = 1;
		}
		else if ( !strcmp( *argv, "-h" ) || !strcmp( *argv, "--help" ) ) {
			fprintf( stderr, "%s\n", HELP );
			return 0;
		}	
		argv++;
	}

	//Dump the arguments if you need to
	if ( arg.dumpArgs ) {
		dump_args( &arg );
	}

	//Catch any problems
	if ( !arg.method )
		arg.method = ( arg.body ) ? "POST" : "GET";
	else if ( !method_is_valid( arg.method ) ) {
		fprintf( stderr, PP ": Wrong method requested: %s\n", arg.method );
		return 1;
	}

#if 1
	if ( !arg.filter ) {
		fprintf( stderr, PP ": No filter (or library) specified.\n" );
		return 1;
	} 

	if ( arg.filter ) {
		filter = NULL;
		for ( filter_t *f = filters; f->name; f++ ) {
			if ( strcmp( f->name, arg.filter ) == 0 ) {
				filter = f->filter, fname = (char *)f->name;
				break;
			}
		}
		if ( !filter ) {
			fprintf( stderr, PP ": Filter '%s' not supported.\n", arg.filter );
			return 1;
		}
	}

	if ( arg.luatest ) {
		lua_State *L = NULL;
		char err[ 1024 ] = { 0 };
		int count = 0;

		if ( !( L = luaL_newstate() ) ) {
			fprintf( stderr, PP ": Error opening Lua state\n" );
			return 1;
		}

		if ( !lua_exec_file( L, arg.luatest, err, sizeof( err ) ) ) {
			fprintf( stderr, PP ": Error running file '%s': %s\n", arg.luatest, err );
			return 1;
		}

		count = lua_count( L, 1 );
		if ( !( test.table = lt_make( count * 2 ) ) ) {
			fprintf( stderr, PP ": Error creating ztable!\n" );
			return 1;
		}

		if ( !lua_to_ztable( L, 1, test.table ) ) {
			fprintf( stderr, PP ": Error converting Lua table to ztable!\n" );
			return 1;
		}

		lt_lock( test.table );
		lua_close( L );

	#if 0
		for ( const char **term = terms; *term; term++ ) {

		}
	#else
		int i = 0, run = 1;
		if ( ( i = lt_geti( test.table, "ctype" )	) > -1 )
			arg.ctype = test.ctype = zhttp_dupstr( lt_text_at( test.table, i ) );

		if ( ( i = lt_geti( test.table, "host" )	) > -1 )
			arg.host = test.host = zhttp_dupstr( lt_text_at( test.table, i ) );

		if ( ( i = lt_geti( test.table, "uri" )	) > -1 )
			arg.uri = test.uri = zhttp_dupstr( lt_text_at( test.table, i ) );

		if ( ( i = lt_geti( test.table, "method" )	) > -1 )
			arg.method = test.method = zhttp_dupstr( lt_text_at( test.table, i ) );

		if ( ( i = lt_geti( test.table, "protocol" )	) > -1 )
			arg.protocol = test.protocol = zhttp_dupstr( lt_text_at( test.table, i ) );

		//If the content-type is a serializable type, let's do something with that here
		if ( test.ctype && ( !strcmp( test.ctype, CTYPE_JSON ) || !strcmp( test.ctype, CTYPE_XML ) ) ) {
			run = 0;
			
			//Extract the values first and convert them
			if ( lt_geti( test.table, "values" ) > -1 ) {
		
				int blen = 0, slen = 0;
				char *str = NULL, err[ 1024 ] = {0};
				ztable_t *vt = lt_copy_by_key( test.table, "values" );
				zhttpr_t *b = NULL;
				lt_reset( vt );
				
				if ( !strcmp( test.ctype, CTYPE_JSON ) )
					0; //str = zjson_encode( vt, err, sizeof( err ) ); 
				else {
					fprintf( stderr, PP ": Serialzation with %s is not enabled yet.\n", test.ctype );
					return 1;
				}

				//The encoding process failed here...
				if ( !str ) {
					fprintf( stderr, PP ": Serialization failed: %s\n", err );
					return 1;
				}

				//Chop values (assumes that encoding looks like '{"values": {" 
				memmove( str, &str[11], ( slen = strlen( str ) ) - 11 );
				memset( &str[ slen - 13 ], 0, 13 ); 

				//Add this one to the body
				if ( !( b = malloc( sizeof( zhttpr_t ) ) ) || !memset( b, 0, sizeof( zhttpr_t ) ) ) {
					fprintf( stderr, PP ": Could not allocate space for serialized body\n" );
					return 1;
				}

				b->field = zhttp_dupstr( "body" );
				b->value = (unsigned char *)str;
				req.clen = b->size = strlen( str );
				add_item( &req.body, b, zhttpr_t *, &blen ); 
				arg.body = NULL;
			}
		}		

		struct luavv_t {
			const char *val;
			char ***array;
			int run; // Will not run if ctype is xml or json
			int alen;
		} vv[] = {
			{ "headers", &arg.headers, 1 }
		, { "values", &arg.body, run }
		, { NULL }
		};

		for ( struct luavv_t *val = vv; val->val; val++ ) {
			if ( ( i = lt_geti( test.table, val->val ) ) > -1 ) {
				ztable_t *tt = lt_copy_by_index( test.table, i );
				lt_reset( tt ), lt_next( tt );

				//Both sides should be text (or text and integer, die if not for now)
				for ( zKeyval *kv; ( kv = lt_next( tt ) ) ; ) {
					char *a = NULL;
					int alen = 2048;

					if ( kv->key.type == ZTABLE_TRM ) {
						break;	
					}

					if ( kv->key.type != ZTABLE_TXT ) {
						fprintf( stderr, PP ": Key in %s table at %s was not a string.\n", val->val, arg.luatest );
						return 1;
					}
	 
					if ( kv->value.type != ZTABLE_TXT && kv->value.type != ZTABLE_INT ) {
						fprintf( stderr, PP ": Value at key '%s.%s' at %s was not a string or number.\n", val->val, kv->key.v.vchar, arg.luatest );
						return 1;
					}

					if ( !( a = malloc( alen ) ) || !memset( a, 0, alen ) ) {
						fprintf( stderr, PP ": Failed to allocate space for record in %s table in file %s.\n", val->val, arg.luatest );
						return 1;
					}

					if ( kv->value.type == ZTABLE_INT )	
						snprintf( a, 2047, "%s=%d", kv->key.v.vchar, kv->value.v.vint );
					else {
						snprintf( a, 2047, "%s=%s", kv->key.v.vchar, kv->value.v.vchar );
					} 

					add_item( val->array, a, unsigned char *, &val->alen );
				}
				lt_free( tt );
			}
		}


		//Extract expects and status, etc
		if ( ( i = lt_geti( test.table, "expects" ) ) > -1 ) {
			ztable_t *t = lt_copy_by_index( test.table, i );

			//Get the expected status if there is one
			if ( ( i = lt_geti( t, "expects.status" ) ) > -1 ) {
				test.expected.status = lt_int_at( t, i );
			}
		
			//Get the expected content-type if there is one
			if ( ( i = lt_geti( t, "expects.ctype" ) ) > -1 ) {
				test.expected.ctype = lt_text_at( t, i );
			}

			//Get the expected content-length if there is one
			if ( ( i = lt_geti( t, "expects.clen" ) ) > -1 ) {
				test.expected.clength = lt_int_at( t, i );
			}
					
			//Get the expected payload if there is one
			if ( ( i = lt_geti( t, "expects.payload" ) ) > -1 ) {
				if ( test.expected.clength )
					test.expected.payload = lt_blob_at( t, i ).blob;
				else {
					test.expected.payload = (unsigned char *)lt_text_at( t, i );
					test.expected.clength = strlen( lt_text_at( t, i ) );
				}
			}
	
			lt_free( t );
		}
		lt_free( test.table ), free( test.table );
	#endif
	}

	if ( !arg.path ) {
		fprintf( stderr, PP ": Directory to application not specified.\n" );
		return 1;
	}

	if ( !arg.uri )
		arg.uri = "/";
	else if ( *arg.uri != '/' ) {
		fprintf( stderr, PP ": URI is unspecified (only specify what comes after the domain).\n" );
		return 1;
	}

#else
	if ( !arg.lib ) {
		fprintf( stderr, PP ": No library specified.\n" );
		return 1;
	} 

	//Load the app, find the symbol and run the code...
	if ( !( app = dlopen( arg.lib, RTLD_LAZY ) ) ) {
		fprintf( stderr, PP ": Could not open application at %s: %s.\n", arg.lib, dlerror() );
		return 1;
	}

	//Find the name, since that will cause problems if not specified
	if ( !( fname = dlsym( app, NSYMBOL ) ) ) {
		dlclose( app );
		fprintf( stderr, PP ": symbol '%s' not found in filter: %s\n", NSYMBOL, strerror(errno) );
		return 1;
	} 

	//Find the main function symbol
	if ( !( filter = dlsym( app, FSYMBOL )) ) {
		dlclose( app );
		fprintf( stderr, PP ": symbol '%s' not found in filter: %s\n", FSYMBOL, strerror(errno) );
		return 1;
	}
#endif

	//Populate the request structure.  Normally, one will never populate this from scratch
	req.path = arg.uri;
		
	//TODO: When coming from Lua file, all of this will result in a leak... 
	req.ctype = !arg.ctype ? "text/html" : arg.ctype;
	req.host = !arg.host ? "example.com" : arg.host;
	req.method = arg.method;
	req.protocol = !arg.protocol ? "HTTP/1.1" : arg.protocol;

#if 0
	req.alias = !arg.alias ? zhttp_dupstr( "example.com" ) : zhttp_dupstr( arg.alias );
#endif

	//TODO: This is damned ugly...
	//Add any extra headers
	if ( arg.headers ) { 
		for ( char **headers = arg.headers; *headers; headers++ ) {
			if ( index( *headers, ':' ) ) { 
				int a = 0, b = 0, eq = 0;
				char aa[ 1024 ] = { 0 }, bb[ 1024 ] = { 0 };
				for ( char *header = *headers; *header; header++ ) {
					if ( !( eq += ( *header == ':' ) ) ) 
						aa[ a++ ] = *header;
					else {
						*header != ':' ? bb[ b++ ] = *header : 0;
					}
				}
				http_copy_header( &req, aa, bb );
			}
		}
		free( arg.headers ); 
	}

	//Also need to add GET vars from URI
	//TODO: Single char values do not work... strangely enough
	for ( char *ww = index( arg.uri, '?' ); ww ; ) {
		char aa[ 1024 ] = {0}, bb[ 1024 ] = {0};
		zWalker w = {0};
		ww++;

		for ( int len; strwalk( &w, ww, "&=" ); ) {
			len = ( w.chr == '=' || w.chr == '&' ) ? w.size	- 1 : w.size;
			if ( w.chr == '=' ) 
				memcpy( aa, &ww[ w.pos ], len );
			else {
				memcpy( bb, &ww[ w.pos ], len );
				http_copy_uripart( &req, aa, bb );
				memset( aa, 0, sizeof( aa ) );
				memset( bb, 0, sizeof( bb ) );
			}
		}

		break;
	}  

	//POST, PUT & DELETE typically expect a body...
	if ( method_expects_body( req.method ) && arg.body ) {
		//Make it multipart if requested
		if ( arg.multipart ) {
			//Need to free the original ctype
			if ( req.ctype ) {
				free( req.ctype );
			}
			req.ctype = "multipart/form-data";
		}

		//Save all bodies 
		for ( char **body = arg.body; *body; body++ ) {
			if ( index( *body, '=' ) ) { 
				int a = 0, b = 0, eq = 0, bblen = 0;
				char aa[ 1024 ] = {0};
				unsigned char *bb = NULL;
				for ( char *value = *body; *value; value++ ) {
					if ( !( eq += ( *value == '=' ) ) ) 
						aa[ a++ ] = *value;
					else if ( *value != '=' ) {
						if ( *value != '@' ) {
							bb = (unsigned char *)value, bblen = strlen( value );	
							http_copy_formvalue( &req, aa, bb, bblen );
						}
						else { 
							if ( !( bb = read_file( ++value, &bblen, err, sizeof( err ) ) ) ) {
								fprintf( stderr, PP ": couldn't open test file: %s\n", value );	
								//TODO: Things should be freed here...
								return 1;
							}
							http_copy_formvalue( &req, aa, bb, bblen );
							free( bb );
						}
						break;
					}
				}
			}
		}
		free( arg.body ); 
	}

	//Assemble a message from here...
	if ( !http_finalize_request( &req, err, sizeof( err ) ) ) {
		fprintf( stderr, "%s\n", err );
		return 1; 
	}

	//Stop and dump the request
	if ( arg.dumpHttp ) {
		print_httpbody( &req );
		write( 2, req.msg, req.mlen );
		http_free_request( &req );
		return 0;
	}

	//Mock the site config data (or populate from file)
	sconf.name = req.host;
	sconf.alias = "";
	sconf.dir = arg.path;
	sconf.filter = fname;
	sconf.root_default = "";

	//Mock the connection data
	conn.count = 0;
	conn.status = 0;
	conn.ctx = NULL;
	conn.config = NULL;
	conn.hconfig = &sconf;
	conn.ipv4 = "192.168.0.1";
	//conn.ipv6 = '192.168.0.1';

	//Open any needed files (dying if you fail to do so)
	if ( arg.headerf ) {
		if ( *arg.headerf	== '-' )
			header_fd = 1;	
		else if ( ( header_fd = open( arg.headerf, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ) == -1 ) {
			fprintf( stderr, PP ": Opening header file %s - %s", arg.headerf, strerror( errno ) ); 
			http_free_request( &req );
			http_free_response( &res );
			return 1;
		}
	}

	if ( arg.bodyf ) {
		if ( *arg.bodyf	== '-' )
			body_fd = 1;	
		else if ( ( body_fd = open( arg.bodyf, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH ) ) == -1 ) {
			fprintf( stderr, PP ": Opening body file %s - %s", arg.bodyf, strerror( errno ) ); 
			http_free_request( &req );
			http_free_response( &res );
			return 1;
		}
	}

	//
	int status = filter( 0, &req, &res, &conn );

#if 0
	//A failure isn't technically a failure...  it could be a 400, and this could be exactly what's supposed to happen...
	if ( !status ) {
		fprintf( stderr, PP ": HTTP funct '%s' failed to execute\n", FSYMBOL );
		//2 is the device I should write to...
		write( 1, res.msg, res.mlen );
		fflush( stdout );
		http_free_request( &req );
		http_free_response( &res );
		return 1;
	}
#endif
	
	//The Lua test
	if ( arg.luatest ) {
		//If the table still exists, you need to run against the expects key		
		if ( !test.table ) {
			fprintf( stderr, PP ": Lua table was freed too soon\n" ); 
			return 1;
		}

		//Print the site and URL
		fprintf( stdout, "%20s:%s\nExpected %d; Got %d\n", 
			arg.uri, arg.path, test.expected.status, res.status );
		fflush( stdout );

		//Just show the status
		if ( test.expected.status != res.status ) {
			write( 2, res.msg, res.mlen );
			fflush( stderr );
		}

		//Dump the response
		if ( arg.dumpResp ) {
			write( 1, res.msg, res.mlen );
			fflush( stdout );
		}

		return 0;	
	}

	//Show whatever message should have come out
	if ( arg.headerf ) {
		write( header_fd, res.msg, res.mlen - res.clen );
	}

	if ( arg.msgonly || arg.bodyf ) {
		int sp = res.mlen - res.clen;	
		write( body_fd, &res.msg[ sp ], res.mlen - sp );
	}
	else if ( !arg.headerf ) {
		write( body_fd, res.msg, res.mlen );
		fflush( stdout );
	}

#if 0
	if ( arg.lib && dlclose( app ) == -1 ) {
		http_free_request( &req );
		http_free_response( &res );
		fprintf( stderr, PP ": Failed to close application: %s\n", strerror( errno ) );
		return 1;
	}
#else
	http_free_request( &req );
	http_free_response( &res );
#endif

	//Close files
	if ( header_fd > 2 ) {
		close( header_fd );
	}

	if ( body_fd > 2 ) {
		close( body_fd );
	}

	//After we're done, look at the response
	return 0;
}

