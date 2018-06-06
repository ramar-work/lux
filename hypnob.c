/* hypnob.c */
#include "vendor/single.h"
#include <dirent.h>

#define PROG "hypnob"

#define VXPRINT(...) (opt_set(opts, "--verbose")) ? fprintf( stdout, __VA_ARGS__ ) : 0;

#define DB_SITELIST "hypno_sitelist"

#define PRINT_UA( args ) \
	nsprintf(args->dirname ); \
	nsprintf(args->sitename ); \
	nsprintf(args->domain ); \
	nsprintf(args->home ); \
	nsprintf(args->confdir ); \
	nsprintf(args->dbpath );

#ifdef INCLUDE_LINE_NO_ERROR_H
 #define ERRL(...) \
	( snprintf( err, 24, "[ @%s(), %d ]                 ", __FUNCTION__, __LINE__ ) ? 1 : 1 \
	&& ( err += 24 ) && snprintf( err, errlen - 24, __VA_ARGS__ ) ) ? 0 : 0
#else
 #define ERRL(...) snprintf( err, errlen, __VA_ARGS__ ) ? 0 : 0
#endif


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
	{ "assets", 'd', S_IRWXU },
	{ "models", 'd', S_IRWXU },
	{ "db", 'd', S_IRWXU },
	{ "files", 'd', S_IRWXU },
	{ "sql", 'd', S_IRWXU  },
	{ "views", 'd', S_IRWXU  },
	{ NULL }
};


static const char ListTmpl[] =
	"{{# list }}"
	"\t{{ .id }}: {{ .sitename }} - {{ .location }} - {{ .date_created }}\n "
	"{{/ list }}"
;

static const char ConfDir[] = ".hypno";

static const char DbName[] = "hypno.db";

static const char ListStmt[] =
"SELECT * FROM " DB_SITELIST
;

static const char ListStmtSpecific[] =
"SELECT * FROM " DB_SITELIST " WHERE id = 1"
;

static const char CreateStmt[] =
 "CREATE TABLE " DB_SITELIST " ("
 " id INTEGER PRIMARY KEY AUTOINCREMENT,"
 " unique_id TEXT," /*A hash for quick lookup*/
 " sitename TEXT,"
 " location TEXT," /*Absolute path for now*/ 
 " date_created TEXT,"
 " date_created_i INTEGER"
");"
;

static const char AddStmt[] =
 "INSERT INTO " DB_SITELIST " VALUES ( NULL, ?1, ?2, ?3, ?4, ?5 )"
;

static const char CheckStmt[] =
 "SELECT id FROM " DB_SITELIST " WHERE unique_id = ?1"
;

static const int errlen = 1024;
static char err[1024] = {0};

int init_hypno_local ( Option *opts, UserArgs *args, char *err )
{
	char *home = NULL; 
	char *file = NULL;
	char *dbname = NULL;
	struct stat sb;
	int status = 0;
	Database db; 

 	args->home = home = getenv( "HOME" ) ? getenv( "HOME" ) : "."; 
	args->confdir = strcmbd( "/", home, ConfDir );
	args->dbpath = strcmbd( "/", home, ConfDir, DbName );
	status = stat( args->confdir, &sb );

	if ( status > -1 ) 
		return 1;
	else if ( status == -1 && errno != ENOENT )
		return ERRL( "Can't access directory: %s - %s.", args->confdir, strerror(errno) ); 
	else if ( status == -1 ) {
		VXPRINT( "Creating directory: %s\n", args->confdir ) 

		if ( mkdir( args->confdir, S_IRWXU ) == -1 )
			return ERRL( "Error creating directory: %s\n", strerror( errno ) );

		VXPRINT( "Done.\n" );

		VXPRINT( "Seeding new database at: %s\n", args->confdir );

		//Create by opening and closing really quickly
		if ( !sq_open( &db, args->dbpath ) )
			return ERRL( "Could not create new database." );

		if ( !sq_lexec( &db, CreateStmt, "create", NULL ) )
			return ERRL( "Could not create new database." );

		if ( !sq_close( &db ) )
			return ERRL( "Could not create new database." );

		VXPRINT( "Done.\n" );
	}

	//Should never get here.
	return 0;
}


void free_new ( UserArgs *args )
{
	free ( args->confdir );
	free ( args->dbpath );
}


