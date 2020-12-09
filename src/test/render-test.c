#include "../render.h"

#define TESTDIR "tests/render/"

#define SPACE_TEST "sister_cities"

/*Max key searches*/
#define LKV_MAX_SEARCH 10

/*Defines for each key type in a zTable*/
#define LKV_TERM -7

#define LKV_LAST \
	.hash = { LKV_TERM }

#define TEXT_KEY( str ) \
	.key = { LITE_TXT, .v.vchar = str } 

#define TEXT_VALUE( str ) \
	.value = { LITE_TXT, .v.vchar = str }

#define BLOB_KEY( b ) \
	.key = { LITE_BLB, .v.vblob = { sizeof( b ), (unsigned char *)b }}

#define BLOB_VALUE( b ) \
	.value = { LITE_BLB, .v.vblob = { sizeof( b ), (unsigned char *)b }}

#define INT_KEY( b ) \
	.key = { LITE_INT, .v.vint = b } 

#define INT_VALUE( b ) \
	.value = { LITE_INT, .v.vint = b } 

#define FLOAT_VALUE( b ) \
	.value = { LITE_FLT, .v.vfloat = b } 

#define USR_VALUE( b ) \
	.value = { LITE_USR, .v.vusrdata = b } 

#define TABLE_VALUE( b ) \
	.value = { LITE_TBL }

#define NULL_VALUE( ) \
	.value = { LITE_NUL }

#define TRM_VALUE( ) \
	.value = { LITE_TRM }

#define TRM( ) \
	.key = { LITE_TRM }

#define START_TABLEs( str ) \
	.key = { LITE_TXT, .v.vchar = str }, .value = { LITE_TBL }

#define START_TABLEi( num ) \
	.key = { LITE_INT, .v.vint = num }, .value = { LITE_TBL }

#define END_TABLE() \
	.key = { LITE_TRM }
#if 0
struct block { const char *model, *view; } files[] = {
	{ TESTDIR "empty.lua", TESTDIR "empty.tpl" },
	{ TESTDIR "simple.lua", TESTDIR "simple.tpl" },
	{ TESTDIR "castigan.lua", TESTDIR "castigan.tpl" },
	{ TESTDIR "multi.lua", TESTDIR "multi.tpl" },
	{ NULL }
};
#else
/*
All of these are defined seperately, becuase many will be re-used for the tests.
*/
zKeyval NozTable[] = {
	{ TEXT_KEY( "zxy" ), TEXT_VALUE( "def" ) },
	{ TEXT_KEY( "def" ), INT_VALUE( 342 ) },
	{ TEXT_KEY( "ghi" ), INT_VALUE( 245 ) },
	{  INT_KEY( 553   ), INT_VALUE( 455 ) },
	{ TEXT_KEY( "jkl" ), INT_VALUE( 981 ) },
	{  INT_KEY( 8234  ), INT_VALUE( 477 ) },
	{  INT_KEY( 11    ), INT_VALUE( 343 ) },
	{  INT_KEY( 32    ),  INT_VALUE( 3222 ) },
	{ TEXT_KEY( "abc" ), TEXT_VALUE( "Large angry deer are following me." ) },
	{ LKV_LAST } 
};


