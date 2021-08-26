/* ------------------------------------------- * 
 * http.c 
 * ====
 * 
 * Summary 
 * -------
 * Handle any web request from Lua (w/o cURL).
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 *
 * See LICENSE in the top-level directory for more information.
 *
 * TODO
 * ----
 * - Only handles GET right now.  Needs other methods.
 * - Consider merging with zhttp to enable packaging responses.
 * - Allow alternate SSL backends. (at least OpenSSL)
 * - Test with http://etc.com:2000 (port numbers)
 * 
 * CHANGELOG 
 * ---------
 * -
 * ------------------------------------------- */
#include "http.h"

#define ERR(v,l,...) \
	snprintf( v, l, __VA_ARGS__ ) && 0

/*Write defines here*/
//#define SHOW_RESPONSE
//#define WRITE_RESPONSE
#define SHOW_REQUEST
#define VERBOSE 
//#define SSL_DEBUG
//#define INCLUDE_TIMEOUT
/*Stop*/

#ifndef SSL_DEBUG 
 #define SSLPRINTF( ... )
 #define EXIT(x)
 #define MEXIT(x,m)
 #define RUN(c) (c)
#else
 #define SSLPRINTF( ... ) fprintf( stderr, __VA_ARGS__ ) ; fflush(stderr);
 #define EXIT(x) \
	fprintf(stderr,"Forced Exit.\n"); exit(x);
 #define MEXIT(x,m) \
	fprintf(stderr,m); exit(x);
 #define RUN(c) \
  (c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#endif

#ifndef VERBOSE
 #define VPRINTF( ... )
#else
 #define VPRINTF( ... ) fprintf( stderr, __VA_ARGS__ ) ; fflush(stderr);
#endif


//User-Agent
static const char default_ua[] = 
	"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36";


//base16 decoder
static int radix_decode( char *number, int radix ) {
	const int hex[127] = {['0']=0,1,2,3,4,5,6,7,8,9,
		['a']=10,['b']=11,['c']=12,13,14,15,['A']=10,11,12,13,14,15};
	int tot=0, mult=1, len=strlen(number);
	number += len; //strlen( digits ); 
	while ( len-- ) {
		//TODO: cut ascii characters not in a certain range...
		tot += ( hex[ (int)*(--number) ] * mult);
		mult *= radix;
	}

	return tot;
}


//Extracts the HTTP message body from the message
int extract_body ( wwwResponse *r ) {
	//when sending this, we have to skip the header
	const char *t = "\r\n\r\n";
	uint8_t *nsrc = r->data; 
	uint8_t *rr = malloc(1);
	int pos, tot=0, y=0, br = r->len;

	if (( pos = memstrat( r->data, t, br ) ) == -1 ) {
		fprintf( stderr, "carriage return not found...\n" ); 
		return 0;
	}

	//Move ptrs and increment things, this is the start of the headers...
	pos += 4;
	nsrc += pos; 
	br -= pos;
	t += 2;
	r->body = &r->data[ pos ];

	//chunked
	if ( !r->chunked ) {
		return 1;
	} 

	//find the next \r\n, after reading the bytes
	while ( 1 ) {
		//define a buffer for size
		//search for another \r\n, since this is the boundary
		int sz, szText;
		char tt[ 64 ];
		memset( &tt, 0, sizeof(tt) );
		if ((szText = memstrat( nsrc, t, br )) == -1 ) {
			break;
		}
		memcpy( tt, nsrc, szText );
		sz = radix_decode( tt, 16 );
	
		//move up nsrc, and get ready to read that
		nsrc += szText + 2;
		rr = realloc( rr, tot + sz );
		memset( &rr[ tot ], 0, sz );
		memcpy( &rr[ tot ], nsrc, sz );

		//modify digits
		nsrc += sz + 2, /*move up ptr + "\r\n" and "\r\n"*/
		tot += sz,
		br -= sz + szText + 2;

		//this has to be finalized as well
		if ( br == 0 ) {
			break;
		}
	}

#if 0
	r->body = tt;
	r->clen = tot;
#else
	//Trade pointers and whatnot
	int ndSize = pos + strlen("\r\n\r\n") + tot;
	uint8_t *newData = malloc( ndSize );
	memset( newData, 0, ndSize );
	memcpy( &newData[ 0 ], r->data, ndSize - tot );
	memcpy( &newData[ ndSize - tot ], rr, tot );

	//Free stuff
	free( r->data );
	free( rr  );

	//Set stuff
	r->data = NULL;	
	r->data = newData;
	r->len  = ndSize;
	r->body = &newData[ ndSize - tot ];
	r->clen = tot;
#endif
	return 1;
}


#if 0
//16 ^ 0 = 1    = n * ((16^0) or 1) = n
//16 ^ 1 = 16   = n * ((16^1) or 16) = n
//16 ^ 2 = ...
//printf("%d\n", n ); getchar();
int main() {
int p;
struct intpair { char *c; int i; } tests[] = {
 { NULL }
,{ "fe",  254 }	
,{ "c",   12	}
,{ "77fe",30718 }
,{ "87fe",34814 }
#if 0
,{ "87fecc1c",2281622556}
#endif
,{ NULL	}
};
struct intpair *a = tests;

while ( (++a)->c ) 
	printf("%d == %d = %s\n", 
	p=radix_decode( a->c, 16 ),a->i,p==a->i?"true":"false");
exit( 0);
#endif

#if 0
//Write data to some kind of buffer with something
static size_t WriteDataCallbackCurl (void *p, size_t size, size_t nmemb, void *ud) {
	size_t realsize = size * nmemb;
	Sbuffer *sb = (Sbuffer *)ud;
	uint8_t *ptr = realloc( sb->buf, sb->len + realsize + 1 ); 
	if ( !ptr ) {
		fprintf( stderr, "No additional memory to complete request.\n" );
		return 0;
	}
	sb->buf = ptr;
	memcpy( &sb->buf[ sb->len ], p, realsize );
	sb->len += realsize;
	sb->buf[ sb->len ] = 0;
	return realsize;
}
#endif


//
void timer_handler (int signum) {
	static int count =0;
	fprintf( stdout, "timer expired %d times\n", ++count );
	//when sockets die, they die
	//no cleanup, this is bad, but I can deal with it for now, and even come up with a way to clean up provided data strucutres are in one place...
	exit( 0 );	
}

  // get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


static int get_content_length (char *msg, int mlen) {
	int pos;
	if ( (pos = memstrat( msg, "Content-Length", mlen )) == -1 ) { 
		return -1;
	}

	int r = memchrat( &msg[ pos ], '\r', mlen - pos );	
	int s = memchrat( &msg[ pos ], ' ', mlen - pos );	
	char lenString[ 24 ] = { 0 };
	memcpy( lenString, &msg[ pos + (s+1) ], r - s );

	//Return this if not found.
	return atoi( lenString );
}


static char * get_content_type (char *dest, char *msg, int mlen) {
	int pos;
	if ( (pos = memstrat( msg, "Content-Type", mlen )) == -1 ) { 
		return NULL;
	}

	//TODO: Make this safer
	int r = memchrat( &msg[ pos ], '\r', mlen - pos );	
	int s = memchrat( &msg[ pos ], ' ', mlen - pos );	
	memcpy( dest, &msg[ pos + (s+1) ], r - s );
	//fprintf( stderr, "content-type: %s\n", ctString );exit(0);
	//return mti_geti( ctString );
	return dest;
}



static int get_status (char *msg, int mlen) {
	//NOTE: status line will not always have an 'OK'
	const char *ok[] = {
		"HTTP/0.9 200"
	, "HTTP/1.0 200"
	, "HTTP/1.1 200"
	, "HTTP/2.0 200"
	, NULL
	};
	const char **lines = ok;
	int stat = 0;
	int status = 0;

	//just safe?
	while ( *lines ) {
		if ( memcmp( msg, *lines, strlen(*lines) ) == 0 ) {
			stat = 1;
			char statWord[ 10 ] = {0};
			memcpy( statWord, &msg[ 9 ], 3 );
			return atoi( statWord );
			break;
		}
		lines++;
	}

	return -1;
}


#ifdef DEBUG_H
void print_www ( wwwResponse *r ) {
	fprintf( stderr, "status: %d\n", r->status );
	fprintf( stderr, "len:    %d\n", r->len );
	fprintf( stderr, "clen:   %d\n", r->clen );
	fprintf( stderr, "ctype:  %s\n", r->ctype );
	fprintf( stderr, "chunked:%d\n", r->chunked );
	fprintf( stderr, "redrUri:%s\n", r->redirect_uri	 );
#if 0
	fprintf( stderr, "\nbody\n" ); write( 2, r->body, 200 );
		freeaddrinfo( servinfo );
	fprintf( stderr, "\ndata\n" ); write( 2, r->data, 400 );
#endif
}
#endif



//Use int pointers
static int select_www( const char *addr, int *port, int *secure ) {
	//Checking for secure or not...
	//you can extract the root from here...
	if ( memcmp( "https", addr, 5 ) == 0 )
		*secure = 1, *port = 443;
	else if ( memcmp( "http", addr, 4 ) == 0 )
		*secure = 0, *port = 80;
	else {
		return 0;
	}

	return 1;
}



char * path_www ( const char *p ) {
	return NULL;
}


const char GetMsgFmt[] = 
	"GET %s HTTP/1.1\r\n"
	"Host: %s\r\n"
	"User-Agent: %s\r\n\r\n"
;



//Get a new fd
int get_fd ( ) {
	return 0;
}



int make_http_request ( const char *p, int port, zhttp_t *r, zhttp_t *res, char *errmsg, int errlen ) {
	//socket connect is the shorter way to do this...
	struct addrinfo hints, *servinfo, *pp;
	int rv, sockfd;
	char b[10] = {0}, s[ INET6_ADDRSTRLEN ], ipv4[ INET_ADDRSTRLEN ];
	snprintf( b, sizeof(b), "%d", port );	
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Prepare our response structure
	memset( res, 0, sizeof( zhttp_t ) );

#ifdef INCLUDE_TIMEOUT 
	//Set up a timer to kill requests that are taking too long.
	//TODO: There is an alternate way to do with with socket(), I think
	struct itimerval timer;
	memset( &timer, 0, sizeof(timer) );
#if 0
	signal( SIGVTALRM, timer_handler );
#else
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = &timer_handler;
	sigaction( SIGVTALRM, &sa, NULL );
#endif
	//Set timeout to 3 seconds (remember: no cleanup takes place...)
	timer.it_interval.tv_sec = 0;	
	timer.it_interval.tv_usec = 0;
	//NOTE: For timers to work, both it_interval and it_value must be filled out...
	timer.it_value.tv_sec = 2;	
	timer.it_value.tv_usec = 0;
	//if we had an interval, we would set that here via it_interval
	if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
		snprintf( errmsg, errlen, "timeout set error: %s\n", strerror( errno ) );
		return 0;
	}
#endif

	//Get the address info of the domain here.
	//TODO: Use Bind or another library for this.  Apparently there is no way to detect timeout w/o using a signal...
	if ( ( rv = getaddrinfo( r->host, b, &hints, &servinfo ) ) != 0 ) {
		snprintf( errmsg, errlen, "%s => %s", gai_strerror( rv ), r->host );
		return 0;
	}

	//Loop and find the right address
	for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
		if ( ( sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol ) ) == -1) {	
			snprintf( errmsg, errlen, "client socket error: %s", strerror( errno ) );
			continue;
		}

		//fprintf(stderr, "%d\n", sockfd);
		if ( connect( sockfd, pp->ai_addr, pp->ai_addrlen) == -1 ) {
			close( sockfd );
			snprintf( errmsg, errlen, "client connect error: %s", strerror( errno ) );
			continue;
		}

		break;
	}

	//If we completely failed to connect, do something.
	if ( pp == NULL ) {
		snprintf( errmsg, errlen, "client: failed to connect" );
		return 1;
	}

	//This is some weird stuff...
