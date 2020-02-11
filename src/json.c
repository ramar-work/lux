/*Procthatjson*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "obsidian/parsely.h"
#include "obsidian/tab.h"

#define JSON_MAX_DEPTH 100

#define xerr(c, ...) \
	(fprintf( stderr, __VA_ARGS__ ) ? c : c)

#ifndef DEBUG_JSON
 #define qprintf( ... )
 #define dump(...) 

#else	

 #define qprintf( ... ) \
	fprintf( stderr, __VA_ARGS__ );

 #define dump( ... ) \
	json_dump( __VA_ARGS__ )
#endif


static uint8_t rootname[] = "root";
typedef struct { int text, type, key, index; } Depth; 


//Trim any characters 
static unsigned char *json_trim (uint8_t *msg, char *trim, int len, int *nlen) 
{
	//Define stuff
	uint8_t *m = msg;
	int nl= len;
	//Move forwards and backwards to find whitespace...
	while ( memchr(trim, *(m + ( nl - 1 )), strlen(trim)) && nl-- ) ; 
	while ( memchr(trim, *m, strlen(trim)) && nl-- ) m++;
	*nlen = nl;
	return m;
}



#if 0
/*Slower, but cleaner*/
int json_reset ( Table *t, uint8_t **b, int *a, int side  )
{
	dump( *b, *a, "Adding value" );
	if ( side )
	{
		if ( lt_addbk( t, *b, *a ) ) 
		{
			fprintf( stderr, "%s\n", lt_strerror( t ));
			return 0;
		}
	}	
	else 
	{
		if ( lt_addbv( t, *b, *a ) ) 
		{
			fprintf( stderr, "%s\n", lt_strerror( t ));
			return 0;
		}
	}

	*b = NULL;
	*a = 0;
	return 1;
}
#endif



//Dump
#ifdef DEBUG_JSON
static void json_dump (uint8_t *a, int b, const char *msg)
{
	(msg) ? fprintf( stderr, "%s: ", msg ) : 0;
	write( 2, "'", 1 );
	write( 2, a, b );
	write( 2, "'\n", 2 );
}
#endif



//Get an approximation of the number of keys needed
int json_count ( uint8_t *src, int len )
{
	int sz = 0;
	int c = len;

	//You can somewhat gauge the size needed by looking for all commas
	for ( c=len; c; c-- )
		sz += ( memchr( "{[,]}", *src,  5 ) ) ? 1 : 0, src++;

	return sz;
}



