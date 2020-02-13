int proc_echo ( int fd, struct HTTPBody *rq, struct HTTPBody *rs, void *ctx ) {

	//Allocate a big buffer and do work
	int progress = 0;
	char *buf = malloc( 6 );
	const char *names[] = { "Headers", "GET", "POST" };
	struct HTTPRecord **t[] = { rq->headers, rq->url, rq->body };
	char sbuf[ 4096 ];
	memset( sbuf, 0, sizeof( sbuf ) );

	//Reallocate a buffer
	int suplen = snprintf( sbuf, sizeof(sbuf)-1, "<h2>URL</h2>\n%s<br>\n", rq->path );
	if ( ( buf = realloc( buf, suplen ) ) == NULL ) {
		write( fd, http_500_fixed, strlen( http_500_fixed ) );
		close( fd );
		return 0;
	}

	//Add the URL (paths should never be more than 2048, but ensure this before write)
	memcpy( &buf[ progress ], sbuf, suplen );
	progress += suplen;

	//Loop through all three...
	for ( int i=0; i < sizeof(t)/sizeof(struct HTTPRecord **); i++ ) {
		
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
	}

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

	return 0;
}
