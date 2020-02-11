#include "socket.h"

//tell me the error returned.  I don't know what happened...
void whatsockerr( int e ) {
	if ( e == EBADF  ) fprintf( stderr, "Got sockerr: %s", "EBADF " );
	else if ( e == ECONNREFUSED  ) fprintf( stderr, "Got sockerr: %s", "ECONNREFUSED " );
	else if ( e == EFAULT  ) fprintf( stderr, "Got sockerr: %s", "EFAULT " );
	else if ( e == EINTR  ) fprintf( stderr, "Got sockerr: %s", "EINTR " );
	else if ( e == EINVAL  ) fprintf( stderr, "Got sockerr: %s", "EINVAL " );
	else if ( e == ENOMEM  ) fprintf( stderr, "Got sockerr: %s", "ENOMEM " );
	else if ( e == ENOTCONN  ) fprintf( stderr, "Got sockerr: %s", "ENOTCONN " );
	else if ( e == ENOTSOCK  ) fprintf( stderr, "Got sockerr: %s", "ENOTSOCK " );
	else if ( e == EAGAIN || e == EWOULDBLOCK  ) fprintf( stderr, "Got sockerr: %s", "EAGAIN || EWOULDBLOCK " );
}