//A very raw version that uses no special anything
int json_json ( Table *t, uint8_t *src, int len )
{
	//Define
	Depth depther[ JSON_MAX_DEPTH ];
	Depth *depth = &depther[0];
	int dwatch = 0;
	int sz = json_count( src, len ); 
	int adjust = 0;
	unsigned char *b = NULL;
	Parser p = { 
		.words = {
			{ (char *)"\"", "\"", "\\"  },
			{ "{" },
			{ "["   },
			{ "}" },
			{ "]"   },
			{ ":"  },
			{ ","  },
			{ NULL  }
		}
	};

	//Initialize the table structure (we are making some serious approximations)
	if ( !lt_init( t, NULL, sz ) )
		return xerr( 1, "Did not init lt...\n" );

	//Prepare everything
	memset( depther, 0, sizeof(Depth) * JSON_MAX_DEPTH );	
	pr_prepare( &p );	
	depth->type = 1;
	lt_addblobkey( t, rootname, 4 );

	//Some stats
	#if 0
	fprintf( stderr, "Read %d bytes from file '%s'\n", bytes, file );
	fprintf( stderr, "Requested size of %d for hash table\n", sz + 1000 );
	fprintf( stderr, "Initializing hash table with %d keys\n", t.modulo );
	#endif

	//make a JSON table out of this
	while ( pr_next( &p, src, len ) )
	{
		if (p.word == NULL)	
			break; 

		//Save reference to whatever string was found
		if ( *p.word == '"' )
		{
			b = json_trim( &src[ p.prev ], "\"' \t\n\r", p.size, &adjust );
			depth->text = 1;  
		}

		//Descend into a table.
		else if ( *p.word == '{' )
		{
			if ( !depth->type ) 
			{
				if ( lt_addintkey( t, depth->index++ ) == 0 )
				{
					qprintf( "%s\n", lt_strerror( t ));
					return 0;
				}
			}

			qprintf( "Switching to alphabetical indices...\n" );
			lt_descend( t );
			( dwatch < 100 ) ? dwatch++, depth++ : 0;
			depth->type = 1;
		}

		//Descend into a table.
		else if ( *p.word == '[' ) 
		{
			qprintf( "Switching to numeric indices...\n" );
			lt_descend( t );	
			if ( lt_addintkey( t, depth->index++ ) == 0 )
			{
				fprintf( stderr, "%s\n", lt_strerror( t ));
				return 0;
			}

			( dwatch < 100 ) ? dwatch++, depth++ : 0;
			depth->type = 0;
		}

		//There should always be a key in front of this
		else if ( *p.word == ':' )
		{	
			if ( depth->text )
			{
				dump( b, adjust, "Adding key" );
				lt_addbk( t, b, adjust );
				b = NULL;
				adjust = 0;
			}

			depth->text = 0;
			depth->key = 1;
		}

		//There is most likely a value before this
		else if ( *p.word == ',' )
		{
			//Add the key if there was text
			if ( depth->text ) 
			{
				dump( b, adjust, "Adding value" );
				lt_addbv( t, b, adjust );
				lt_finalize( t );
				b = NULL;
				adjust = 0;
				depth->text = 0;
			}

			depth->key = 0;
		}


		//Jump out (but we could be potentially be somewhere else)
		else if ( *p.word == '}' )
		{
			if ( depth->key )
			{
				dump( b, a, "Adding value" );
				if ( depth->type ) 
				{
					lt_addbv( t, b, adjust );
					lt_finalize( t );	
					b = NULL;
					adjust = 0;
					depth->text = 0;
				}
			}

			lt_ascend( t );	
			memset( depth, 0, sizeof(Depth));	
			( dwatch > 0 ) ? dwatch--, depth-- : 0;
		}

		//Jump out (but we could be potentially be somewhere else)
		else if ( *p.word == ']' )
		{
			lt_ascend( t );
			memset( depth, 0, sizeof(Depth));	
			( dwatch > 0 ) ? dwatch--, depth-- : 0;
		}

		qprintf( stderr, "match is:      %c\n", *p.word );
		dump( &src[ p.prev ], p.size, "Current value" );
		qprintf( stderr, "level:         %d\n", dwatch);
		qprintf( stderr, "keyType:       %s\n", !depth->type ? "numeric" : "alpha" );
	}

	lt_lock( t );
	lt_printall( t );
	return 1;
}



//...
void json_free ( Table *t )
{
	lt_free( t );
}



/*This tests that this works with a large dataset...*/
int main ( int argc, char *argv[] ) 
{
	//...
	Table t;
	char *file = NULL;
	unsigned char *src = NULL, *ns = NULL;
	int bytes=0, fd = 0;
	struct stat sb;


	//Check argv
	if ( argc < 2 )
	{
		fprintf( stderr, "No args supplied.  please specify filename.\n" );
		fprintf( stderr, "Usage:\n%s [ filename ]\n", argv[0] );
		return 0;
	}
	else 
	{
		file = argv[1];		
		memset( &sb, 0, sizeof(struct stat) );
		//fprintf( stderr, "file: %s\n", file );
		if ( !file || stat( file, &sb) == -1 ) 
		{
			return xerr( 1, "Failed to stat file: %s\n", file );
		}
	}

	//Open the file
	if ( (fd = open( file, O_RDONLY )) == -1 )
		return xerr( 1, "You suck at life...\n" );

	//Out of laziness, read all into memory
	if ( !(ns = src = malloc( sb.st_size )) )
		return xerr( 1, "You fail at mallocing...\n" );

	//read it all (but it won't come at once...)
	for ( int rd=0; rd != -1 && rd != sb.st_size; )
		bytes += ((rd = read( fd, &src[ bytes ], sb.st_size )) == -1) ? 0 : rd;

	//Close a file
	if ( close( fd ) == -1 )
		return xerr( 1, "Couldn't close the file." );

	//Dump the content first (if you want)
	//write( 2, src, sb.st_size );

	//....
	if ( !json_json( &t, src, bytes ) )
		return xerr( 1, "You're going to die...\n" );

	lt_dump( &t );
	json_free( &t );
	free( src ); 
	return 0;
}