#ifdef INCLUDE_TIMEOUT 
	struct sigaction da;
	da.sa_handler = SIG_DFL;
	sigaction( SIGVTALRM, &da, NULL );
#endif

	//Get the internet address
	inet_ntop( pp->ai_family, 
		get_in_addr((struct sockaddr *)pp->ai_addr), ipv4, sizeof( ipv4 ));
	freeaddrinfo( servinfo );

	unsigned char *msg = NULL, *bmsg = r->msg;
	int sb = 0, rb = 0, mlen = r->mlen;
	int crlf = -1, first = 0;

	//Must account for the data sent.
	for ( int b; ; ) {
		//These should be blocking, so this is probably a legitimate error
		if ( ( b = send( sockfd, bmsg, mlen, 0 )) == -1 ) {
			snprintf( errmsg, errlen, 
				"Error sending mesaage to: %s - %s\n", s, strerror(errno) );
			return 0;
		}
	
		if ( ( sb += b ) != mlen ) {
			continue;
		}

		break;
	}

	//WE most likely will receive a very large page... so do that here...	
#if 0
	if ( !( msg = malloc( 16 ) ) || !memset( msg, 0, 16 ) ) {
		return ERR( errmsg, errlen, "%s\n", "Allocation failure." );
	} 
#endif

	for ( int blen = 0, chunked = 0;; ) {
		uint8_t xbuf[ 4096 ] = {0};

		if ( ( blen = recv( sockfd, xbuf, sizeof(xbuf), 0 ) ) == -1 ) {
			//SSLPRINTF( "Error receiving mesaage to: %s\n", s );
			break;
		}
		else if ( !blen ) {
			//SSLPRINTF( "%s\n", "No new bytes sent.  Jump out of loop." );
			break;
		}

		if ( !first++ ) {
			res->status = get_status( (char *)xbuf, blen );
			res->clen = get_content_length( (char *)xbuf, blen );
			if ( !get_content_type( r->ctype, (char *)xbuf, blen ) ) {
				return 0;
			}

			chunked = memstrat( xbuf, "Transfer-Encoding: chunked", blen ) > -1;
			if ( chunked ) {
				SSLPRINTF( "%s\n", "Chunked not implemented for HTTP" );
				return ERR( errmsg, errlen, "%s\n", "Chunked not implemented for HTTP" );
			}
			if (( crlf = memstrat( xbuf, "\r\n\r\n", blen ) ) == -1 ) {
				0;//this means that something went wrong... 
			}
			//fprintf(stderr, "found content length and eoh: %d, %d\n", r->clen, crlf );
			crlf += 4;
			//write( 2, xbuf, blen );
		}

		//read into a bigger buffer	
		if ( !( res->msg = realloc( res->msg, res->mlen + blen ) ) ) {
			return ERR( errmsg, errlen, "%s\n", "Realloc of destination buffer failed." );
		}

		memcpy( &res->msg[ res->mlen ], xbuf, blen ); 
		res->mlen += blen;

		if ( !res->clen )
			return ERR( errmsg, errlen, "%s\n", "No length specified, parser error!." );
		else if ( res->clen && ( res->mlen - crlf ) == res->clen ) {
			SSLPRINTF( "Full HTTP message received\n" );
			break;
		}
	}

	//Close the fd
	if ( close( sockfd ) == -1 ) {
		//Report the error regardless
	}

	return 1;
}


