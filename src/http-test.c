#include "http.h"
#include "aeon_thumb_favicon.c"

const char default_protocol[] = "HTTP/1.1";

struct HTTPRecord *text_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Ok</h2>", 11 } 
};
struct HTTPRecord *uint8_body[] = { 
	&(struct HTTPRecord){ NULL, NULL, aeon_thumb_favicon_jpg, 3848 } 
};
struct HTTPRecord *headers[] = {
	&(struct HTTPRecord){ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
	&(struct HTTPRecord){ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
	&(struct HTTPRecord){ "Accept", NULL, (uint8_t *)"*/*", 3 },
	&(struct HTTPRecord){ NULL }
};
struct HTTPRecord xheaders[] = {
	{ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
	{ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
	{ "Accept", NULL, (uint8_t *)"*/*", 3 },
	{ NULL }
};

struct HTTPBody response_missing_body = {
	.clen = 12,
	.mlen = 16,
	.hlen = 16,
	.status = 200,
	.ctype = "text/html",
	.method = NULL,
	.protocol = "HTTP/1.1",
};

struct HTTPBody response_small_body = {
	.status = 200,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)text_body,
	.headers = NULL
};

struct HTTPBody response_binary_body = {
	.status = 200,
	.ctype = "image/jpeg",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)uint8_body 
};

//TODO: Add a response with no body value or len, and see that the check works...
#if 0
struct HTTPBody response_binary_body = {
	.status = 200,
	.ctype = "image/jpeg",
	.protocol = "HTTP/1.1",
	.body = (struct HTTPRecord **)uint8_body 
};
#endif

struct HTTPBody response_small_body_with_headers = {
	.status = 200,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.headers = (struct HTTPRecord **)&headers,
	.body = (struct HTTPRecord **)&text_body,
#if 0
	.body = ( struct HTTPRecord ** )( struct HTTPRecord *[] ){
		&( struct HTTPRecord ){ NULL, NULL, (uint8_t *)"<h2>Ok</h2>", 11 }
	},
	.headers = ( struct HTTPRecord ** )( struct HTTPRecord *[] ){
		&( struct HTTPRecord ){ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
		{ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
		{ "Accept", NULL, (uint8_t *)"*/*", 3 },
	},
#endif
};

struct HTTPBody response_with_error_not_found = {
	.status = 404,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = &( struct HTTPRecord * ){
		&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Resource not found</h2>", 28 }
	}
};


#if 0
struct HTTPBody response_with_error_internal_server_error = {
	.status = 500,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = &( struct HTTPRecord * ){
		&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Internal Server Error</h2>", 31 }
	}
};

struct HTTPBody response_with_invalid_error_code = {
	.status = 909,
	.ctype = "text/html",
	.protocol = "HTTP/1.1",
	.body = &( struct HTTPRecord * ){
		&(struct HTTPRecord){ NULL, NULL, (uint8_t *)"<h2>Never</h2>", 14 }
	}
};
#endif


#if 0
//Build a response body like it would be in a real environment
struct HTTPBody *build_test_object ( int status, char *ctype, uint8_t *body, int bodylen, struct HTTPRecord **headers ) {
	
	//Allocate
	struct HTTPBody *object = NULL;
	if ( !( object = malloc( sizeof( struct HTTPRecord ) ) ) ) {
		fprintf( stderr, "Failed to allocate space for test object.\n" );
		exit( 0 );
		return NULL;
	}

	if ( body ) {
		struct HTTPRecord *b = NULL;
		if ( !( *object->body = b = malloc( bodylen ) ) || !memset( b, 0, bodylen ) ) {
			fprintf( stderr, "Failed to allocate space for test body object.\n" );
			exit( 0 );
			return NULL;
		}

		if ( !memcpy( b, body, bodylen ) ) {
			fprintf( stderr, "Failed to allocate space for test body object.\n" );
			exit( 0 );
			return NULL;
		}
	}

	if ( headers ) {
		int headerlen = 0;
		while ( *headers ) {
			//Allocate each new header item...
			//ADDITEM( 
			struct HTTPRecord *r = malloc( sizeof( struct HTTPRecord ) );
			//memcpy( r, *headers, sizeof( struct HTTPRecord ) );
			fprintf( stderr, "field   : %s\n", (*headers)->field ); 
			fprintf( stderr, "metadata: %s\n", (*headers)->metadata ); 
			fprintf( stderr, "value   : " );
			write( 2, (*headers)->value, (*headers)->size ); 
			fprintf( stderr, "\n" );
			headers++;
		}
	}

	object->status = status;
	object->ctype = ctype;
	object->protocol = (char *)default_protocol;
	return NULL;
}
#endif

struct HTTPBody *responses[] = {
	&response_missing_body,
	&response_small_body,
	&response_small_body_with_headers,
	&response_with_error_not_found,
	&response_binary_body,
#if 0
	&response_with_error_internal_server_error,
	&response_with_invalid_error_code,
#endif
	NULL
};

struct HTTPBody *requests[] = {
	NULL
};


struct HTTPRecord ** build_header_structure( struct HTTPRecord **h ) {
	struct HTTPRecord **hh = headers;
	struct HTTPRecord **new = NULL;
	int newlen = 0;
	while ( (*hh)->field ) {
		struct HTTPRecord *k = malloc( sizeof( struct HTTPRecord ) );
		k->field = (*hh)->field;
		k->metadata = (*hh)->metadata;
		k->value = (*hh)->value;
		k->size = (*hh)->size;
		ADDITEM( k, struct HTTPRecord, new, newlen, NULL );
		hh++;
	} 
	ADDITEM( NULL, struct HTTPRecord, new, newlen, NULL );
	return new;
}

int main ( int argc, char *argv[] ) {
	struct HTTPRecord **hk = build_header_structure( headers );
#if 0
	while ( *hk ) {
		fprintf(stderr,"%s\n",(*hk)->field );
		hk++;
	}
return 0;
#endif
#if 0
	struct HTTPBody *resp[] = {
		build_test_object( 200, "text/html", NULL, 0, NULL ),
		build_test_object( 200, "text/html", (uint8_t *)"<h2>Ok!</h2>", 11, NULL ),
		build_test_object( 200, "text/html", aeon_thumb_favicon_jpg, aeon_thumb_favicon_jpg_len, NULL ),
		build_test_object( 200, "text/html", (uint8_t *)"<h2>Ok!</h2>", 11, (struct HTTPRecord *[]){ 
			&( struct HTTPRecord ){ "X-Case-Contact", NULL, (uint8_t *)"Lydia", 5 },
			&( struct HTTPRecord ){ "ETag", NULL, (uint8_t *)"dd323d9asdf", 11 },
			NULL
	#if 0
			{ "Accept", NULL, (uint8_t *)"*/*", 3 },
	#endif
		}),
		build_test_object( 404, "text/html", (uint8_t *)"<h2>Internal Server Error</h2>", 31, NULL ),
		build_test_object( 500, "text/html", (uint8_t *)"<h2>Never</h2>", 14, NULL ),
		build_test_object( 909, "text/html", (uint8_t *)"<h2>Ok!</h2>", 11, NULL ),
		NULL
	};
#endif

	//All this does is output text strings
#if 0
	fprintf( stderr, "Request parsing:\n" );
	struct HTTPBody **req = requests;
	while ( *req ) {
		char err[ 2048 ] = { 0 };
		struct HTTPBody *body = http_finalize_response( *req, err, sizeof(err) );
		if ( !body ) {
			fprintf( stderr, "FAILED - %s\n", err );
			req++;
			continue;
		}

		fprintf( stderr, "SUCCESS - " );
		write( 2, body->msg, body->mlen );
		req++;
	}
#endif

	fprintf( stderr, "Response packing:\n" );
	struct HTTPBody **res = responses;
	while ( *res ) {
		//fprintf( stderr, "%p\n", *res );
		struct HTTPBody *body = NULL;
		char err[ 2048 ] = { 0 };
		int m = 0;

		if ( !( body = http_finalize_response( *res, err, sizeof(err) ) ) ) {
			fprintf( stderr, "FAILED - %s\n", err );
			res++;
			continue;
		}

		fprintf( stderr, "SUCCESS - " );
		write( 2, body->msg, body->mlen );
		if ( m ) {
			//free( (*res)->headers );
		}
		res++;
	}
	return 0;
}
