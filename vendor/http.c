/* http.c */
#include "http.h"

//Data
static const char http_text_html[] = "text/html";
static const char content_string[] = "content";
static const char whitespace[]     = "\t\r\n\" ";
static const char http_200[] = ""
	"HTTP/1.1 200 OK\r\n"
	"Content-Length: 11\r\n"
	"Content-Type: text/html\r\n\r\n"
	"<h2>Ok</h2>";
static const char http_405[] = ""
	"HTTP/1.1 405 Method Not Allowed\r\n";
static const char http_100[] = ""
	"HTTP/1.1 100 Continue\r\n\r\n";
static const char http_post[] = "" 
	"HTTP/1.1 200 OK\r\n"
	"Content-Length: 129\r\n"
	"Content-Type: text/html\r\n\r\n"
	"<form method=POST enctype='multipart/form-data'>"
		"<input type=text name=edward></input>"
		"<input type=file name=julius></input>"
	"</form>";
static const char *http_status[] = {
	[HTTP_100] = "Continue",
	[HTTP_101] = "Switching Protocols", 
	[HTTP_200] = "OK",
	[HTTP_201] = "Created",
	[HTTP_202] = "Accepted",
	[HTTP_204] = "No Content",
	[HTTP_206] = "Partial Content",
	[HTTP_300] = "Multiple Choices",
	[HTTP_301] = "Moved Permanently",
	[HTTP_302] = "Found",
	[HTTP_303] = "See Other",
	[HTTP_304] = "Not Modified",
	[HTTP_305] = "Use Proxy",
	[HTTP_307] = "Temporary Redirect",
	[HTTP_400] = "Bad Request",
	[HTTP_401] = "Unauthorized",	
	[HTTP_403] = "Forbidden",			
	[HTTP_404] = "Not Found",				
	[HTTP_405] = "Method Not Allowed",
	[HTTP_406] = "Not Acceptable",
	[HTTP_407] = "Proxy Authentication Required",
	[HTTP_408] = "Request Timeout",
	[HTTP_409] = "Conflict",
	[HTTP_410] = "Gone",
	[HTTP_411] = "Length Required",
	[HTTP_412] = "Precondition Failed",
	[HTTP_413] = "Request Entity Too Large",
	[HTTP_414] = "Request URI Too Long",
	[HTTP_415] = "Unsupported Media Type",
	[HTTP_416] = "Requested Range",
	[HTTP_417] = "Expectation Failed",
	[HTTP_418] = "I'm a teapot",
	[HTTP_500] = "Internal Server Error",
	[HTTP_501] = "Not Implemented",
	[HTTP_502] = "Bad Gateway",
	[HTTP_503] = "Service Unavailable",
	[HTTP_504] = "Gateway Timeout"
};



static void http_dump (uint8_t *blob, int adjust, const char *msg)
{
	fprintf( stderr, "%s\n", msg );
	write( 1, "'", 1 );
	write( 1, blob, adjust );
	write( 1, "'", 1 );
	write( 1, "\n", 1 );
}


//An http error handler
_Bool http_error_h (Recvr *r, HTTP *h, HTTP_Status st) 
{
	http_set_status(h, st);

#if 0
	const char *msg = "http response looks like:\n";
	write( 2, msg, strlen( msg ));
	write( 2, h->error, strlen( h->error ) );
#endif

	http_set_content(h, http_text_html, (uint8_t *)h->error, strlen(h->error));
	http_pack_response( h );
	r->stage = NW_AT_WRITE;
	return 1;
}



//Get the method, version and location
_Bool http_parse_first_line (HTTP *h, uint8_t *msg, int len) 
{
	//Define stuff
	HTTP_Request *r = &h->request;
	Parser p={ .words={{" "},{"\r"},{NULL}} }; 
	int tlen = 0;
	struct T { char *ptr; int maxlen; } tt[] = {
		{ r->method,   HTTP_METHOD_MAX },
		{ r->path,     HTTP_URL_MAX },
		{ r->protocol, HTTP_PROTO_MAX }
	};

	pr_prepare( &p );

	if ((tlen = memchrat( &msg[0], '\n', len )) == -1)
		return 0;  //This was a fail...  specify the error somewhere....

	for ( int i=0;  i<3 && pr_next( &p, &msg[0], tlen + 1 ); i++ ) 
	{
		memset( tt[i].ptr, 0, tt[i].maxlen );
		if ( p.size + 1 > tt[i].maxlen ) return 0; //Also was a fail, specify error...
		memcpy( tt[i].ptr, &msg[ p.prev ], p.size );
		tt[i].ptr[ p.size ] = '\0';	
	}

	return 1;
}



