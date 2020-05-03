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
	int (*filter)( struct HTTPBody *, struct HTTPBody *, struct config *, struct host * );
};

struct senderrecvr { 
	int (*read)( int, struct HTTPBody *, struct HTTPBody *, void * );
	int (*write)( int, struct HTTPBody *, struct HTTPBody *, void * ); 
	void (*init)( void ** );
	void (*free)( void ** );
	int (*pre)( int, struct config *, void ** );
	int (*post)( int, struct config *, void ** ); 
	struct filter *filters;
	char *config;
	void *data;
}; 

struct model {
	int (*exec)( int, void * );
	int (*stop)( int * );
	void *data;
};

int srv_response ( int, struct senderrecvr * );
int srv_setsocketoptions ( int fd );
int srv_writelog ( int fd, struct sockAbstr *su );
#endif 
