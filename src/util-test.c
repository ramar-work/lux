#include "util.h"

const char *words[] = {
	"arbutus",
	"TAMMYPIGEONBACON",
	"carousel",
	"3333242423824234!",
	"enterprise",
	"?something_last_way?",
	"El Segundo",
	NULL
};


void iterate_items ( void ** list ) {
	while ( list && *list ) list++;
}

void iterate_chars ( char ** list ) {
	while ( list && *list ) {
		fprintf( stderr, "%s, ", *list );
		list++;
	}
	fprintf( stderr, "\n" );
}

void iterate_ints ( int ** list ) {
	while ( list && *list ) {
		fprintf( stderr, "%d, ", **list );
		list++;
	}
	fprintf( stderr, "\n" );
}

void iterate_structs ( void ** list ) {
	while ( list && *list ) {
		fprintf( stderr, "%p, ", *list );
		list++;
	}
	fprintf( stderr, "\n" );
}

void iterate_charlist ( char *** list ) {
	while ( list && *list ) {
		fprintf( stderr, "%p=", *list );
		while ( *list && **list ) {
			fprintf( stderr, "%s, ", **list );
			(*list)++;
		}
		list++;
	}
	fprintf( stderr, "\n" );
}

int main ( int argc, char *argv[] ) {

	//Test add_item
	fprintf( stderr, "ADD ITEMS TO LIST:\n" );
	//Add ints
	int intlistlen = 0;
	int **intlist = NULL;
	int intset[] = { 0, 2, 3, 4, 555, 6723124, 12, 1, 8, 9 };
	for ( int i=0; i<sizeof(intset)/sizeof(int); i++ ) {
		if ( !add_item( &intlist, &(intset[i]), int *, &intlistlen ) ) {
			fprintf( stderr, "Failed to add int to list.\n" );
			exit( 0 );
		}
	}
	add_item( &intlist, NULL, int *, &intlistlen );
	fprintf( stderr, "intlist should contain %d elements: ", intlistlen );
	iterate_ints( intlist );
	free( intlist );

	//Add strings
	char **boxlist = NULL;
	int boxlistlen = 0;
	char *boxset[] = { "intel", "amd", "ryzen", "fuschia" };
	for ( int i=0; i<sizeof(boxset)/sizeof(char *); i++ ) {
		if ( !add_item( &boxlist, boxset[i], char *, &boxlistlen ) ) {
			fprintf( stderr, "Failed to add string to list.\n" );
			exit( 0 );
		}
	}
	fprintf( stderr, "boxlist should contain %d elements: ", boxlistlen );
	iterate_chars( (char **)boxlist );
	free( boxlist );

	//Add random datatypes
	struct randomstruct { int a,b,c,d; }; 
	int structlistlen = 0;
	struct randomstruct **structlist = NULL;
	struct randomstruct *structset[] = {
		&(struct randomstruct){ 1, 3, 4, 5 },
		&(struct randomstruct){ 1, 3, 4, 8 },
		&(struct randomstruct){ 1, 3, 4, 7 },
		&(struct randomstruct){ 1, 3, 4, 6 },
	};
	for ( int i=0; i<sizeof(structset)/sizeof(struct randomstruct *); i++ ) {
		if ( !add_item( &structlist, structset[i], struct randomstruct *, &structlistlen ) ) {
			fprintf( stderr, "Failed to add struct to list.\n" );
			exit( 0 );
		}
	}
	fprintf( stderr, "structlist should contain %d elements: ", structlistlen );
	iterate_structs( (void **)structlist );
	free( structlist );

	//This is a tricky one, but it would probably work...
	char ***charlistlist = NULL;
	int charlistlen = 0;
	char **charlistset[] = { 
		( char *[] ){ "that", "there", "cletus", NULL },
		( char *[] ){ "this", "here", "jetus", NULL },
		( char *[] ){ "there", "where", "HERE!", NULL },
	};
	for ( int i=0; i<sizeof(charlistset)/sizeof(char **); i++ ) {
		if ( !add_item( &charlistlist, charlistset[i], char **, &charlistlen ) ) {
			fprintf( stderr, "Failed to add charlist to list.\n" );
			exit( 0 );
		}
	}
	fprintf( stderr, "charlistlist should contain %d elements: ", charlistlen );
	iterate_charlist( charlistlist );
	free( charlistlist );


	//Test random character generation
	fprintf( stderr, "RANDOM CHARACTER GENERATION (static):\n" );
	char buf[ 24 ];
	char *a = srand_nums( buf, sizeof(buf) );
	fprintf( stderr, "random nums: %s\n", a );
	char *b = srand_letters( &buf, sizeof(buf) );
	fprintf( stderr, "random letters: %s\n", b );
	char *c = srand_chars( &buf, sizeof(buf) );
	fprintf( stderr, "random chars: %s\n", c );

	fprintf( stderr, "RANDOM CHARACTER GENERATION (dynamic):\n" );
	char *d = mrand_nums( 20 );
	fprintf( stderr, "random nums: %s\n", d );
	free(d);
	char *e = mrand_letters( 33 ); 
	fprintf( stderr, "random letters: %s\n", e );
	free(e);
	char *f = mrand_chars( 42 ); 
	fprintf( stderr, "random chars: %s\n", f );
	free(f);

	//Test append with a bunch of stuff for now...
	fprintf( stderr, "APPEND_TO_UINT8T:\n" );
	const char **ww = words;
	uint8_t *w = NULL;
	int wlen = 0;
	while ( *ww ) {
		w = append_to_uint8t ( &w, &wlen, (uint8_t *)*ww, strlen( *ww ) );
		if ( !w ) {
			fprintf( stderr, "FAILED - append_to_uint8t on '%s'\n", *ww );
			free( w );	
			break;	
		} 
		ww++;
	}

	if ( !append_to_uint8t( &w, &wlen, (uint8_t *)"\0", 1 ) ) {
		fprintf( stderr, "FAILED - failed to terminate appended string.\n" );
		free( w );	
		return 1;
	}

	fprintf( stderr, "SUCCESS - '%s'\n", (char *)w ); 
	free( w );
	return 0;

}
