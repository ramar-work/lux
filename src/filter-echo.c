#include "filter-echo.h"
int filter_echo ( struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {

	//Allocate a big buffer and do work
	char err[ 2048 ] = { 0 };
	char sbuf[ 4096 ] = { 0 };
	char *buf = NULL;
	const char *names[] = { "Headers", "GET", "POST" };
	struct HTTPRecord **t[] = { rq->headers, rq->url, rq->body, NULL };
	struct HTTPRecord ***tt = t;
	const char urlfmt[] = "<h2>URL</h2>\n%s<br>\n";
	int buflen = 0; 
	int progress = 0;

	//Sanity checks
	if ( !rq->path || !( buflen = (strlen(urlfmt) - 1) + strlen(rq->path) ) )
		return http_set_error( rs, 500, "Path cannot be zero..." );

	//Reallocate a buffer
	if ( !( buf = realloc( buf, buflen ) ) || !memset( buf, 0, buflen ) )
		return http_set_error( rs, 500, strerror( errno ) );

	if ( ( buflen = snprintf( buf, buflen, urlfmt, rq->path )) == -1 )
		return http_set_error( rs, 500, "Failed to zero memory..." );

#if 1
	//Switch to whiles, b/c it's just easier to follow...
	while ( **tt ) {
		//Do something with end str
		tt++;
	}
#else
	//Loop through all three...
	for ( int i=0; i < sizeof(t)/sizeof(struct HTTPRecord **); i++ ) {
#if 0
		//Define stuff
		struct HTTPRecord **w = t[i];
		char *endstr = ( *w ) ? "\n" : "\n-<br>\n";
		memset( sbuf, 0, sizeof(sbuf) );
		suplen = snprintf( sbuf, sizeof(sbuf)-1, "<h2>%s</h2>%s", names[i], endstr );

		//Reallocate a buffer
		if ( ( buf = realloc( buf, suplen + progress ) ) == NULL ) {
			write( fd, http_500_fixed, strlen( http_500_fixed ) );
			close( fd );
			return 0;
		}

		//Write the title out
		memset( &buf[ progress ], 0, suplen );
		memcpy( &buf[ progress ], sbuf, suplen );
		progress += suplen;

		//Now go through the rest
		while ( *w ) {
			struct HTTPRecord *r = *w;	
			int fieldLen = strlen( r->field );
			//Allocate enough for fields and length of strings: ' => ', '<br>', '\n' & '\0'
			int newSize = fieldLen + r->size + 10; 
			//Return early on lack of memory...
			if ( ( buf = realloc( buf, newSize + progress ) ) == NULL ) {
				write( fd, http_500_fixed, strlen(http_500_fixed) );
				close( fd );
				return 0;
			}

			//Initialize the new memory
			memset( &buf[ progress ], 0, newSize );

			//Go through and copy everything else.
			memcpy( &buf[ progress ], r->field, fieldLen );
			progress += fieldLen;
			memcpy( &buf[ progress ], " => ", 4 );
			progress += 4;
			memcpy( &buf[ progress ], r->value, r->size ); 
			progress += r->size;
			memcpy( &buf[ progress ], "<br>\n", 5 ); 
			progress += 5;
			w++;
		}
#endif
	}
#endif

#if 0
	//Get the length of the format string and allocate enough for buffer and thing
	int sendLen = strlen( http_200_custom ) + 6 + progress; //get the length of number 
	int actualLen = 0, written = 0;
	char *sendBuf = malloc( sendLen );	
	memset( sendBuf, 0, sendLen );
	written = snprintf( sendBuf, sendLen,	http_200_custom, progress );
	memcpy( &sendBuf[ written ], buf, progress );
	actualLen = written + progress;

	//Send the message to server, and see if it's read or not...
	if ( write( fd, sendBuf, actualLen ) == -1 ) {
		fprintf(stderr, "Couldn't write all of message..." );
		close(fd);
		return 0;
	}
#endif

	return 0;
}