//Set the content length
_Bool http_get_content_length (HTTP *h, uint8_t *msg, int32_t len) 
{
	int a, b;
	char length_char[128]={0};
	HTTP_Request *r = &h->request;

	if ((a = memstrat(msg, "Content-Length", len)) > -1) 
	{
		/*Move up pointer*/
		a += strlen("Content-Length: ");
		b = memchrat(&msg[a], '\r', len - a); 
		if (b > 127 || b == -1)
			return 0;

		/*Copy to string*/
		memcpy(length_char, &msg[a], b);
		length_char[ b ] = '\0';

		/*Make sure it's numeric*/
		for (int i=0;i<strlen(length_char); i++)  {
			if ((int)length_char[i] < 48 || (int)length_char[i] > 57)
				return (fprintf(stderr, "content length was not number\n") ? 0 : 0);
		}

		/*Set the content-length*/
		r->clen = atoi(length_char);
		return 1; 
	}
	return 0;
}



//Get the distance from the headers on
_Bool http_get_header_length (HTTP *h, uint8_t *msg, int32_t len) 
{
	HTTP_Request *r = &h->request;
	return ((r->hlen = memstrat(msg, "\r\n\r\n", len)) == -1) ? 0 : 1;
}



//Figure out the message length
_Bool http_get_message_length (HTTP *h, uint8_t *msg, int32_t len) 
{
	//if these are equal, then we haven't gotten the full thing yet...
	HTTP_Request *r = &h->request;
	if (r->hlen + 4 == len)
		return 0;
	r->mlen = len - (r->hlen + 4);	
	return 1;	
}



//Set the boundary
_Bool http_get_boundary (HTTP *h, uint8_t *msg, int32_t len) 
{
	//Define
	HTTP_Request *r = &h->request;
	int bs, be;

	//Ugly boundary extraction
	if ( (bs = memstrat(msg, "boundary=", r->hlen)) > -1) {
		bs += 9;
		be = memchrat(&msg[bs], '\r', r->hlen - bs); 
		be = (be > -1) ? be : r->hlen - bs; 
		memcpy(r->boundary, &msg[bs], be);
		r->boundary[be] = '\0';
	}

	return 1; 
}



//Trim whitespace
unsigned char *httpvtrim (uint8_t *msg, int len, int *nlen) 
{
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr("\r\n\t ", *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr("\r\n\t ", *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}



//Trim any characters 
unsigned char *httptrim (uint8_t *msg, const char *trim, int len, int *nlen) 
{
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), 4) && nl-- ) ; 
	while ( memchr(trim, *m, 4) && nl-- ) m++;
	*nlen = nl;
	return m;
}