#if 0
int make_https_request ( const char *p, int port, zhttp_t *r, zhttp_t *res, char *errmsg, int errlen ) {
#if 1
	//Define
	gnutls_session_t session;
	gnutls_datum_t out;
	gnutls_certificate_credentials_t xcred;
	int ret, type, err = 0;
	unsigned int status;

	//Initialize
	memset( &session, 0, sizeof(gnutls_session_t));
	memset( &xcred, 0, sizeof(gnutls_certificate_credentials_t));

	//Do socket connect (but after initial connect, I need the file desc)
	if ( RUN( !gnutls_check_version("3.4.6") ) )
		return ERR( errmsg, errlen, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

	if ( RUN( ( err = gnutls_global_init() ) < 0 ) )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( RUN( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 ))
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( RUN( (err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 ))
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	//Set client certs this way...
	//gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );

	//Initialize gnutls and set things up
	if ( RUN( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 ))
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( RUN( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, r->host, strlen(r->host)) ) < 0))
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	if ( RUN( ( err = gnutls_set_default_priority( session ) ) < 0) )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));
	
	if ( RUN( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) < 0) )
		return ERR( errmsg, errlen, "%s\n", gnutls_strerror( err ));

	//Set random handshake details
	gnutls_session_set_verify_cert( session, r->host, 0 );

#if 0
	//socket connect is the shorter way to do this...
#else
	//socket connect is the shorter way to do this...
	struct addrinfo hints, *servinfo, *pp;
	int rv, sockfd;
	char b[10] = {0}, s[ INET6_ADDRSTRLEN ], ipv4[ INET_ADDRSTRLEN ];
	snprintf( b, sizeof(b), "%d", port );	
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	//Prepare our response structure
	memset( res, 0, sizeof( zhttp_t ) );

#ifdef INCLUDE_TIMEOUT 
	//Set up a timer to kill requests that are taking too long.
	//TODO: There is an alternate way to do with with socket(), I think
	struct itimerval timer;
	memset( &timer, 0, sizeof(timer) );
#if 0
	signal( SIGVTALRM, timer_handler );
#else
	struct sigaction sa;
	memset( &sa, 0, sizeof(sa) );
	sa.sa_handler = &timer_handler;
	sigaction( SIGVTALRM, &sa, NULL );
#endif
	//Set timeout to 3 seconds (remember: no cleanup takes place...)
	timer.it_interval.tv_sec = 0;	
	timer.it_interval.tv_usec = 0;
	//NOTE: For timers to work, both it_interval and it_value must be filled out...
	timer.it_value.tv_sec = 2;	
	timer.it_value.tv_usec = 0;
	//if we had an interval, we would set that here via it_interval
	if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
		snprintf( errmsg, errlen, "timeout set error: %s\n", strerror( errno ) );
		return 0;
	}
#endif

	//Get the address info of the domain here.
	//TODO: Use Bind or another library for this.  Apparently there is no way to detect timeout w/o using a signal...
	if ( ( rv = getaddrinfo( r->host, b, &hints, &servinfo ) ) != 0 ) {
		snprintf( errmsg, errlen, "%s => %s", gai_strerror( rv ), r->host );
		return 0;
	}

	//Loop and find the right address
	for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
		if ( ( sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol ) ) == -1) {	
			snprintf( errmsg, errlen, "client socket error: %s", strerror( errno ) );
			continue;
		}

		//fprintf(stderr, "%d\n", sockfd);
		if ( connect( sockfd, pp->ai_addr, pp->ai_addrlen) == -1 ) {
			close( sockfd );
			snprintf( errmsg, errlen, "client connect error: %s", strerror( errno ) );
			continue;
		}

		break;
	}

	//If we completely failed to connect, do something.
	if ( pp == NULL ) {
		snprintf( errmsg, errlen, "client: failed to connect" );
		return 1;
	}

	//This is some weird stuff...
#ifdef INCLUDE_TIMEOUT 
	struct sigaction da;
	da.sa_handler = SIG_DFL;
	sigaction( SIGVTALRM, &da, NULL );