#if 0
//
zKeyval ST[] = {
	{ TEXT_KEY( "ashor" )  , TABLE_VALUE( )         },
		{ INT_KEY( 7043 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog." )  },
		{ INT_KEY( 7002 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog another time." )  },
		{ TRM() },
	{ LKV_LAST } 
};


zKeyval MT[] = {
	{ INT_KEY( 1 )    , TEXT_VALUE( "Wash the dog." )  },
	{ TEXT_KEY( "ashor" )  , TABLE_VALUE( )         },
		{ INT_KEY( 7043 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog." )  },
		{ INT_KEY( 7002 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog another time." )  },
		{ TRM() },
	{ TEXT_KEY( "artillery" )       , TABLE_VALUE( )         },
		/*Database records look a lot like this*/
		{ INT_KEY( 0 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "val" ), BLOB_VALUE( "MySQL makes me tired." ) },
			{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo." ) },
			{ TRM() },
		{ TRM() },
	{ TEXT_KEY( "michael" )       , TABLE_VALUE( )         },
		{ TEXT_KEY( "val" ), BLOB_VALUE( "MySQL makes me giddy." ) },
		{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo." ) },
		{ TRM() },
	{ TEXT_KEY( "jackson" )       , TABLE_VALUE( )         },
		{ TEXT_KEY( "val" ), BLOB_VALUE( "MySQL makes me ecstatic." ) },
		{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo." ) },
		{ TRM() },
	{ LKV_LAST } 
};
#endif


zKeyval SinglezTable[] = {
	{ TEXT_KEY( "ashor" )  , TABLE_VALUE( )         },
		{ INT_KEY( 7043 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog." )  },
		{ INT_KEY( 7002 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog another time." )  },
		{ INT_KEY( 7003 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog for the 3rd time." )  },
		{ INT_KEY( 7004 )    , USR_VALUE ( NULL ) },
		{ INT_KEY( 7008 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog again." )  },
		{ INT_KEY( 7009 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog for the last time." )  },
		{ TRM() },

	{ TEXT_KEY( "artillery" )       , TABLE_VALUE( )         },
		/*Database records look a lot like this*/
		{ INT_KEY( 0 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "val" ), TEXT_VALUE( "MySQL makes me a bother." ) },
			{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo" ) },
			{ TRM() },
		{ INT_KEY( 1 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo" ) },
			{ TEXT_KEY( "val" ), TEXT_VALUE( "MySQL makes me ecstatic." ) },
			{ TRM() },
		{ INT_KEY( 2 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "val" ), TEXT_VALUE( "MySQL makes me giddy." ) },
			{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo" ) },
			{ TRM() },
		{ INT_KEY( 3 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "val" ), TEXT_VALUE( "MySQL makes me tired." ) },
			{ TEXT_KEY( "rec" ), TEXT_VALUE( "Choo choo cachoo" ) },
			{ TRM() },
		{ TRM() },

	{ LKV_LAST } 
};



zKeyval DoublezTableAlpha[] = {
	{ TEXT_KEY( "cities" )       , TABLE_VALUE( )         },
		/*Database records look a lot like this*/
		{ INT_KEY( 0 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "San Francisco" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "CA" ) },
			{ TEXT_KEY( "desc" ), BLOB_VALUE( "There are so many things to see and do in this wonderful town.  Like talk to a billionaire startup founder or super-educated University of Berkeley professors." ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				//Pay attention to this, I'd like to embed uint8_t data here (I think Lua can handle this)
				{ TEXT_KEY( "claim_to_fame" ), TEXT_VALUE( "The Real Silicon Valley" ) },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 870887 ) },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 1 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "New York" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "NY" ) },
			{ TEXT_KEY( "desc" ), BLOB_VALUE( "New York City is one of the most well-known destinations on earth and home to over 8 million residents." ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				{ TEXT_KEY( "claim_to_fame" ), TEXT_VALUE( "The Greatest City on Earth" ) },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 8750000 ) },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 2 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "Raleigh" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "NC" ) },
			{ TEXT_KEY( "desc" ), BLOB_VALUE( "Otherwise known as the Oak City, around 600,000 residents call Raleigh home." ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				{ TEXT_KEY( "claim_to_fame" ), TEXT_VALUE( "Silicon Valley of the South" ) },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 350001 ) },
				{ TRM() },
			{ TRM() },
		{ TRM() },
	{ LKV_LAST } 
};



zKeyval DoublezTableNumeric[] = {
	{ TEXT_KEY( "cities" )       , TABLE_VALUE( )         },
		/*Database records look a lot like this*/
		{ INT_KEY( 0 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "San Francisco, CA" ) },
			{ TEXT_KEY( "desc" ), TEXT_VALUE( "It reeks of weed and opportunity. You know you want it..." ) },
			{ TEXT_KEY( "population" ), INT_VALUE( 332420 ) },
			{ TEXT_KEY( SPACE_TEST ), TABLE_VALUE( )         },
				{ INT_KEY( 0 ), TEXT_VALUE( "Sydney, Austrailia" ) },
				{ INT_KEY( 1 ), TEXT_VALUE( "Beijing, China" ) },
				{ INT_KEY( 2 ), TEXT_VALUE( "Perth, Australia" ) },
				{ INT_KEY( 3 ), TEXT_VALUE( "Johannesburg, South Africa" ) },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 1 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "Durham, NC" ) },
			{ TEXT_KEY( "population" ), INT_VALUE( 33242 ) },
			{ TEXT_KEY( SPACE_TEST ), TABLE_VALUE( )         },
				{ INT_KEY( 0 ), TEXT_VALUE( "Arusha, Tanzania" ) },
				{ INT_KEY( 1 ), TEXT_VALUE( "Durham, United Kingdom" ) },
				{ INT_KEY( 2 ), TEXT_VALUE( "Kostroma, Russia" ) },
				{ INT_KEY( 3 ), TEXT_VALUE( "Toyama, Japan" ) },
				{ INT_KEY( 4 ), TEXT_VALUE( "Zhuzhou, Hunan Province, China" ) },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 2 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "Tampa, FL" ) },
			{ TEXT_KEY( "desc" ), TEXT_VALUE( "It's home..." ) },
			{ TEXT_KEY( "population" ), INT_VALUE( 777000 ) },
			{ TEXT_KEY( SPACE_TEST ), TABLE_VALUE( )         },
				{ INT_KEY( 0 ), TEXT_VALUE( "Agrigento, Sicily" ) },
				{ INT_KEY( 1 ), TEXT_VALUE( "Ashdod South, Isreal" ) },
				{ INT_KEY( 2 ), TEXT_VALUE( "Barranquilla, Colombia" ) },
				{ INT_KEY( 3 ), TEXT_VALUE( "Boca del Rio, Veracruz" ) },
				{ TRM() },
			{ TRM() },
		{ TRM() },

	{ TEXT_KEY( "ashor" )  , TABLE_VALUE( )         },
		{ INT_KEY( 7043 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog." )  },
		{ INT_KEY( 7002 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog another time." )  },
		{ INT_KEY( 7003 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog for the 3rd time." )  },
		{ INT_KEY( 7004 )    , USR_VALUE ( NULL ) },
		{ INT_KEY( 7008 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog again." )  },
		{ INT_KEY( 7009 )    , TEXT_VALUE( "The quick brown fox jumps over the lazy dog for the last time." )  },
		{ TRM() },
	{ LKV_LAST } 
};


zKeyval MultiLevelzTable[] = {
	{ TEXT_KEY( "cities" )       , TABLE_VALUE( )         },
		/*Database records look a lot like this*/
		{ INT_KEY( 0 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "San Francisco" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "CA" ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				//Pay attention to this, I'd like to embed uint8_t data here (I think Lua can handle this)
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 870887 ) },
				{ TEXT_KEY( "demographics" ), TABLE_VALUE( )         },
#if 0
					{ TEXT_KEY( "Black" ),    INT_VALUE( 5.5 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 40.5 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 35.4 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 15.2 ) },
#else
					{ TEXT_KEY( "Black" ),    INT_VALUE( 5 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 40 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 35 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 15 ) },
#endif
					{ TEXT_KEY( "Other" ),    INT_VALUE( 20 ) },
					{ TRM() },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 1 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "New York" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "NY" ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 19750000 ) },
				{ TEXT_KEY( "demographics" ), TABLE_VALUE( )         },
#if 0
					{ TEXT_KEY( "Black" ),    INT_VALUE( 17.7 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 55.8 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 8.9 ) },
#else
					{ TEXT_KEY( "Black" ),    INT_VALUE( 17 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 55 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 8 ) },
#endif
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 19 ) },
					{ TEXT_KEY( "Other" ),    INT_VALUE( 13 ) },
					{ TRM() },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 2 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "Raleigh" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "NC" ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 870887 ) },
				{ TEXT_KEY( "demographics" ), TABLE_VALUE( )         },
#if 0
					{ TEXT_KEY( "Black" ),    INT_VALUE( 28.4 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 57.74 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 4.69 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 11.81 ) },
#else
					{ TEXT_KEY( "Black" ),    INT_VALUE( 28 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 57 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 4 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 11 ) },
#endif
					{ TEXT_KEY( "Other" ),    INT_VALUE( 9 ) },
					{ TRM() },
				{ TRM() },
			{ TRM() },
		{ TRM() },
	{ LKV_LAST } 
};


#if 0
zKeyval MultiLevelzTableExtreme[] = {
	{ TEXT_KEY( "cities" )       , TABLE_VALUE( )         },
		/*Database records look a lot like this*/
		{ INT_KEY( 0 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "San Francisco" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "CA" ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				//Pay attention to this, I'd like to embed uint8_t data here (I think Lua can handle this)
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 870887 ) },
				{ TEXT_KEY( "demographics" ), TABLE_VALUE( )         },
#if 0
					{ TEXT_KEY( "Black" ),    INT_VALUE( 5.5 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 40.5 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 35.4 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 15.2 ) },
#else
					{ TEXT_KEY( "Black" ),    INT_VALUE( 5 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 40 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 35 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 15 ) },
#endif
					{ TEXT_KEY( "Other" ),    INT_VALUE( 20 ) },
					{ TRM() },
				{ TEXT_KEY( "sisterCities" ), TABLE_VALUE( )         },
					{ INT_KEY( 0 ), TEXT_VALUE( "Sydney, Austrailia" ) },
					{ INT_KEY( 1 ), TEXT_VALUE( "Beijing, China" ) },
					{ INT_KEY( 2 ), TEXT_VALUE( "Perth, Australia" ) },
					{ INT_KEY( 3 ), TEXT_VALUE( "Johannesburg, South Africa" ) },
					{ TRM() },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 1 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "New York" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "NY" ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 19750000 ) },
				{ TEXT_KEY( "demographics" ), TABLE_VALUE( )         },
#if 0
					{ TEXT_KEY( "Black" ),    INT_VALUE( 17.7 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 55.8 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 8.9 ) },
#else
					{ TEXT_KEY( "Black" ),    INT_VALUE( 17 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 55 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 8 ) },
#endif
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 19 ) },
					{ TEXT_KEY( "Other" ),    INT_VALUE( 13 ) },
					{ TRM() },
				{ TEXT_KEY( "sisterCities" ), TABLE_VALUE( )         },
					{ INT_KEY( 0 ), TEXT_VALUE( "Arusha, Tanzania" ) },
					{ INT_KEY( 1 ), TEXT_VALUE( "Durham, United Kingdom" ) },
					{ INT_KEY( 2 ), TEXT_VALUE( "Kostroma, Russia" ) },
					{ INT_KEY( 3 ), TEXT_VALUE( "Toyama, Japan" ) },
					{ INT_KEY( 4 ), TEXT_VALUE( "Zhuzhou, Hunan Province, China" ) },
					{ TRM() },
				{ TRM() },
			{ TRM() },

		{ INT_KEY( 2 )       , TABLE_VALUE( )         },
			{ TEXT_KEY( "city" ), BLOB_VALUE( "Raleigh" ) },
			{ TEXT_KEY( "parent_state" ), TEXT_VALUE( "NC" ) },
			{ TEXT_KEY( "metadata" ), TABLE_VALUE( )         },
				{ TEXT_KEY( "skyline" ), BLOB_VALUE( "CA" ) },
				{ TEXT_KEY( "population" ), INT_VALUE( 870887 ) },
				{ TEXT_KEY( "demographics" ), TABLE_VALUE( )         },
#if 0
					{ TEXT_KEY( "Black" ),    INT_VALUE( 28.4 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 57.74 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 4.69 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 11.81 ) },
#else
					{ TEXT_KEY( "Black" ),    INT_VALUE( 28 ) },
					{ TEXT_KEY( "White" ),    INT_VALUE( 57 ) },
					{ TEXT_KEY( "Asian" ),    INT_VALUE( 4 ) },
					{ TEXT_KEY( "Hispanic" ), INT_VALUE( 11 ) },
#endif
					{ TEXT_KEY( "Other" ),    INT_VALUE( 9 ) },
					{ TRM() },
				{ TEXT_KEY( "sisterCities" ), TABLE_VALUE( )         },
					{ INT_KEY( 0 ), TEXT_VALUE( "Agrigento, Sicily" ) },
					{ INT_KEY( 1 ), TEXT_VALUE( "Ashdod South, Isreal" ) },
					{ TRM() },
				{ TRM() },
			{ TRM() },
		{ TRM() },
	{ LKV_LAST } 
};
#endif
#endif


int get_count ( zKeyval *kv ) {
	int a = 0;
	while ( *kv->hash != LKV_TERM ) 
		a ++, kv ++;
	return a;	
}


zTable *convert_lkv ( zKeyval *kv ) {
	char buf[ 2048 ];
	zTable *t = malloc( sizeof( zTable ) );
	lt_init( t, NULL, get_count( kv ) );

	while ( *kv->hash != LKV_TERM ) {
		if ( kv->key.type == LITE_TXT )
			lt_addtextkey( t, kv->key.v.vchar );
		else if ( kv->key.type == LITE_BLB )
			lt_addblobkey( t, kv->key.v.vblob.blob, kv->key.v.vblob.size );
		else if ( kv->key.type == LITE_INT )
			lt_addintkey( t, kv->key.v.vint );
		else if ( kv->key.type == LITE_TRM ) {
			lt_ascend( t );
			kv ++;
			continue;
		}
		else {
			//Abort immediately b/c this is an error
			fprintf( stderr, "%s: %d - Attempted to add wrong key type!  Bailing!", __FILE__, __LINE__ );
			exit( 0 );	
		}

		if ( kv->value.type == LITE_TXT )
			lt_addtextvalue( t, kv->value.v.vchar );
		else if ( kv->value.type == LITE_BLB )
			lt_addblobvalue( t, kv->value.v.vblob.blob, kv->value.v.vblob.size );
		else if ( kv->value.type == LITE_INT )
			lt_addintvalue( t, kv->value.v.vint );
		else if ( kv->value.type == LITE_FLT )
			lt_addfloatvalue( t, kv->value.v.vfloat );
		else if ( kv->value.type == LITE_USR )
			lt_addudvalue( t, kv->value.v.vusrdata );
		else if ( kv->value.type == LITE_NUL )
			0;//lt_ascend has already been called, thus we should never reach this
		else { 
			if ( kv->value.type != LITE_TBL ) {
				//Abort immediately b/c this is an error
				fprintf( stderr, "%s: %d -  Got unknown or invalid key type!  Bailing!", __FILE__, __LINE__ );
				return NULL;
			}
		}

		if ( kv->value.type != LITE_TBL )
			lt_finalize( t );
		else {
			lt_descend( t );
			kv ++;
			continue;
		}
		kv ++;
	}
	lt_lock( t );
	return t;
}


#define TEST(k,d) \
	{ #k, k }

struct Test {
	zKeyval *kvset;
	const char *name, *desc;
	const char *src, *cmp;
};

#if 0
struct Test tests[] = {
	TEST(NozTable),
	TEST(SinglezTable),
	TEST(DoublezTableAlpha),
	TEST(DoublezTableNumeric),
	TEST(MultiLevelzTable),
	{ NULL }
};
#endif

struct Test tests[] = 
{
	#if 0
	//These should be pretty easy to read:
	//.name   = Name of the test
	//.desc   = A quick description of the test
	//.renSrc = the input that the test will use for find and replace
	//.renCmp = the constant to compare against to make sure that rendering worked
	//.values = the zTable to use for values (these tests do not test any parsing)
	{
		NozTable, "TABLE_NONE", "Template values with no tables.",
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "	<h2>{{ ghi }}</h2>\n"
		 "	<p>\n"
		 "		{{ abc }}\n"
		 "	</p>	\n"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "	<h2>245</h2>\n"
		 "	<p>\n"
		 "		Large angry deer are following me.\n"
		 "	</p>	\n"
		 "</body>\n"
		 "</html>\n"
	},
	#endif

	#if 1
	//one table
	{
		SinglezTable, "TABLE_SINGLE", "one level table",
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "{{# artillery }}"
		 "	<h2>{{ .rec }}</h2>\n"
		 "	<p>\n"
		 "		{{ .val }}\n"
		 "	</p>\n"
		 "{{/ artillery }}"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me tired.\n"
		 "	</p>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me giddy.\n"
		 "	</p>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me ecstatic.\n"
		 "	</p>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me a bother.\n"
		 "	</p>\n"
		 "</body>\n"
		 "</html>\n"
	},
	#endif

	#if 0
	{
		DoublezTableAlpha, "TABLE_DOUBLE", "two level table | key value test",
		//Notice the cities.metadata loop block.  Teset for short and long keys...
		//"    {{ .city }}, {{ .parent_state }} is a city full of {{ .metadata.population }} people.\n" 
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "{{# cities }}\n"
		 "\t<h2>{{ .city }}</h2>\n"
		 "\t<p>\n"
		 "\t\t{{ .city }}, {{ .parent_state }} is a city containing {{ .metadata.population }} people.\n" 
		 "\t</p>\n"
		 "\t<p>\n"
		 "\t\t{{ .desc }}\n"
		 "\t</p>\n"
		 "\t<table>\n"
		 "\t<thead>\n"
		 "\t\t<th>Skyline</th>\n"
		 "\t\t<th>Population</th>\n"
		 "\t</thead>\n"
		 "\t<tbody>\n"
		 "\t{{# .metadata }}\n"
		 "\t\t<tr>\n"
		 "\t\t\t<td>Population: {{ .population }}</td>\n"  
		 "\t\t\t<td>Claim to Fame: {{ cities.metadata.claim_to_fame }}</td>\n"
		 "\t\t</tr>\n"
		 "\t{{/ .metadata }}\n"
		 "\t</tbody>\n"
		 "\t</table>\n"
		 "{{/ cities }}\n"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "\n"
		 "	<h2>San Francisco</h2>\n"
		 "	<p>\n"
		 "    San Francisco, CA is a city full of 870887 people.\n" 
		 "	</p>\n"
		 "	<p>\n"
		 "  	There are so many things to see and do in this wonderful town.  Like talk to a billionaire startup founder or super-educated University of Berkeley professors.\n" 
		 "	</p>\n"
		 "	<table>\n"
		 "	<thead>\n"
		 "		<th>Skyline</th>\n"
		 "		<th>Population</th>\n"
		 "	</thead>\n"
		 "	<tbody>\n"
		 "    <tr>\n"
		 "			<td>Population: 870887</td>\n"  
		 "			<td>Claim to Fame: The Real Silicon Valley</td>\n"
		 "    </tr>\n"
		 "	</tbody>\n"
		 "	</table>\n"
		 "\n"
		 "	<h2>New York</h2>\n"
		 "	<p>\n"
		 "    New York is a city full of 8750000 people.\n" 
		 "	</p>\n"
		 "	<p>\n"
		 "    New York, NY is one of the most well-known destinations on earth and home to over 8 million residents.\n"
		 "	</p>\n"
		 "	<table>\n"
		 "	<thead>\n"
		 "		<th>Skyline</th>\n"
		 "		<th>Population</th>\n"
		 "	</thead>\n"
		 "	<tbody>\n"
		 "    <tr>\n"
		 "			<td>Population: 19750000</td>\n"  
		 "			<td>Claim to Fame: The Greatest City on Earth</td>\n"
		 "    </tr>\n"
		 "	</tbody>\n" "	</table>\n"
		 "\n"
		 "	<h2>Raleigh</h2>\n"
		 "	<p>\n"
		 "    Raleigh, NC is a city full of 350001 people.\n" 
		 "	</p>\n"
		 "	<table>\n"
		 "	<thead>\n"
		 "		<th>Skyline</th>\n"
		 "		<th>Population</th>\n"
		 "	</thead>\n"
		 "	<tbody>\n"
		 "    <tr>\n"
		 "			<td>350001</td>\n"  
		 "			<td>Silicon Valley of the South</td>\n"
		 "    </tr>\n"
		 "	</tbody>\n"
		 "	</table>\n"
		 "</body>\n"
		 "</html>\n"
	},
#endif
	#if 1
	{
		DoublezTableAlpha, "TABLE_DOUBLE", "two level table | key value test",
		//Notice the cities.metadata loop block.  Teset for short and long keys...
		//"    {{ .city }}, {{ .parent_state }} is a city full of {{ .metadata.population }} people.\n" 
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "{{# cities }}\n"
		 "\t<h2>{{ .city }}</h2>\n"
		 "\t<p>\n"
		 "\t\t{{ .city }}, {{ .parent_state }} is a city containing {{ .metadata.population }} people.\n" 
		 "\t</p>\n"
		 "\t<p>\n"
		 "\t\t{{ .desc }}\n"
		 "\t</p>\n"
		 "\t<table>\n"
		 "\t<thead>\n"
		 "\t\t<th>Skyline</th>\n"
		 "\t\t<th>Population</th>\n"
		 "\t</thead>\n"
		 "\t<tbody>\n"
		 "\t{{# .metadata }}\n"
		 "\t\t<tr>\n"
		 "\t\t\t<td>Population: {{ .population }}</td>\n"  
		 "\t\t\t<td>Claim to Fame: {{ cities.metadata.claim_to_fame }}</td>\n"
		 "\t\t</tr>\n"
		 "\t{{/ .metadata }}\n"
		 "\t</tbody>\n"
		 "\t</table>\n"
		 "{{/ cities }}\n"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "\n"
		 "	<h2>San Francisco</h2>\n"
		 "	<p>\n"
		 "    San Francisco, CA is a city full of 870887 people.\n" 
		 "	</p>\n"
		 "	<p>\n"
		 "  	There are so many things to see and do in this wonderful town.  Like talk to a billionaire startup founder or super-educated University of Berkeley professors.\n" 
		 "	</p>\n"
		 "	<table>\n"
		 "	<thead>\n"
		 "		<th>Skyline</th>\n"
		 "		<th>Population</th>\n"
		 "	</thead>\n"
		 "	<tbody>\n"
		 "    <tr>\n"
		 "			<td>Population: 870887</td>\n"  
		 "			<td>Claim to Fame: The Real Silicon Valley</td>\n"
		 "    </tr>\n"
		 "	</tbody>\n"
		 "	</table>\n"
		 "\n"
		 "	<h2>New York</h2>\n"
		 "	<p>\n"
		 "    New York is a city full of 8750000 people.\n" 
		 "	</p>\n"
		 "	<p>\n"
		 "    New York, NY is one of the most well-known destinations on earth and home to over 8 million residents.\n"
		 "	</p>\n"
		 "	<table>\n"
		 "	<thead>\n"
		 "		<th>Skyline</th>\n"
		 "		<th>Population</th>\n"
		 "	</thead>\n"
		 "	<tbody>\n"
		 "    <tr>\n"
		 "			<td>Population: 19750000</td>\n"  
		 "			<td>Claim to Fame: The Greatest City on Earth</td>\n"
		 "    </tr>\n"
		 "	</tbody>\n" "	</table>\n"
		 "\n"
		 "	<h2>Raleigh</h2>\n"
		 "	<p>\n"
		 "    Raleigh, NC is a city full of 350001 people.\n" 
		 "	</p>\n"
		 "	<table>\n"
		 "	<thead>\n"
		 "		<th>Skyline</th>\n"
		 "		<th>Population</th>\n"
		 "	</thead>\n"
		 "	<tbody>\n"
		 "    <tr>\n"
		 "			<td>350001</td>\n"  
		 "			<td>Silicon Valley of the South</td>\n"
		 "    </tr>\n"
		 "	</tbody>\n"
		 "	</table>\n"
		 "</body>\n"
		 "</html>\n"
	},
#endif
	{
		DoublezTableNumeric, "two level table", "two level table | key value test",
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "{{# cities }}\n"
		 "	<h2>{{ .city }}</h2>\n"
		 "	<p>\n"
		 "		{{ .city }} is a city full of {{ .population }} people.\n" 
		 "	</p>\n"
		 "	<p>\n"
		 "    {{ .desc }}\n"
		 "	</p>\n"
		 "	<ul>\n"
		 "	{{# ." SPACE_TEST " }}\n"
		 "		<li>{{ $value }}</li>\n"  /*Notice that this is the only way to do this*/
		 "	{{/ ." SPACE_TEST " }}\n"
		 "	</ul>\n"
		 "{{/ cities }}\n"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me tired.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me giddy.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me ecstatic.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me a bother.\n"
		 "	</p>	\n"
		 "</body>\n"
		 "</html>\n"
	},

#if 0
	//If you can't solve it, don't beat yourself up over it.
	//Either try something else or approach it a different way...
	{
		DoublezTableNumeric, "two level table", "two level table | key value test",
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "{{# cities }}\n"
		 "	<h2>{{ .city }}</h2>\n"
		 "	<p>\n"
		 "    {{ .city }} is a city full of {{ .population }} people.\n" 
		 "	</p>\n"
		 "	<p>\n"
		 "    {{ .desc }}\n"
		 "	</p>\n"
		 "	<ul>\n"
		 "	{{# cities." SPACE_TEST " }}\n"
		 "		<li>{{ $value }}</li>\n"  /*Notice that this is the only way to do this*/
		 "	{{/ cities." SPACE_TEST " }}\n"
		 "	</ul>\n"
		 "{{/ cities }}\n"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me tired.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me giddy.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me ecstatic.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me a bother.\n"
		 "	</p>	\n"
		 "</body>\n"
		 "</html>\n"
	},
	//multi-level tables
	{
		MultiLevelzTable, "key and value", "key and value",
		.src =
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "{{# artillery }}\n"
		 "	<h2>{{$ key }}</h2>\n"
		 "	<p>\n"
		 "		{{$ val }}\n"
		 "	</p>	\n"
		 "{{/ artillery }}\n"
		 "</body>\n"
		 "</html>\n"
		,
		.cmp = 
		 "<html>\n"
		 "<head>\n"
		 "</head>\n"
		 "<body>\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me tired.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me giddy.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me ecstatic.\n"
		 "	</p>	\n"
		 "	<h2>Choo choo cachoo</h2>\n"
		 "	<p>\n"
		 "		MySQL makes me a bother.\n"
		 "	</p>	\n"
		 "</body>\n"
		 "</html>\n"
	},
#endif
	{ NULL }
};


int main (int argc, char *argv[]) {
	char err[ 2048 ] = { 0 };
	struct Test *test = tests;

	while ( test->kvset ) {
		fprintf( stderr, "%s %p\n", test->name, test->kvset );
		zTable *t = convert_lkv( test->kvset );
		int rlen = 0;
		uint8_t *r = NULL;

		//Finding the marks is good if there is enough memory to do it
		if (( r = table_to_uint8t( t, (uint8_t *)test->src, strlen(test->src), &rlen ) ) == NULL ) {
			fprintf(stderr, "Error rendering template at item: %s\n", test->name );
			goto die;	
		}

		//fprintf( stderr, "%p, %d\n", r, rlen ); write( 2, r, rlen ); getchar();
		int cmp = memcmp( r, test->cmp, rlen ); 
		fprintf( stderr, "%s\n", cmp ? "FAILED" : "SUCCESS" );
die:
		free( r );

		if ( t ) { 
			lt_free( t );
			free( t );
		}
		test++;
	}
	return 0;
}