//Create the rest of the request (then send it on)
int http_get_remaining (HTTP *h, uint8_t *msg, int len) 
{
	//Define
	HTTP_Request *r = &h->request;
	LiteBlob *host = NULL;
	int runner = 0;
	struct HS { char *name,a,s; Parser p; /*LiteKv *kv;*/ uint8_t *msg; int ml, type, inc; } hs[] = 
	{
		//URL
		{ "URL",'u', 0  ,.p.words = {{"/"}, {"?"}, {"\0"}, {NULL}},
			/*r->URL,*/ (uint8_t *)r->path, strlen(r->path), 1, 0 },
		//Headers
		{ "headers",'h','\n',.p.words = {{"\r\n"}, {":"}, {"="}, {";"}, {NULL}},
			/*r->headers, */  msg, r->hlen    , 1, 1 },
		/*GET*/
		{ "GET",'g','?' ,.p.words = {{"?"}, {"&"}, {"="}, {NULL}},
			/*r->GET,*/ (uint8_t *)r->path, strlen(r->path), 1, 1 },
		/*MPP*/
		{ "POST",'m', 0  ,.p.words = {{r->boundary},{"\r\n\r\n"},{"\r\n"},{":"},{"="},{";"},{NULL}},
			/*r->body,*/ &msg[r->hlen + 4], r->clen        , 1, 0 },
		/*APP*/
		{ "POST",'a', 0  ,.p.words = {{"\r\n"}, {":"},{"="},{"&"},{NULL}},
			/*r->body,*/ &msg[r->hlen + 4], r->clen        , 1 }
	};

	//Initialize here
	lt_init( &r->table, NULL, 1027 );

	//...
	if ( memcmp( "HEAD", r->method, 4 ) == 0 )
		runner = 1;
	else if ( memcmp( "GET", r->method, 3 ) == 0 )
		runner = memchr( r->path, '?', strlen(r->path) ) ? 3 : 2;
	else if ( memcmp( "POST", r->method, 4 ) == 0 || memcmp( "PUT", r->method, 3 ) == 0 ) 
	{
		runner = memchr( r->path, '?', strlen(r->path) ) ? 3 : 2;
		hs[ runner++ ] = hs [ (*r->boundary) ? 3 : 4 ];
	}


	//...
	for (int i=0; i < runner; i++)
	{
		unsigned char *blob = NULL;
		struct HS *h = &hs[ i ];	
		Parser *ps   = &h->p;
		int bw       = 0;
		int ii       = 0;
		int jj       = 0;
		int kk       = 0;
		int root     = 0;
		int colon    = 0;
		int cstart   = 0,
			  cend     = 0;

		//Add a new key
		lt_addblobkey( &r->table, (unsigned char *)h->name, strlen(h->name));
		lt_descend( &r->table );

		//Find the starting character.
		if ( h->s ) 
		{
			if ((ii = memchrat(h->msg, h->s, h->ml)) == -1) continue;
			h->msg = memchr(h->msg, h->s, h->ml);
			h->ml -= ii;
		}

		h->msg += h->inc;
		pr_prepare( ps );

		while ( pr_next( ps, h->msg, h->ml ) ) 
		{
			int adjust=0;
			if ( ps->word == NULL )
			{
				//unsigned char *blob = NULL;//httpvtrim( &h->msg[ ps->prev - 1 ], ps->size, &adjust );
				if ( strcmp("URL", h->name) == 0 ) 
				{
					lt_addintkey( &r->table, ++bw );
					if ( h->msg[ ps->prev - 1 ] == '?' ) break;
					blob = httpvtrim( &h->msg[ ps->prev - 1 ], ps->size + 1, &adjust );
				}
				else {
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust );
				}
				lt_addblobvalue( &r->table, blob, adjust );
				lt_finalize( &r->table );
				break;
			}
			else if ( *ps->word == '-' ) 
			{
				bw = 1; //Find http boundary
				unsigned char *ret = NULL;
				if ((ii = memstrat( &h->msg[ ps->prev ], "; name=", h->ml - ps->prev )) == -1)
					continue;
				int begin = ii + strlen("; name=") + ps->prev;
				int jj = memstrat( &h->msg[ begin ], "\r", h->ml - begin );
				int kk = memstrat( &h->msg[ begin ], ";", h->ml - begin );
				
				if ( jj == -1 && kk == -1 )
					continue;
				if ( jj == -1 )
					ret = httptrim( &h->msg[ begin ], whitespace, kk, &adjust );
				else if (kk == -1)
					ret = httptrim( &h->msg[ begin ], whitespace, jj, &adjust );
				else {
					ret = httptrim( &h->msg[ begin ], whitespace, (jj < kk) ? jj : kk, &adjust );
				}
				lt_addblobkey( &r->table, ret, adjust );
				lt_descend( &r->table );
			}
			else if ( *ps->word == '/' ) //memcmp( "/", w, 1 ) == 0 ) 
			{
				//root is ALWAYS first...
				if ( !root ) 
				{
					lt_addintkey( &r->table, bw );
					lt_addblobvalue( &r->table, (uint8_t *)"/", 1 );
					lt_finalize( &r->table );
					root = 1;
					if (h->ml == 1)
						break;
					else {
						continue;
					}
				}

				lt_addintkey( &r->table, ++bw );
				lt_addblobvalue( &r->table, &h->msg[ ps->prev - 1 ], ps->size + 1 );
				lt_finalize( &r->table );
			}
			else if ( *ps->word == '?' ) //memcmp( "?", w, 1 ) == 0 ) {
			{
				lt_addintkey( &r->table, ++bw );
				lt_addblobvalue( &r->table, &h->msg[ ps->prev - 1 ], ps->size + 1 );
				lt_finalize( &r->table ); 
			}	
			else if ( memchr( ":=", *ps->word, 2 ) ) 
			{
				if ( h->a == 'h' ) {
					if ( colon ) continue;
					else { 
						colon = 1;
						cstart = ps->next;
					}
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust ); 
					lt_addblobkey( &r->table, blob, adjust );
					//write( 2, blob, adjust ); write( 2, " => ", 4 );
				}
				else {
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust ); 
					lt_addblobkey( &r->table, blob, adjust );
				}
			}
			else if ( memchr( ";\r&", *ps->word, 3 ) ) 
			{
				//unsigned char *blob = NULL;
				if (bw) 
					{ bw=0;  continue; }

				//Unset the colon
				if ( h->a == 'h' ) 
				{
					if ( *ps->word == '\r' && colon ) {	
						colon = 0;
						//write( 2, &h->msg[cstart], ps->next - cstart  );
						blob = httpvtrim( &h->msg[ cstart ], ps->next - cstart, &adjust ); 
					}
					else if ( *ps->word == ';' && colon ) {
						continue;
					} 
				} 
				else {
					blob = httpvtrim( &h->msg[ ps->prev ], ps->size, &adjust ); 
				}

				//Value fudging... cuz Multipart post is the stupidest protocol ever written
				lt_addblobvalue( &r->table, blob, adjust ); 
				lt_finalize( &r->table );

				//Handle multipart values...
				if ( memcmp( "\r\n\r\n", ps->word, 4 ) == 0 ) 
				{
					SHOWDATA( "%d, %s\n", ps->next, r->boundary );
					int aa = memstrat( &h->msg[ ps->next ], r->boundary, h->ml - ps->next ); 
					if ( aa == -1 )
						continue;	
					aa -= 2;
					lt_addblobkey (&r->table, (uint8_t *)content_string, strlen(content_string)); 
					lt_addblobvalue( &r->table, &h->msg[ ps->next ], aa - 2);
					lt_finalize(&r->table);
					lt_ascend(&r->table);
					ps->next += aa;
				}
			}
		}

		lt_ascend( &r->table );
	}

	lt_lock( &r->table );

	//Now extract the hostname and save it to a buffer
	if ( !(host = &lt_blob( &r->table, "headers.Host" ))->size )
		h->hostname = NULL;
 	else 
	{
		int p=0;
		int pos = (( p = memchrat( host->blob, ':', host->size )) == -1) ? host->size : p;
		h->hostname = malloc( pos + 1 );
		memset( h->hostname, 0, pos + 1 ); 
		memcpy( h->hostname, host->blob, pos );
		h->hostname[ pos ] = '\0';		
	}
	return 1;
}