int create_cmd ( Option *opts, UserArgs *args, char *err )
{
	File *f = Filelist;
	Database db;
	memset(&db,0,sizeof(db));
	char *file = NULL;
	int   fd = 0;
	char *hash = 0;
	char  md[ 2048 ] = { 0 };

	if ( !args->dirname )
		return ERRL( "Argument --at either not specified or empty." );
	
	//Hash first 12 characters to make a unique id
	SQWrite SqlCheck[] = {
		{ SQ_TXT, .v.c = ( hash = (char *)shash_long( args->sitename, 4 ) ) }
	 ,{ .sentinel = 1 }
	};

	if ( !sq_open( &db, args->dbpath ) )
		return ERRL( "Could not open database: %s", args->dbpath );

	if ( !sq_lexec( &db, CheckStmt, "check", SqlCheck ) ) 
		return ERRL( "Failed to access database data at %s", args->dbpath );

	lt_dump( &db.kvt );
	//How do I check for zero results?
	if ( !sq_close( &db ) )
		return ERRL( "Could not close database handle for: %s", args->dbpath );

#if 0
	if ( sq_insert_oneshot( args->dbpath, CheckStmt, SqlCheck ) )
	{ //If the hash exists, die...
		return err( 0, "Site bla already exists..." );
	}
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
	file = strcmbd( "/", args->dirname, "data.lua" );
	fprintf( stderr, "Creating metadata file: %s\n", file );
	if ( (fd = open( file, O_CREAT | O_RDWR, S_IRWXU )) == -1 )
		return err( 0, "Error opening file: %s\n", strerror( errno ) );

	fprintf( stderr, "%s\n", md );
	write( fd, md, strlen( md )); 
	close( fd );
	free( file );


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

#if 0
	if ( !sq_insert_oneshot( args->dbpath, AddStmt, SqlImporter ) )
		return err( 0, "Failed to add database record for new site." );
#endif

	if ( !sq_open( &db, args->dbpath ) )
		return ERRL( "Could not open database: %s", args->dbpath );

	if ( !sq_lexec( &db, AddStmt, "import", SqlImporter) ) 
		return ERRL( "Failed to access database data at %s", args->dbpath );

	if ( !sq_close( &db ) )
		return ERRL( "Could not close database handle for: %s", args->dbpath );
	return 1;
}


int list_cmd ( Option *opts, UserArgs *args, char *err )
{
	Database db;
	Render r;
	Buffer *rr = NULL;
	int ref = 0;
	char *a = NULL;

	if ( !sq_open( &db, args->dbpath ) )
		return ERRL( "Failed to access hypno database." );
	
	if ( !sq_lexec( &db, ListStmt, "list", NULL ) ) 
		return ERRL( "Failed to access database data:..." );// %s.", db.error );

	//Stupid long workaround, get hash of list.0.id, if null then die
	ref = lt_get_long_i( &db.kvt, (uint8_t *)"list.0.id", strlen("list.0.id") );
	if ( !(a = lt_text_at( &db.kvt, ref )) ) {
		sq_close( &db );
		fprintf( stdout, "No sites found. (Try creating one with --create)." ); 
		return 1;
	}
	else { 
		//if there are no results, there is no reason to do this, however, it shouldn't crash...	
		render_init( &r, &db.kvt );
		render_map( &r, (uint8_t *)ListTmpl, strlen( ListTmpl ));
		render_render( &r ); 
		rr = render_rendered( &r );
		write( 1, bf_data( rr ), bf_written( rr ));
		sq_close( &db );
		render_free( &r );
	}

	if ( !sq_close( &db ) ) {
		return ERRL( "Failed to access hypno database." );
	}
	return 1;
}



Option opts[] =
{
	{ "-c", "--create",   "Create a new application directory here.", 's' },
	{ "-e", "--eat",      "Feed this a certain URL and see how it evaluates.",'s' },
	{ "-l", "--list",     "List all sites and their statuses." },

	{ "-a", "--at",       "Create a new application directory here.",'s' },
	{ "-n", "--name",     "Use this as a site name.", 's' },
	{ "-d", "--domain",   "Use this domain.", 's' },
	{ "-v", "--verbose",  "Be verbose." },
	{ "-h", "--help",     "Show help and quit." },
	{ .sentinel = 1 }
};

struct Cmd
{ 
	const char *cmd;
	int (*exec)( Option *, UserArgs *, char * );
} Cmds[] = {
	{ "--create"   , create_cmd  }
 ,{ "--list"     , list_cmd  }
 ,{ NULL         , NULL      }
};

int main (int argc, char *argv[])
{
	( argc < 2 ) ? opt_usage( opts, 0, "nothing to do.", 0 ) : opt_eval( opts, argc, argv );
	
	//Set basic arguments
	int status = 0;
	UserArgs ua;
	memset( &ua, 0, sizeof(UserArgs));
	ua.dirname = opt_get( opts, "--at" ).s;
	ua.sitename = ( !opt_set( opts, "--create" ) ) ? opt_get( opts, "--name" ).s : opt_get( opts, "--create" ).s; 
	ua.domain = opt_get( opts, "--domain" ).s;
	PRINT_UA( (&ua) );


	//Check --flag
	if ( opt_set( opts, "--create" ) ) {
		if ( !ua.sitename ) 
			return fprintf( stderr, PROG ": argument not specified for --create flag." ) ? 1 : 1;
		if ( ua.sitename[0] == '-' || ua.sitename[1] == '-' )
			return fprintf( stderr, PROG ": --create or --name does not have an argument." ) ? 1 : 1;
	}
	
	//Check if the HOME/.hypno directory exists, creating it if it does not.
	if ( !init_hypno_local( opts, &ua, err ) )
		return fprintf( stderr, PROG ": %s\n", err ) ? 1 : 1;

	//Evaluate all main stuff by looping through the above structure.
	struct Cmd *cmd = Cmds;	
	while ( cmd->cmd ) {
	#ifdef TESTOPTS_H
		fprintf( stderr, "Got option: %s? %s\n", cmd->cmd, opt_set(opts, cmd->cmd ) ? "YES" : "NO" );
	#endif
		if ( opt_set(opts, cmd->cmd ) && !cmd->exec( opts, &ua, err ) ) {
			fprintf( stderr, PROG ": %s\n", err );
			return 1;
		}
		cmd++;
	}

	free_new( &ua );
	return 0;
} 