#endif

	//Get the internet address
	inet_ntop( pp->ai_family, 
		get_in_addr((struct sockaddr *)pp->ai_addr), ipv4, sizeof( ipv4 ));
	freeaddrinfo( servinfo );
#endif

	//Set up GnuTLS to read things
	gnutls_transport_set_int( session, sockfd );
	gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

	//Do the first handshake
	do {
		 RUN( ret = gnutls_handshake( session ) );
	} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

	//Check the status of said handshake
	if ( ret < 0 ) {
		//fprintf( stderr, "ret: %d\n", ret );
		if ( RUN( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) ) {	
			type = gnutls_certificate_type_get( session );
			status = gnutls_session_get_verify_cert_status( session );
			err = gnutls_certificate_verification_status_print( status, type, &out, 0 );
			fprintf( stdout, "cert verify output: %s\n", out.data );
			gnutls_free( out.data );
			//jump to end, but I don't do go to
		}
		return ERR( errmsg, errlen, "Handshake failed: %s\n", gnutls_strerror( ret ));
	}
	else {
		//desc = gnutls_session_get_desc( session );		
		//consider dumping the session info here...
		//fprintf( stdout, "- Session info: %s\n", desc );
		//gnutls_free( desc );
	}

	//TODO: This needs to be a large loop...
	if ( RUN(( err = gnutls_record_send( session, r->msg, r->mlen ) ) < 0 ) ) {
		return ERR( errmsg, errlen, "%s", "GnuTLS 3.4.6 or later is required for this example." );	
	}

	//This is a sloppy quick way to handle EAGAIN
	int crlf = -1, first = 0, chunked = 0;
	#if 0
	if ( !( msg = malloc( 16 ) ) || !memset( msg, 0, 16 ) ) {
		return ERR( r->err, "%s\n", "Allocation failure." );
	} 
	#endif

	//there should probably be another condition used...
	for ( ;; ) {	
		char xbuf[ 4096 ] = {0};
		int ret = gnutls_record_recv( session, xbuf, sizeof(xbuf)); 
		SSLPRINTF( "gnutls_record_recv returned %d\n", ret );

		//receive
		if ( !ret ) {
			SSLPRINTF( "Peer has closed the TLS Connection\n" );
			break;
		}
		else if ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
			SSLPRINTF( "Warning: %s\n", gnutls_strerror( ret ) );
			continue;
		}
		else if ( ret < 0 ) {
			SSLPRINTF( "Error: %s\n", gnutls_strerror( ret ) );
			break;	
		}
		else if ( ret > 0 ) {
			SSLPRINTF( "Recvd %d bytes:\n", ret );
			SSLPRINTF( "AM i HERE?!\n" );
			if ( !first++ ) {

				//Get status, content-length or xfer-encoding if available 
				char ctypebuf[ 128 ] = {0};
				res->status = get_status( (char *)xbuf, ret );
				res->clen = get_content_length( xbuf, ret );
				if ( get_content_type( ctypebuf, (char *)xbuf, ret ) ) {
					//I feel like content-type should always be specified...
					//But you can get something smaller...?  or zero-length?
					res->ctype = zhttp_dupstr( ctypebuf );
				}
				chunked = memstrat( xbuf, "Transfer-Encoding: chunked", ret ) > -1;
				if ( ( crlf = memstrat( xbuf, "\r\n\r\n", ret )) == -1 ) {
					SSLPRINTF( "%s\n", "No CRLF sequence found, response malformed." );
					return ERR( errmsg, errlen, "%s\n", "No CRLF sequence found, response malformed." );
				}
				//Increment the crlf by the length of "\r\n\r\n"	
				crlf += 4;
				if ( chunked ) {
					char *m = &xbuf[ crlf ];
					//parse the chunked length
					int lenp = memstrat( m, "\r\n", ret - crlf );	
					//to save chunk length, I need to convert to hex to decimal
					char lenpxbuf[ 10 ] = {0};
					memcpy( lenpxbuf, m, lenp );
					int sz = 0; //atoi( lenpxbuf );
				}
			}
			SSLPRINTF( "Did I get HERE?!\n" );

			//Finalize chunked messages.
			if ( chunked && ret == 5 ) {
				if ( memcmp( xbuf, "0\r\n\r\n", 5 ) == 0 ) {
					//fprintf(stderr, "the last one came in" );
					//fprintf( stderr, "%s, %d: my code ran.... but why stop?", __FILE__, __LINE__ ); 
					break;
				}
				else {
					fprintf(stderr, "not sure what happened.." );
				}
			}

//fprintf( stderr, "msg size: %d, add to msg: %d\n", res->mlen, ret + 1 );
//write( 2, xbuf, ret );

			//Read into a bigger buffer	
			if ( !( res->msg = realloc( res->msg, res->mlen + ( ret + 1 ) ) ) ) {
				SSLPRINTF( "%s\n", "Realloc of destination buffer failed." );
				return ERR( errmsg, errlen, "%s\n", "Realloc of destination buffer failed." );
			}

			memcpy( &res->msg[ res->mlen ], xbuf, ret ), res->mlen += ret;

			//If it's chunked, try sending a 100-continue
			if ( chunked ) {
				const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
				//int err = gnutls_record_send( session, cont, strlen(cont)); 
				fprintf(stderr, "%s (%d)\n", gnutls_strerror( err ), err );
			}
			else {
				if ( !res->clen ) {
					SSLPRINTF( "%s\n", "No length specified, parser error!." );
					return ERR( errmsg, errlen, "%s\n", "No length specified, parser error!." );
				}
				else if ( res->clen && ( res->mlen - crlf ) == res->clen ) {
					SSLPRINTF( "Full message received: clen: %d, mlen: %d", res->clen, res->mlen );
					break;
				}
				SSLPRINTF( "recvd: %d , clen: %d , mlen: %d\n", res->mlen - crlf, res->clen, res->mlen );
			}
		}
	} /* end while */

	//TODO: Unsure why this is here or what it really does...
	int tries=0;
	while ( tries++ < 3 && ( err = gnutls_bye(session, GNUTLS_SHUT_WR) ) != GNUTLS_E_SUCCESS ) ;
	if ( err != GNUTLS_E_SUCCESS ) {
		return ERR( errmsg, errlen, "%s\n",  gnutls_strerror( ret ) );
	}

	//Close the file and free all of GnuTLS's structures
	if ( close( sockfd ) == -1 ) {

	}
	gnutls_deinit( session );