//Automatically sets the right status line
_Bool http_set_status (HTTP *h, HTTP_Status st) {
	HTTP_Response *res = &h->response;
	res->status = st;
	res->sttext = http_status[st];
	return 1;
}



//Set content-type
_Bool http_set_content_type (HTTP *h, const char *ctype) {
	HTTP_Response *res = &h->response;
	res->ctype = ctype;
	return 1;
}



//Set content-length (you do this b/c checks are needed)
_Bool http_set_content_length (HTTP *h, int len) {
	HTTP_Response *res = &h->response;
	res->clen = len;
	return 1;
}



//...
_Bool http_set_content_body (HTTP *h, uint8_t *body, int len) {
	HTTP_Response *res = &h->response;

	//This brings up something interesting.  If we're out of space. Having a 
	//buffer around for other purposes could be INCREDIBLY useful...	
	return ( !bf_append( h->resb, body, (res->clen = len)) ) ? 0 : 1;
}



//...
_Bool http_set_content (HTTP *h, const char *type, uint8_t *body, int len) {
	HTTP_Response *res = &h->response;
	res->clen  = len;
	res->ctype = type;

	if ( !bf_append( h->resb, body, (res->clen = len)) ) {
		return 0;
	} 

	return 1;
}



//...
_Bool http_set_content_and_status (HTTP *h, HTTP_Status st, const char *type, uint8_t *body, int len) 
{
	HTTP_Response *res = &h->response;
	res->status = st;
	res->sttext = http_status[st];
	res->clen   = len;
	res->ctype  = type;
	//ADDSEND(res->msg, body, len);
	if ( !bf_append( h->resb, body, (res->clen = len)) ) {
		return 0;
	} 
	return 1;
}



