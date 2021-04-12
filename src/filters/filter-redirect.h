/* -------------------------------------------------------- *
 * filter-redirect.c
 * ======
 * 
 * Summary 
 * -------
 * Enables the server to serve redirects at a server level.
 * 
 * -------------------------------------------------------- */

#include <zhttp.h>
#include "../server.h"

const int 
filter_redirect ( int fd, struct HTTPBody *req, struct HTTPBody *res, struct cdata *conn );