#endif
	return 1;
}
#endif

//Load webpages via HTTP/S
#if 0
int load_www ( const char *p, wwwResponse *r ) {
#else
int load_www ( const char *p, zhttp_t *r, char *errmsg, int errlen ) {
#endif
	int err, ret, sd, ii, type, len, sockfd, c = 0, chunked = 0, port = 0, secure = 0;
	unsigned int status;
	uint8_t *msg = NULL;
	char *desc = NULL, buf[ 4096 ] = { 0 }, GetMsg[2048] = { 0 }, rootBuf[ 128 ] = { 0 };
	const char *fp = NULL, *site = NULL, *urlpath = NULL;

#if 0
	//Pack a message
	if ( port != 443 )
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, root, ua );
	else {
		char hbbuf[ 128 ] = { 0 };
		//snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", root, t->port );
		snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s:%d", root, port );
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, hbbuf, ua );
	}
#endif
	
#if 0
	//NOTE: Although it is definitely easier to use CURL to handle the rest of the 
	//request, dealing with TLS at C level is more complicated than it probably should be.  
	//Do either an insecure request or a secure request
	if ( !secure ) {
		//socket connect is the shorter way to do this...
		struct addrinfo hints, *servinfo, *pp;
		int rv;
		char b[10] = {0}, s[ INET6_ADDRSTRLEN ];
		snprintf( b, sizeof(b), "%d", port );	
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

	#ifdef INCLUDE_TIMEOUT 
		//Set up a timer to kill requests that are taking too long.
		//TODO: There is an alternate way to do with with socket(), I think
		struct itimerval timer;
		memset( &timer, 0, sizeof(timer) );
	#if 0
		signal( SIGVTALRM, timer_handler );
	#else
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa) );
		sa.sa_handler = &timer_handler;
		sigaction( SIGVTALRM, &sa, NULL );
	#endif
		//Set timeout to 3 seconds (remember: no cleanup takes place...)
		timer.it_interval.tv_sec = 0;	
		timer.it_interval.tv_usec = 0;
		//NOTE: For timers to work, both it_interval and it_value must be filled out...
		timer.it_value.tv_sec = 2;	
		timer.it_value.tv_usec = 0;
		//if we had an interval, we would set that here via it_interval
		if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
			fprintf( stderr, "Set Timer Error: %s\n", strerror( errno ) );
			return 0;
		}
	#endif

		//Get the address info of the domain here.
		//TODO: Use Bind or another library for this.  Apparently there is no way to detect timeout w/o using a signal...
		if ( ( rv = getaddrinfo( root, b, &hints, &servinfo ) ) != 0 ) {
			fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( rv ) );
			return 0;
		}

		//Loop and find the right address
		for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
			if ( ( sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol ) ) == -1) {	
				fprintf(stderr,"client: socket error: %s\n",strerror(errno));
				continue;
			}

			//fprintf(stderr, "%d\n", sockfd);
			if ( connect( sockfd, pp->ai_addr, pp->ai_addrlen) == -1 ) {
				close( sockfd );
				fprintf(stderr,"client: connect: %s\n",strerror(errno));
				continue;
			}

			break;
		}

		//If we completely failed to connect, do something.
		if ( pp == NULL ) {
			fprintf(stderr, "client: failed to connect\n");
			return 1;
		}

		//This is some weird stuff...
	#ifdef INCLUDE_TIMEOUT 
		struct sigaction da;
		da.sa_handler = SIG_DFL;
		sigaction( SIGVTALRM, &da, NULL );
	#endif

		//Get the internet address
		inet_ntop( pp->ai_family, 
			get_in_addr((struct sockaddr *)pp->ai_addr), r->ipv4, sizeof(r->ipv4));
		#ifdef DEBUG_H
    fprintf( stderr,"Client connected to: %s\n", r->ipv4 );
		#endif
		freeaddrinfo( servinfo );

		int sb = 0, rb = 0;
	#if 1
		//This is the easy way
		char *req = GetMsg, mlen = strlen( req );
	#else

	#endif

		//And do the send and read?
		//If we send a lot of data, like a giant post, then this should be reflected here...
		for ( ;; ) {	
			int b;
			if ( ( b = send( sockfd, req, mlen, 0 )) == -1 ) {
				fprintf(stderr, "Error sending mesaage to: %s\n", s );
			}
		
			if ( ( sb += b ) != mlen ) {
				fprintf(stderr, "%d total bytes sent\n", sb );
				continue;
			}

			break;
		}
 
		//WE most likely will receive a very large page... so do that here...	
		int crlf = -1, first = 0;
		if ( !( msg = malloc( 16 ) ) || !memset( msg, 0, 16 ) ) {
			return ERR( r->err, "%s\n", "Allocation failure." );
		} 

		for ( int blen = 0, chunked = 0;; ) {
			uint8_t xbuf[ 4096 ] = {0} ;

			if ( ( blen = recv( sockfd, xbuf, sizeof(xbuf), 0 ) == -1 ) ) {
				SSLPRINTF( "Error receiving mesaage to: %s\n", s );
				break;
			}
			else if ( !blen ) {
				SSLPRINTF( "%s\n", "No new bytes sent.  Jump out of loop." );
				break;
			}

			if ( !first++ ) {
				r->status = get_status( (char *)xbuf, blen );
				r->clen = get_content_length( (char *)xbuf, blen );
				if ( !get_content_type( r->ctype, (char *)xbuf, blen ) ) {
					return 0;
				}
				chunked = memstrat( xbuf, "Transfer-Encoding: chunked", blen ) > -1;
				if ( chunked ) {
					SSLPRINTF( "%s\n", "Chunked not implemented for HTTP" );
					return ERR( r->err, "%s\n", "Chunked not implemented for HTTP" );
				}
				if (( crlf = memstrat( xbuf, "\r\n\r\n", blen ) ) == -1 ) {
					0;//this means that something went wrong... 
				}
				//fprintf(stderr, "found content length and eoh: %d, %d\n", r->clen, crlf );
				crlf += 4;
				//write( 2, xbuf, blen );
			}

			//read into a bigger buffer	
			if ( !( msg = realloc( msg, r->len + blen ) ) ) {
				return ERR( r->err, "%s\n", "Realloc of destination buffer failed." );
			}

			memcpy( &msg[ r->len ], xbuf, blen ); 
			r->len += blen;

			if ( !r->clen )
				return ERR( r->err, "%s\n", "No length specified, parser error!." );
			else if ( r->clen && ( r->len - crlf ) == r->clen ) {
				SSLPRINTF( "Full HTTP message received\n" );
				break;
			}
		}

		//Close the fd
		if ( close( sockfd ) == -1 ) {

		}
	}
	else {
		//Define
		gnutls_session_t session;
		gnutls_datum_t out;
		gnutls_certificate_credentials_t xcred;

		//Initialize
		memset( &session, 0, sizeof(gnutls_session_t));
		memset( &xcred, 0, sizeof(gnutls_certificate_credentials_t));

		//Do socket connect (but after initial connect, I need the file desc)
		sockfd = 0;

		if ( RUN( !gnutls_check_version("3.4.6") ) )
			return ERR( r->err, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		if ( RUN( ( err = gnutls_global_init() ) < 0 ) )
			return ERR( r->err, "%s\n", gnutls_strerror( err ));

		if ( RUN( ( err = gnutls_certificate_allocate_credentials( &xcred ) ) < 0 ))
			return ERR( r->err, "%s\n", gnutls_strerror( err ));

		if ( RUN( (err = gnutls_certificate_set_x509_system_trust( xcred )) < 0 ))
			return ERR( r->err, "%s\n", gnutls_strerror( err ));

		//Set client certs this way...
		//gnutls_certificate_set_x509_key_file( xcred, "cert.pem", "key.pem" );

		//Initialize gnutls and set things up
		if ( RUN( ( err = gnutls_init( &session, GNUTLS_CLIENT ) ) < 0 ))
			return ERR( r->err, "%s\n", gnutls_strerror( err ));

		if ( RUN( ( err = gnutls_server_name_set( session, GNUTLS_NAME_DNS, root, strlen(root)) ) < 0))
			return ERR( r->err, "%s\n", gnutls_strerror( err ));

		if ( RUN( ( err = gnutls_set_default_priority( session ) ) < 0) )
			return ERR( r->err, "%s\n", gnutls_strerror( err ));
		
		if ( RUN( ( err = gnutls_credentials_set( session, GNUTLS_CRD_CERTIFICATE, xcred )) < 0) )
			return ERR( r->err, "%s\n", gnutls_strerror( err ));

		//Set random handshake details
		gnutls_session_set_verify_cert( session, root, 0 );

#if 0
		//socket connect is the shorter way to do this...
#else
		struct addrinfo hints, *servinfo, *pp;
		int rv;
		char b[ 10 ] = { 0 }, s[ INET6_ADDRSTRLEN ];
		snprintf( b, 10, "%d", port );	

		//Initialize the hints structure
		memset( &hints, 0, sizeof( hints ) );
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

	#ifdef INCLUDE_TIMEOUT 
		//Set up a timer to kill requests that are taking too long.
		//TODO: There is an alternate way to do with with socket(), I think
		struct itimerval timer;
		memset( &timer, 0, sizeof(timer) );
	#if 0
		signal( SIGVTALRM, timer_handler );
	#else
		struct sigaction sa;
		memset( &sa, 0, sizeof(sa) );
		sa.sa_handler = &timer_handler;
		sigaction( SIGVTALRM, &sa, NULL );
	#endif
		//Set timeout to 3 seconds (remember: no cleanup takes place...)
		timer.it_interval.tv_sec = 0;	
		timer.it_interval.tv_usec = 0;
		//NOTE: For timers to work, both it_interval and it_value must be filled out...
		timer.it_value.tv_sec = 2;	
		timer.it_value.tv_usec = 0;
		//if we had an interval, we would set that here via it_interval
		if ( setitimer( ITIMER_VIRTUAL, &timer, NULL ) == -1 ) {
			fprintf( stderr, "Set Timer Error: %s\n", strerror( errno ) );
			return 0;
		}
	#endif

		//Get the address info of the domain here.
		if ( (rv = getaddrinfo( root, b, &hints, &servinfo )) != 0 ) {
			fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( rv ) );
			return 0;
		}

		//Loop and find the right address
		for ( pp = servinfo; pp != NULL; pp = pp->ai_next ) {
			if ( ( sockfd = socket( pp->ai_family, pp->ai_socktype, pp->ai_protocol ) ) == -1) {	
				fprintf( stderr, "client: socket error: %s\n",strerror( errno ) );
				continue;
			}

			//fprintf(stderr, "%d\n", sockfd);
			if ( connect( sockfd, pp->ai_addr, pp->ai_addrlen) == -1 ) {
				close( sockfd );
				fprintf( stderr, "client: connect: %s\n", strerror( errno ) );
				continue;
			}

			break;
		}

		//If we completely failed to connect, do something.
		if ( pp == NULL ) {
			fprintf(stderr, "client: failed to connect\n");
			return 1;
		}
		freeaddrinfo( servinfo );
#endif

		//Set up GnuTLS to read things
		gnutls_transport_set_int( session, sockfd );
		gnutls_handshake_set_timeout( session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT );

		//Do the first handshake
		do {
			 RUN( ret = gnutls_handshake( session ) );
		} while ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 );

		//Check the status of said handshake
		if ( ret < 0 ) {
			//fprintf( stderr, "ret: %d\n", ret );
			if ( RUN( ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR ) ) {	
				type = gnutls_certificate_type_get( session );
				status = gnutls_session_get_verify_cert_status( session );
				err = gnutls_certificate_verification_status_print( status, type, &out, 0 );
				fprintf( stdout, "cert verify output: %s\n", out.data );
				gnutls_free( out.data );
				//jump to end, but I don't do go to
			}
			return ERR( r->err, "Handshake failed: %s\n", gnutls_strerror( ret ));
		}
		else {
			desc = gnutls_session_get_desc( session );		
			//consider dumping the session info here...
			//fprintf( stdout, "- Session info: %s\n", desc );
			gnutls_free( desc );
		}

		//This is the initial request... wtf were you thinking?
		if ( RUN(( err = gnutls_record_send( session, GetMsg, len ) ) < 0 ) )
			return ERR( r->err, "%s\n", "GnuTLS 3.4.6 or later is required for this example." );	

		//This is a sloppy quick way to handle EAGAIN
		int crlf = -1, first = 0;
		if ( !( msg = malloc( 16 ) ) || !memset( msg, 0, 16 ) ) {
			return ERR( r->err, "%s\n", "Allocation failure." );
		} 
	
		//there should probably be another condition used...
		for ( ;; ) {	
			char xbuf[ 4096 ];
			memset( xbuf, 0, sizeof(xbuf) ); 
			int ret = gnutls_record_recv( session, xbuf, sizeof(xbuf)); 
			SSLPRINTF( "gnutls_record_recv returned %d\n", ret );

			//receive
			if ( !ret ) {
				SSLPRINTF( "Peer has closed the TLS Connection\n" );
				break;
			}
			else if ( ret < 0 && gnutls_error_is_fatal( ret ) == 0 ) {
				SSLPRINTF( "Warning: %s\n", gnutls_strerror( ret ) );
				continue;
			}
			else if ( ret < 0 ) {
				SSLPRINTF( "Error: %s\n", gnutls_strerror( ret ) );
				break;	
			}
			else if ( ret > 0 ) {
				SSLPRINTF( "Recvd %d bytes:\n", ret );
write( 2, xbuf, ret );
				if ( !first++ ) {
					//Get status, content-length or xfer-encoding if available 
					r->status = get_status( (char *)xbuf, ret );
					r->clen = get_content_length( xbuf, ret );
					if ( !get_content_type( r->ctype, (char *)xbuf, ret ) ) {
						return 0;
					}
					r->chunked = memstrat( xbuf, "Transfer-Encoding: chunked", ret ) > -1;
					if ((crlf = memstrat( xbuf, "\r\n\r\n", ret )) == -1 ) {
						SSLPRINTF( "%s\n", "No CRLF sequence found, response malformed." );
						return ERR( r->err, "%s\n", "No CRLF sequence found, response malformed." );
					}
		
					//Increment the crlf by the length of "\r\n\r\n"	
					crlf += 4;
					if ( r->chunked ) {
						char *m = &xbuf[ crlf ];
						//parse the chunked length
						int lenp = memstrat( m, "\r\n", ret - crlf );	
						//to save chunk length, I need to convert to hex to decimal
						char lenpxbuf[ 10 ] = {0};
						memcpy( lenpxbuf, m, lenp );
						int sz = 0; //atoi( lenpxbuf );
					}
						
					#if 0	
					SSLPRINTF( "Got %s.",r->chunked ?"chunked message.":"message w/ content-length.");
					SSLPRINTF( "Got status: %d\n", r->status );
					SSLPRINTF( "Got clen: %d\n", r->clen );
					SSLPRINTF( "%s\n", "Initial message:" );
				  //write(2, xbuf, ret );
					#endif
				}

				//Finalize chunked messages.
				if ( r->chunked && ret == 5 ) {
					if ( memcmp( xbuf, "0\r\n\r\n", 5 ) == 0 ) {
						//fprintf(stderr, "the last one came in" );
						//fprintf( stderr, "%s, %d: my code ran.... but why stop?", __FILE__, __LINE__ ); 
						break;
					}
					else {
						fprintf(stderr, "not sure what happened.." );
					} 
				}
			}

			//Read into a bigger buffer	
			if ( !( msg = realloc( msg, r->len + ( ret + 1 ) ) ) ) {
				SSLPRINTF( "%s\n", "Realloc of destination buffer failed." );
				return ERR( r->err, "%s\n", "Realloc of destination buffer failed." );
			}
			memcpy( &msg[ r->len ], xbuf, ret );
			r->len += ret;

			//If it's chunked, try sending a 100-continue
			if ( r->chunked ) {
				const char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
				//int err = gnutls_record_send( session, cont, strlen(cont)); 
				fprintf(stderr, "%s (%d)\n", gnutls_strerror( err ), err );
			}
			else {
				if ( !r->clen ) {
					SSLPRINTF( "%s\n", "No length specified, parser error!." );
					return ERR( r->err, "%s\n", "No length specified, parser error!." );
				}
				else if ( r->clen && ( r->len - crlf ) == r->clen ) {
					SSLPRINTF( "Full message received: clen: %d, mlen: %d", r->clen, r->len );
					break;
				}
				SSLPRINTF( "recvd: %d , clen: %d , mlen: %d\n", r->len - crlf, r->clen, r->len );
			}
		} /* end while */

		int tries=0;
		//why does this hang?
		while (tries++ < 3 && (err = gnutls_bye(session, GNUTLS_SHUT_WR)) != GNUTLS_E_SUCCESS ) ;
		//if ((err = gnutls_bye(session, GNUTLS_SHUT_RDWR)) < 0 ) {

		if ( err != GNUTLS_E_SUCCESS ) {
			return ERR( r->err, "%s\n",  gnutls_strerror( ret ) );
		}

		//Close the file and free all of GnuTLS's structures
		if ( close( sockfd ) == -1 ) {

		}
		gnutls_deinit( session );
		gnutls_certificate_free_credentials( xcred );
		gnutls_global_deinit();
	}

	//Now, both requests ought to be done.  Set things here.
	r->data = msg;
	//print_www( r );
	extract_body( r );
	//print_www( r );
	return 1;