//Set the http header, moves the LiteKv structure underneath
_Bool http_set_header (HTTP *h, const char *key, const char *value) {
	return 1;
}



//Set the version
_Bool http_set_version (HTTP *h, float version) {
	HTTP_Response *res = &h->response;
	res->version = (version == 0.9 || version == 1.0 || version == 1.1 || version == 2) ? version : 1.1;
	return 1;
}



//Copy to a buffer and tell me how much space I've got left
int http_copy_to_buffer (HTTP *h) {
return 0;
}



//Pack an HTTP request
int http_pack_request (HTTP *h) {
	return 0;
}



//Pack an HTTP response
int http_pack_response (HTTP *h) 
{
	//Define 
	HTTP_Response *res = &h->response;
	uint8_t statline[1024] = { 0 };
	char ff[4] = { 0 };
	const char *fmt   = "HTTP/%s %d %s\r\n";
	const char *cfmt  = "Content-Length: %d\r\n";
	const char *ctfmt = "Content-Type: %s\r\n\r\n";

	//Set defaults
	(!res->version) ? res->version = 1.1              : 0;
	(!res->ctype)   ? res->ctype   = "text/html"      : 0;
	(!res->status)  ? res->status  = 200              : 0;
	(!res->sttext)  ? res->sttext  = http_status[200] : 0;
	snprintf(ff, 4, (res->version < 2) ? "%1.1f" : "%1.0f", res->version);

	//Copy the status line
	res->mlen += snprintf( (char *)&statline[res->mlen], 1024, fmt, 
		ff, res->status, res->sttext);

	//All other headers get res->mlen here
	//....?

	//Always have at least a content length line
	res->mlen += snprintf( (char *)&statline[res->mlen], 1024, cfmt, res->clen);

	//Finally, set a content-type
	res->mlen += snprintf( (char *)&statline[res->mlen], 1024, ctfmt, res->ctype);

	//Stop now if this is a zero length message.
	if (!res->clen) {
		//memcpy( &res->msg[0], statline, res->mlen );
		if ( !bf_append( h->resb, statline, res->mlen ) ) {
			return 0;
		}
		return 1;
	}

	//Move the message (provided there's space)		
	//This will fail when using fixed buffers...
	if ( !bf_prepend( h->resb, statline, res->mlen ) ) {
		return 0;
	}
	//memmove( &res->msg[res->mlen], &res->msg[0], res->clen);
	//memcpy( &res->msg[0], statline, res->mlen ); 
	res->mlen += res->clen;
	return 1;
}



//Save HTTP Post stuff
int http_save_items ( ) {
	//Loops through some array and saves stuff...
	return 0;
}



//Print an HTTP request
void http_print_request (HTTP *h) 
{
	HTTP_Request *r = &h->request;
	stprintf("method",         r->method);
	stprintf("path",           r->path);
	stprintf("protocol",       r->protocol);
	nmprintf("content length", (int)r->clen);
	nmprintf("header length",  (int)r->hlen);
	nmprintf("message length", (int)r->mlen);
	stprintf("boundary",       r->boundary);
	fprintf( stderr , "\n" );
}



