//First web app built in C
#include <zhttp.h>
#include <server.h>

int app ( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn ) {

	//Define a message
	char * message = strdup( "<h2>Hello, World!</h2>" );

	//Send something out
#if 0
	http_set_response( res, 200, "text/html", message, len, headers ); 
#else
	http_set_status( res, 200 ); 
	http_set_ctype( res, "text/html" );
	http_set_content( res, message, strlen(message) );
#endif

	//Return true because it worked
	return 1;

}
