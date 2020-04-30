#include "http.h"
#include "socket.h"
#include "config.h"

#ifndef SERVER_H
#define SERVER_H

#define RD_EAGAIN 88
#define WR_EAGAIN 89
#define AC_EAGAIN 21
#define AC_EMFILE 31
#define AC_EEINTR 41

#if 1
	//#define CAFILE   "/etc/ssl/certs/ca-certificates.crt"
	#define FPATH "tlshelp/"
	#define KEYFILE  FPATH "x509-server-key.pem"
	#define CERTFILE FPATH "x509-server.pem"
	#define CAFILE   FPATH "x509-ca.pem"
	#define CRLFILE  FPATH "crl.pem"
#else
	#define FPATH "tlshelp/collinshosting-final/"
	#define KEYFILE  FPATH "x509-collinshosting-key.pem"
	#define CERTFILE FPATH "x509-collinshosting-server.pem"
	#define CAFILE   FPATH "collinshosting_com.ca-bundle"
	#define CRLFILE  FPATH "crl.pem"
#endif

struct filter {
	const char *name;
	int (*filter)( struct HTTPBody *, struct HTTPBody *, void * );
};

struct senderrecvr { 
	int (*proc)( int, struct HTTPBody *, struct HTTPBody *, struct config *, void * ); 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void (*init)( void ** );
#if 0
	void (*free)( int, struct HTTPBody *, struct HTTPBody *, void * );
#else
	void (*free)( int, struct HTTPBody *, struct HTTPBody *, void * );
#endif
	int (*pre)( int, void *, void ** );
#if 0
	int (*post)( int, void *, void ** ); 
#else
	int (*post)( int, void *, void ** ); 
#endif
	void *data;
}; 

struct model {
	int (*exec)( int, void * );
	int (*stop)( int * );
	void *data;
};

struct values {
	int port;
	int ssl;
	int start;
	int kill;
	int fork;
	char *user;
	char *config;
};

#endif 