//Print an HTTP response
void http_print_response (HTTP *h) 
{
	HTTP_Response *r = &h->response;
	nmprintf("status",         r->status);
	stprintf("status_text",    r->sttext);
	nmprintf("bypass",         r->bypass);
	fdprintf("version",        r->version);
	stprintf("Content-Type",   r->ctype);

	nmprintf("Content-Length", r->clen);
	nmprintf("header length",  (int)r->hlen);
	nmprintf("message length", (int)r->mlen);

	bdprintf("message",        r->msg, r->mlen);
	fprintf( stderr , "\n" );
}



//Read messages (no streaming or anything else)
_Bool http_read (Recvr *r, void *p, char *e) 
{
	//I'm sorry.  This looks terrible...
	_Bool rd_all_data = 0;
	HTTP *h                 = (HTTP *)r->userdata;
	HTTP_Request *request   = &h->request;
	HTTP_Response *response = &h->response;
	request->msg            = r->request;
	response->msg           = r->response;
	h->reqb                 = &r->_request;
	h->resb                 = &r->_response;

	//GET
	if ( memstrat(r->request, "GET", r->recvd) > -1 ) 
	{
		//Parse the line
		if ( !http_parse_first_line(h, r->request, r->recvd) )
			return 0;
		if ( !http_get_header_length(h, r->request, r->recvd) )
			return 0;
		rd_all_data = 1;
	}

	//POST
	else if ( memstrat(r->request, "POST", r->recvd) > -1 ) 
	{
		/*If all data not received, send another message (100-continue)*/	
	 #ifdef HTTP_VERBOSE
		fprintf(stderr, "POST received %d bytes.\n", r->recvd);
	 #endif

		//Parse the line
		if ( !strlen( request->path ) && !http_parse_first_line(h, r->request, r->recvd) )
			return 0;

		//Get content-length (and reject if it's not there)
		if ( !request->clen && !http_get_content_length(h, r->request, r->recvd) )
			return 0;

		//Get the distance to the end of the headers
		if ( !request->hlen && !http_get_header_length(h, r->request, r->recvd) )
			return 0;

		//Get the message body length. 
		if ( !http_get_message_length(h, r->request, r->recvd) )
			return 0; /*We need to try reading again*/

		//Get the distance to the end of the headers
		if ( !http_get_boundary(h, r->request, r->recvd) )
			return 0;

		//Print response
		//print_request(h);
		//fprintf(stderr, "(request->mlen = %d) - (request->clen = %d)\n", request->mlen, request->clen);

		//Check if we've finally received everything
		if (h->request.mlen == h->request.clen) 
		{
			rd_all_data = 1;
			h->request.mlen += h->request.hlen;
		}
		else {
			//write 100 continue if not
			if (write(r->client->fd, http_100, strlen(http_100)) == -1)
			{
				switch ( errno )
				{
					case EAGAIN:
					case EBADF:
					case EFBIG:
					case EINTR:
					case EIO:
					case ENOSPC:
					case EPIPE:
					case ERANGE:
						http_err( r, h, 500, "%s\n", strerror( errno ) );
						break;
					default:
						http_err( r, h, 500, "unknown error occurred at read" );
				}
				return 1;	
			}
			return 0;
		}
	}
	//No support for other things yet
	else {
		//memcpy(r->response, http_405, strlen(http_405));
		http_err( r, h, 405, "The method requested by the client was invalid." ); 
		return 0;
	}

	if (NW_CALL( rd_all_data )) {
		r->stage = NW_AT_PROC;
	}
	return 1;
}



//Process a request otherwise
_Bool http_proc (Recvr *r, void *p, char *e) {
	r->stage = NW_AT_WRITE;
	//r->len = 0;
	return 1;
}



//Put this dummy writer here for now.
_Bool http_fin (Recvr *r, void *p, char *e) {
	HTTP *h = (HTTP *)r->userdata;
	r->stage = NW_COMPLETED;
	memset(h, 0, sizeof(HTTP));
	return 1;
}
