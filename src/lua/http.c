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

#define ERR(v,...) \
	snprintf( v, sizeof(v), __VA_ARGS__ ) && 0

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
#else
 #define SSLPRINTF( ... ) fprintf( stderr, __VA_ARGS__ ) ; fflush(stderr);
 #define EXIT(x) \
	fprintf(stderr,"Forced Exit.\n"); exit(x);
 #define MEXIT(x,m) \
	fprintf(stderr,m); exit(x);
#endif


#ifndef DEBUG_H
 #define RUN(c) (c)
#else
 #define RUN(c) \
 (c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#endif

#ifndef VERBOSE
 #define VPRINTF( ... )
#else
 #define VPRINTF( ... ) fprintf( stderr, __VA_ARGS__ ) ; fflush(stderr);
#endif


//User-Agent
static const char ua[] = 
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


static char * get_content_type (char *dest,char *msg, int mlen) {
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



//Load webpages via HTTP/S
int load_www ( const char *p, wwwResponse *r ) {
	const char *fp = NULL, GetMsgFmt[] = 
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: %s\r\n\r\n"
	;
	int err, ret, sd, ii, type, len, sockfd, c = 0, chunked = 0, port = 0, secure = 0;
	unsigned int status;
	uint8_t *msg = NULL;
	char *desc = NULL, buf[ 4096 ] = { 0 }, GetMsg[2048] = { 0 }, rootBuf[ 128 ] = { 0 };
	const char *root = NULL, *site = NULL, *path = NULL, *urlpath = NULL;

	//Negotiate http/https
	if ( !select_www( p, &port, &secure ) ) {
		return ERR( r->err, "URL '%s' appears to be a fragment.", p );
	}

	//What does this do?
	p = ( secure ) ? &p[8] : &p[7], fp = p;

	//Chop the URL very simply and crudely.
	if (( c = memchrat( p, '/', strlen( p ) )) == -1 )
		path = "/", root = p;
	else {	
		memcpy( rootBuf, p, c );
		path = &p[ c ];
		root = rootBuf;
	}

	//Pack a message
	if ( port != 443 )
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, root, ua );
	else {
		char hbbuf[ 128 ] = { 0 };
		//snprintf( hbbuf, sizeof( hbbuf ) - 1, "www.%s:%d", root, t->port );
		snprintf( hbbuf, sizeof( hbbuf ) - 1, "%s:%d", root, port );
		len = snprintf( GetMsg, sizeof(GetMsg) - 1, GetMsgFmt, path, hbbuf, ua );
	}

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

		int sb = 0, rb = 0, mlen = strlen(GetMsg);
		//And do the send and read?
		//If we send a lot of data, like a giant post, then this should be reflected here...
		for ( ;; ) {	
			int b;
			if ( ( b = send( sockfd, GetMsg, mlen, 0 )) == -1 ) {
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

		for ( ;; ) {
			uint8_t xbuf[ 4096 ];
			memset( xbuf, 0, sizeof(xbuf) );
			int blen = recv( sockfd, xbuf, sizeof(xbuf), 0 );

			if ( blen == -1 ) {
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
				r->chunked = memstrat( xbuf, "Transfer-Encoding: chunked", blen ) > -1;
				if ( r->chunked ) {
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
	print_www( r );
	extract_body( r );
	print_www( r );
	return 1;
}


//A long form Lua function for making requests...
int http_request ( lua_State *L ) {
	luaL_checkstring( L, 1 );
	const char *addr = lua_tostring( L, 1 );
	wwwResponse w = {0};
	if ( !load_www( addr, &w ) ) {
		return luaL_error( L, w.err ); 
	}

	//Prepare the table.
	lua_pop( L, 1 );
	lua_newtable( L );

	lua_pushstring( L, "status" );
	lua_pushboolean( L, 1 );
	lua_settable( L, 1 );

	lua_pushstring( L, "results" );
	lua_newtable( L );

	lua_pushstring( L, "body" );
	lua_pushlstring( L, (char *)w.body, w.clen );
	lua_settable( L, 3 );
	
	lua_pushstring( L, "size" );
	lua_pushinteger( L, w.clen );
	lua_settable( L, 3 );

	lua_pushstring( L, "content_type" );
	lua_pushstring( L, w.ctype );
	lua_settable( L, 3 );

	lua_pushstring( L, "resolved_ip" );
	lua_pushstring( L, w.ipv4 );
	lua_settable( L, 3 );

#if 0
	lua_pushstring( L, "msglength" );
	lua_pushinteger( L, w.len );
	lua_settable( L, 3 );

	lua_pushstring( L, "msg" );
	lua_pushlstring( L, (char *)w.data, w.len );
	lua_settable( L, 3 );
#endif

	lua_settable( L, 1 );
		
	free( w.data );
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