#endif
	return 1;
}



//TODO: This should move to lua.c or something
int extr_args ( zhttp_t *r, zTable *t, const char *text ) {
	int pos = lt_geti( t, text );
	if ( pos > -1 ) {
		lt_reset( t ), lt_set( t, pos + 1 );
		for ( zKeyval *kv ; ( kv = lt_next( t ) ) ; ) {
			//Everything is a text key (and it should be)
			if ( kv->key.type == ZTABLE_TRM ) {
				return 1;	
			}
			if ( !strcmp( text, "headers" ) )
				http_copy_header( r, kv->key.v.vchar, kv->value.v.vchar );
			else if ( !strcmp( text, "query" ) )
				http_copy_uripart( r, kv->key.v.vchar, kv->value.v.vchar );
			else if ( !strcmp( text, "body" ) ) {
				//You need to save something
				int len = strlen( kv->value.v.vchar ); 
				http_copy_formvalue( r, kv->key.v.vchar, kv->value.v.vchar, len );
			}
		}
	}
	return 0;
}




//A long form Lua function for making requests...
int http_request ( lua_State *L ) {
	//#1 needs to be either string or table
	const char *addr = NULL; //lua_tostring( L, 1 );
	char err[ 1024 ] = {0};
	int argcount = lua_gettop( L );
	zhttp_t qhttp = {0}, rhttp = {0};
	zTable *rt = lt_make( 256 );

	//Make sure that address was specified somewhere
	if ( !argcount )
		return luaL_error( L, "Not enough arguments to http.send()" );
	else if ( argcount > 2 ) {
		return luaL_error( L, "Too many arguments to http.send()" );
	}

	//....
	if ( argcount == 1 ) {
		if ( !lua_isstring( L, 1 ) && !lua_istable( L, 1 ) ) {
			return luaL_error( L, "First argument is neither a string or table." );
		}
		if ( lua_isstring( L, 1 ) )
			addr = lua_tostring( L, 1 );
		else { 
			if ( !lua_to_ztable( L, 1, rt ) || !lt_lock( rt ) )
				return luaL_error( L, "Could not convert Lua data to hash table." );
			else {
				if ( !( addr = lt_text( rt, "address" ) ) ) {
					return luaL_error( L, "Address not specified at table." );
				}
			}	
		}
	}
	else {
		if ( !lua_isstring( L, 1 ) ) {
			return luaL_error( L, "First argument must be a string when specifying multiple arguments" );
		}

		if ( !lua_istable( L, 1 ) ) {
			return luaL_error( L, "Second argument must be a table when specifying multiple arguments" );
		}

		addr = lua_tostring( L, 1 );
		if ( !( rt = lt_make( 128 ) ) ) {
			return luaL_error( L, "Could not allocate space for hash table." );
		}
		else if ( !lua_to_ztable( L, 2, rt ) ) {
			return luaL_error( L, "Could not convert Lua data to hash table." );
		}
	}	

	//Pop all arguments
	lua_pop( L, argcount );

	if ( !rt ) {
		qhttp.method = zhttp_dupstr( "GET" ); 
		qhttp.ctype = zhttp_dupstr( "text/html" );
		//Always add a user agent by default
		http_copy_header( &qhttp, "User-Agent", default_ua );
	}
	else {
		//Get address if you haven't already
		if ( !addr ) {
			addr = lt_text( rt, "address" );
		}

		//Get the content-type, type (of request)
		if ( lt_geti( rt, "method" ) == -1 )
			qhttp.method = zhttp_dupstr( "GET" ); 
		else {
			qhttp.method = zhttp_dupstr( lt_text( rt, "method" ) );
		}

		//Get the content-type, type (of request)
		if ( lt_geti( rt, "contenttype" ) == -1 )
			qhttp.ctype = zhttp_dupstr( "text/html" ); 
		else {
			qhttp.ctype = zhttp_dupstr( lt_text( rt, "contenttype" ) );
		}

		//Look for a user-agent if one was supplied
		if ( lt_geti( rt, "useragent" ) == -1 )
			http_copy_header( &qhttp, "User-Agent", default_ua );
		else {
			http_copy_header( &qhttp, "User-Agent", lt_text( rt, "useragent" ) );
		}
		

		//Add all the headers, query and body parts
		const char *str[] = { "headers", "query", "body", NULL  };
		for ( const char **type = str; *type; type++ ) {
			extr_args( &qhttp, rt, *type );
		}
	}

	//Path, port and address need to be finagled here
	//...
	int c, secure, port;
	if ( !memcmp( "https", addr, 5 ) )
		secure = 1, port = 443, addr = &addr[ 8 ];
	else if ( !memcmp( "http", addr, 4 ) )
		secure = 0, port = 80, addr = &addr[ 7 ];
	else {
		//This should automatically fill in https
		return luaL_error( L, "URL '%s' appears to be a fragment.", addr );
	}

	//Chop the URL very simply and crudely.
	if (( c = memchrat( addr, '/', strlen( addr ) )) == -1 )
		qhttp.path = "/", qhttp.host = zhttp_dupstr( addr );
	else {	
		char rootbuf[ 1024 ] = {0};
		memcpy( rootbuf, addr, c );
		qhttp.path = zhttp_dupstr( &addr[ c ] );
		qhttp.host = zhttp_dupstr( rootbuf );
	}

	//Finalize a request first?
	if ( !http_finalize_request( &qhttp, err, sizeof( err ) ) ) {
		return luaL_error( L, err ); 
	}

	//This is just going to send the request
	if ( !secure ) {
		//I don't really want to die here...
		if ( !make_http_request( addr, port, &qhttp, &rhttp, err, sizeof(err) ) ) {
			return luaL_error( L, err ); 
		}
	}
	else {
		return luaL_error( L, err ); 
	#if 0
		if ( !make_https_request( addr, port, &qhttp, &rhttp, err, sizeof(err) ) ) {
			return luaL_error( L, err ); 
		}
	#endif
	}

	//FINALLY, we need to put all the stuff in the response into a form that
	//a scripting language can use
	//write( 2, rhttp.msg, rhttp.mlen ); write( 2, "\n==\n", 4 );

#if 1
	//Prepare the table.
	lua_newtable( L );

	lua_pushstring( L, "status" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );

	lua_pushstring( L, "results" );
	lua_newtable( L );

	lua_pushstring( L, "body" );
	lua_pushlstring( L, (char *)rhttp.msg, rhttp.clen );
	lua_settable( L, 3 );
	
	lua_pushstring( L, "size" );
	lua_pushinteger( L, rhttp.clen );
	lua_settable( L, 3 );

	lua_pushstring( L, "contenttype" );
	lua_pushstring( L, rhttp.ctype );
	lua_settable( L, 3 );

#if 0
	lua_pushstring( L, "redirect" );
	lua_pushstring( L, w.ipv4 );
	lua_settable( L, 3 );

	lua_pushstring( L, "resolved_ip" );
	lua_pushstring( L, w.ipv4 );
	lua_settable( L, 3 );

	lua_pushstring( L, "resolved_ipv6" );
	lua_pushstring( L, w.ipv4 );
	lua_settable( L, 3 );

	lua_pushstring( L, "msglength" );
	lua_pushinteger( L, w.len );
	lua_settable( L, 3 );

	lua_pushstring( L, "msg" );
	lua_pushlstring( L, (char *)w.data, w.len );
	lua_settable( L, 3 );
#endif

	lua_settable( L, 1 );
	//http_free_body( &qhttp ), http_free_body( &rhttp );	
#endif
	return 1;
}


//Intended for gets
int http_get ( lua_State *L ) {
	luaL_checkstring( L, 1 );
	return 0;
}


int http_post ( lua_State *L ) {
	luaL_checkstring( L, 1 );
	return 0;
}


struct luaL_Reg http_set[] = {
 	{ "send", http_request }
,	{ NULL }
};

#ifndef HYPNO_H
//Allow for testing on the command line
int luaopen_http (lua_State *L) {
	luaL_newlib( L, http_set );
	return 1;
}
#endif
