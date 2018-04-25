//"Manages" sites by creating directories for new projects
//Programs will be much cleaner if opt gives a way to run
//things by pointer...
#include "single.h"
#include <dirent.h>

typedef struct
{
	char *dirname,
       *sitename,
       *domain,
       *home,
       *confdir,
			 *dbpath;
} UserArgs;

typedef struct 
{
	const char *name;
	char type;
	int mode;
} File;


File Filelist[] = 
{
	{ "app", 'd', S_IRWXU },
	{ "assets", 'd', S_IRWXU },
	{ "data", 'd', S_IRWXU },
	{ "db", 'd', S_IRWXU },
	{ "files", 'd', S_IRWXU },
	{ "orm", 'd', S_IRWXU  },
	{ "views", 'd', S_IRWXU  },
	{ NULL }
};


static const char ListTmpl[] =
	"{{# list }}"
	"\t{{ .id }}: {{ .sitename }} - {{ .location }} - {{ .date_created }}\n "
	"{{/ list }}"
;

static const char ConfDir[] = ".sitemgr";

static const char DbName[] = "sitemgr.db";

static const char ListStmt[] =
"SELECT * FROM sitelist" 
;

static const char CreateStmt[] =
 "CREATE TABLE sitelist ("
 " id INTEGER PRIMARY KEY AUTOINCREMENT,"
 " unique_id TEXT," /*A hash for quick lookup*/
 " sitename TEXT,"
 " location TEXT," /*Absolute path for now*/ 
 " date_created TEXT,"
 " date_created_i INTEGER"
");"
;

static const char AddStmt[] =
 "INSERT INTO sitelist VALUES ( NULL, ?1, ?2, ?3, ?4, ?5 )"
;

static const char CheckStmt[] =
 "SELECT id FROM sitelist WHERE unique_id = ?1"
;

static const char Metadata_template[] =
	"backend: Lua\n"
	"name: %s\n"
	"domain: %s\n"
;


void print_new (UserArgs *args )
{
	fprintf( stderr, "%s\n", args->dirname );
	fprintf( stderr, "%s\n", args->sitename );
	fprintf( stderr, "%s\n", args->domain );
	fprintf( stderr, "%s\n", args->home );
	fprintf( stderr, "%s\n", args->confdir );
	fprintf( stderr, "%s\n", args->dbpath );
}


int init_new ( UserArgs *args )
{
	char *home = NULL; 
	char *file = NULL;
	char *dbname = NULL;
	struct stat sb;

 	args->home = home = getenv( "HOME" ) ? getenv( "HOME" ) : "."; 
	args->confdir = strcmbd( "/", home, ConfDir );
	args->dbpath = strcmbd( "/", home, ConfDir, DbName );

	if ( stat( args->confdir, &sb ) > -1 ) 
		return err( 0, "File already exists." ); //Not fatal...

	if ( mkdir( args->confdir, S_IRWXU ) == -1 )
		return err( 0, "Error creating directory: %s\n", strerror( errno ) );

	if ( !sq_create_oneshot( args->dbpath, CreateStmt ) )
		return err( 0, "Could not create new database." );

	return 1;
}


void free_new ( UserArgs *args )
{
	free ( args->confdir );
	free ( args->dbpath );
}


int create_new ( UserArgs *args )
{
	File *f = Filelist;
	char *file = NULL;
	int   fd = 0;
	char *hash = 0;
	char  md[ 2048 ] = { 0 };

	if ( !args->dirname )
		return 0;
	else 
	{
#if 1
		//Hash first 12 characters to make a unique id
		SQWrite SqlCheck[] = 
		{{ SQ_TXT, .v.c = ( hash = (char *)shash_long( args->sitename, 4 ) ) }};
	#if 0
		if ( sq_insert_oneshot( args->dbpath, CheckStmt, SqlCheck ) )
		{ //If the hash exists, die...
			return err( 0, "Site bla already exists..." );
		}
	#endif
#endif
		
		//Create directories
		mkdir( args->dirname, S_IRWXU );
		while ( f->name != NULL ) 
		{
			file = strcmbd( "/", args->dirname, f->name );
			fprintf( stderr, "Creating directory: %s\n", file );
			( f->type == 'd' ) ? mkdir( file, f->mode ) : 0;
			free( file ); 
			f++;
		}

		//Generate metadata
		file = strcmbd( "/", args->dirname, "Metadata" );
		fprintf( stderr, "Writing metadata to file: %s\n", file );
		if ( (fd = open( file, O_CREAT | O_RDWR, S_IRWXU )) == -1 )
			return err( 0, "Error opening file: %s\n", strerror( errno ) );
	
		snprintf( md, 2047, Metadata_template, args->sitename, args->domain );
		fprintf( stderr, "%s\n", md );
		write( fd, md, strlen( md )); 
		close( fd );


		//Add a record to a database
		SQWrite SqlImporter[] = 
		{
			{ SQ_TXT, .v.c = hash },
			{ SQ_TXT, .v.c = (char *)args->sitename },
			{ SQ_TXT, .v.c = (char *)args->dirname },
			{ SQ_DTE },
			{ SQ_DTE },
			{ .sentinel = 1 }
		};

		if ( !sq_insert_oneshot( args->dbpath, AddStmt, SqlImporter ) )
			return err( 0, "Failed to add database record for new site." );
	}

	return 1;
}


void list_sites ( UserArgs *args )
{
#if 0	
	//Uses sq_save and saves you time and trouble...
	Table *t = sq_read_oneshot( args->dbpath, GetStmt );
	render_quick( t, block, blocklen );
#else
	Database db; 
	Render r;
	Buffer *rr = NULL;	
	sq_open( &db, args->dbpath );
	sq_save( &db, ListStmt, "list", NULL );
	render_init( &r, &db.kvt );
	render_map( &r, (uint8_t *)ListTmpl, strlen( ListTmpl ));
	render_render( &r ); 
	rr = render_rendered( &r );
	write( 2, bf_data( rr ), bf_written( rr ));
	sq_close( &db );
	render_free( &r );
#endif
}



Option opts[] =
{
	{ "-a", "--at",       "Create a new application directory here.",'s' },
	{ "-l", "--list",     "List all sites and their statuses." },
	{ "-n", "--name",     "Use this as a site name.", 's' },
	{ "-d", "--domain",   "Use this domain.", 's' },
	{ "-e", "--eat",      "Feed this a certain URL and see how it evaluates.",'s' },
	{ "-v", "--verbose",  "Be verbose." },
	{ "-h", "--help",     "Show help and quit." },
	{ .sentinel = 1 }
};


int main (int argc, char *argv[])
{
	UserArgs ua;
	memset( &ua, 0, sizeof(UserArgs));
#if 0
	ua.dirname = "Alton";
	ua.sitename = "Alton's blues";
	ua.domain = "www.alton-sterling.com";	
#else
	( argc < 2 ) ? opt_usage( opts, 0, "nothing to do.", 0 ) : opt_eval( opts, argc, argv );
	ua.dirname = opt_get( opts, "--at" ).s;
	ua.sitename = opt_get( opts, "--name" ).s;
	ua.domain = opt_get( opts, "--domain" ).s;
#endif
	init_new( &ua );
	print_new( &ua );
	create_new( &ua );
	list_sites( &ua );
	free_new( &ua );
	return 0;
} 
