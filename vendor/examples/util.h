/**
* util.h 
*
* All of everything that you need for work to get done.
*
**/

/* #includes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <math.h>
#ifndef ADDTHREAD
// #include <pthread.h>
#endif
#ifndef ADDEVENT 
// #include <event.h>
#endif

/* Preprocessor hack */
#define PP_CATCH

/* Lua datatype help */
#define LUA_TYPES 9
#define LUA_MT_COUNT 16
#define PROGRAM "libkirk"
#define ps(x) u_mprint(x);
#define fs(x) fprintf(stderr, "%s\n", x);
#define fd(x) fprintf(stderr, "%d\n", x);
#define um(x) u_mprint_stack(state->state, x)
 
/* stupid debugging */
#ifdef DEBUG
#define DEBUG_CODE 1
#define USE_EXCEPTION 1
#else
#define DEBUG_CODE 0
#define USE_EXCEPTION 0
#endif

/* table management */
#define T_DEPTH 10 
#define T_DEPTH_GLOBAL 50 

/* option management */
#define NO_HASH_FOUND -1
#define MAX_HASH 1024
#define MAX_HASHABLE_STRING_LENGTH 1024

/* read in rate for transformations */
#define DEFAULT_READ 64

/* error message limit */
#define ERR_MESSAGE_LIMIT 1024
#define ERR_MESSAGE_WLENGTH 256

/* error macros to make life easier */
/* All of the error macros assume that the pm_t structure has been opened sometime */
#define ENTRY(x,y)  [x] = { .errc = x, .errr = #x, .errv = y },

#define EM(x)  err_##x##_map

/* Free callback macros */
#define FM(x, y) void libkirkfreewrap_##x (void *p) { x( (y *)p ); } 
#define FX(x) libkirkfreewrap_##x
#define u_add(x, y, z) u_vadd((void *)x, y, FX(z))
#define u_nadd(x, y) u_vadd((void *)x, y, NULL)


/* Throw errors using module error maps */
// #define u_err(x,...) u_cverr(&error, NULL, x, __VA_ARGS__)
#define u_err(...) u_cverr(state, 0, NULL, __VA_ARGS__)
#define u_log(...) u_cverr(state, 1, NULL, __VA_ARGS__)
#define u_fatalset(x) u_coreset(state, 1, 1, x)
#define u_wipe() lua_pop(state->state, lua_gettop(state->state)) 


/* Throw errors using Unix errno convention (might not need this) */
/* Consider combining the strings from the macro */
// #define u_syserr(x, ...) u_cverr(&error, "%Y", x, __VA_ARGS__)
		
	

/* Show the value of a long (used for datatype checking, move to u_assess.c) */
#define ASSESS(x) \
	fprintf(stdout, "#x: %ld\n", x); 


/* Timing information */
#define TTR_START() long start_t = u_utimec()
#define TTR_END() long end_t = u_utimec()
#define TTR_ELAPSED() long elapsed_t = (end_t - start_t)
#define TTR_SHOW elapsed_t
	
/* Define compiled module names */
#ifndef NO_CRYPT
#define KIRK_CRYPT CRYPT
#endif
#ifndef NO_DATETIME
#define KIRK_DATETIME DATETIME
#endif
#ifndef NO_DEBUG
#define KIRK_DEBUG DEBUG
#endif
#ifndef NO_DRAGON
#define KIRK_DRAGON DRAGON
#endif
#ifndef NO_ENCSER
#define KIRK_ENCSER ENCSER
#endif
#ifndef NO_ENCODE
#define KIRK_ENCODE ENCODE
#endif
#ifndef NO_FILES
#define KIRK_FILES FILES
#endif
#ifndef NO_HASH
#define KIRK_HASH HASH
#endif
#ifndef NO_MACRO
#define KIRK_MACRO MACRO
#endif
#ifndef NO_SIGNAL
#define KIRK_SIGNAL SIGNAL
#endif
#ifndef NO_SOCKET
#define KIRK_SOCKET SOCKET
#endif
#ifndef NO_SQLITE3
#define KIRK_SQLITE3 SQLITE3
#endif
#ifndef NO_TEST
#define KIRK_TEST TEST
#endif
#define KIRK_UTIL UTIL


/* Status */
typedef enum { 
	FALSE, 
	TRUE, 
	UNDEFINED 
} status_t; 



/* table management */
enum {
	T_ALPHA = 1, /* Expect only table keys that are strings */
	T_MIXED,     /* Expect both numeric and string table keys */
	T_NUMBER,    /* Expect only numeric table keys */
};

/* booleans */
typedef enum { 
	false, 
	true 
} bool_t; 


/* option metadata - also malloc'd -  */
typedef struct 
OV_t {
	char  *name;     /* Specify the option key or name expected */
	char  *strict;	 /* Define any strict adherance */
	char  *table;    /* Type that should be allowed in a table. */
	union {          /* Default value for the option. */
		int  n;	
		char *s;
	} dflt;       
	unsigned char  required;  /* Is this option required */
	unsigned char  found;     /* Is it a proper option?: */
	unsigned char  type;      /* The Lua datatype */
} om_t;



typedef struct 
FV_t {
	/* conditional requireds here */
	char 	  *discard;      /* discard Lua types */
	short   max_depth;     /* max table depth */
	unsigned int max_key_l;     /* the biggest a key should be for function */
	unsigned int err_type  :3;  /* errors here #defined by (int) */
	unsigned int index;         /* Where is the table expected to be? */
	
	/* set values here */
	unsigned int pop_table :1;  /* Pop the table or keep it? */
	unsigned int accept_mt :1;  /* accept metatables (true or false) */
	unsigned int exclusive :1;  /* Accept all keys (default?) */	
	unsigned int expects   :2;  /* expect table type #defined by (int) */

	/* Handle events and errors... */
	int   *start;
	int	*end;
	
	/* I'm not done with any of these yet... and wonder if I ever will be */
	void 	*on_error;     /* Lua error handlers? */
	void	*on_table;     /* a function to use when handling tables */
	void 	*on_discard;   /* Error handlers on encountering discarded */
	void 	*on_alpha;     /* Handler for alpha keys */
	void 	*on_numeric;   /* Handler for numeric keys */
	om_t  *options;      /* C89 Compatible? */
} fm_t;



/* All errors should be defined as either codes or within an array */
typedef union 
LV_t {
	const char *string;
	long number;
	int ud;
} luav_t; 

typedef struct
LLV_t {
	const char *key;
	int index;
	unsigned int type; // -1 means the end...
	//union {
		char *string;
		char *function;
		long number;
		unsigned int boolean :1;
	//} value;
} luab_t;


/* process metadata - manages errors, files, signals, memory, etc */
typedef struct 
PV_t {
	char *path;              /* Working directory */
	int processes;		 /* A process count */
	int memlimit;		 /* Memory limit */
	void **memrefs;		 /* Memory references (mallocs) */
	int  errhd;		 /* A class of error message */
	int  errfd;	         /* Choose a type of error handler */ 
	char *errpath;	         /* File path */
	FILE *stream;	         /* File stream */
	int   errc;              /* A return code for bad functions */
	char *errv;      	 /* A return error string for bad functions */
	char *fmt;       	 /* A format string for log style */
	struct p_metadata *pm_d; /* Default values */
} pm_t;


/**
* struct trval 
* 
* Structure for controlling the behavior of 
* u_ttransform(L, ...)
*
* A union might do a better job when choosing
* between stream types.
*
**/
typedef struct 
TV_t {
	int 	keep;         /* Keep the original values or not? */
	int 	ints;         /* Do no translations, just keep ints */
	int 	start;        /* Define a start byte */
	int 	stop;         /* Define a stop byte */
	int 	read_speed;   /* Define a read speed */
	int 	write_speed;  /* Define a write speed */
	int 	full;         /* Read the contents of whatever in before moving */
	int 	findex;       /* Where in the stack are the functions? */
	int 	tindex;       /* Where is the table index? */
	int 	autotr;       /* Where is the table index? */
	int 	table;        /* Define which of the following to use. */
	int	last_byte_r;  /* The failed byte (if an issue occurred) */
	FILE 	*stream;      /* A valid file pointer */ 
	char	*original;    /* Store the original untransformed items here */
	char	*string;      /* Define a string to read from */
	void 	*conditions;  /* Conditions to stop while loop in C. */
	void 	*readerf;     /* Alt handler to use when processing a bytestream */
	enum { 
		CALL_FGETC,      /* Use fgetc() to read file byte by byte */
		CALL_FGETWC,     /* Use fgetwc to read multibyte sequences */
		CALL_TABLE,      /* Use a Lua table when reading */
		CALL_STRCHR_A,   /* Convert using ASCII strings */ 
		CALL_STRCHR_U,   /* Convert using Unicode strings */
	} proc; 
	enum {
		WT_STR, 			  /* Write results to string */
		WT_TBL	        /* Write results to table */
	} retval;
} tm_t;


/* mix of chars */
char *mt_key_mix[] = {
	"__add",
	"__sub",
	"__mul",
	"__div",
	"__mod",
	"__pow",
	"__unm",
	"__concat",
	"__len",
	"__lt",
	"__gt",
	"__eq",
	"__le",
	"__index",
	"__newindex",
	"__call",
};

typedef struct 
EV_t {
	short errc;
	char  *errr; 
	char  *errv; 
} err_t; 


/* error or logging value types */
typedef struct
MV_t {
	int       rc;           /* 4,8 - Return code. */
	int       size;         /* 4,8 - The size of the error map */ 
	lua_State *state;       /* 8 - The Lua state (so you don't have to repeat it) */
	err_t     *map;         /* 8 - The current error map in use (if there is one) */
	char      *fname;       /* 8 - Function name */
	char      *msg;         /* 8 - A source string for messages */
	struct {
	  char    *file;        /* 8 - File, database or whatever connection */
	  char    *db;          /* 8 - Database or whatever connection */
	  char    *socket;      /* 8 - File, database or whatever connection */
	  int     type;         /* 8 - Type of file for logging or errors */
	} conn;
	char       omit_fname :1;   /* 8 - Function name */
	int       stacktrace :1;   /* 4, 8 - Control display of stack trace */
} msg_t;


/* Memory manager */
typedef struct
memV_t
{
	unsigned int set: 1;    // An indicator
	char *index;  // index (where?)
	void *ref;  // mem ref
	void (*callback)(void *);  // mem ref
} mft_t;


/* state management */
typedef struct 
stateV_t {
	lua_State *state;      // 8 - The state
	mft_t *memref[16];     // 8 - Keep track of all memory allocations
	int mem_ind[16];    // An index tracker...
	int errors;           // 8 - Any errors that have taken place.
	int processes;        // ?
	// struct {
	int       rc;           /* 4,8 - Return code. */
	int       size;         /* 4,8 - The size of the error map */ 
	err_t     *map;         /* 8 - The current error map in use (if there is one) */
	char      *fname;       /* 8 - Function name */
	char      *msg;         /* 8 - A source string for messages */
	// }
	struct {
	  char    *file;        /* 8 - File, database or whatever connection */
	  char    *db;          /* 8 - Database or whatever connection */
	  char    *socket;      /* 8 - File, database or whatever connection */
	  int     type;         /* 8 - Type of file for logging or errors */
	} conn;
	char omit_fname :1;   /* 8 - Function name */
	int flimsy :1;        // Make even the C code use exceptions (sigh...)
	int debug :1;         // Turn debugging messages on within functions. 
	int verbose :1;       // Turn debugging messages on within functions. 
	int error :1;         // could not find the value you wanted.
	int stacktrace :1;   /* 4, 8 - Control display of stack trace */
	unsigned int allocated :1;    /* 1, Flag to check the thing to free */ 
	unsigned int last;   /* only up to the memref limit, can be compilable */
	int memlim;           // 8 - set a memory limit.
	int tmp;             /* 4, 8 - Control display of stack trace */
} state_t;

/* This is for future reference. */
// state_t *state = NULL;
state_t new_state; 
state_t *state = &new_state;  // Turn garbage collection off for this ONLY.

/* Standard fallback crap */
enum { ERR_NONE, ERR_OOB };
	
err_t KIRK_STDD[] = {
	ENTRY(ERR_NONE, "No errors specified.")
	ENTRY(ERR_OOB,  "Unspecified error.")
};


/* utility related crap */
enum {
	ERR_LUA_ERRRUN = LUA_ERRRUN, 
	ERR_LUA_ERRMEM = LUA_ERRMEM, 
	ERR_LUA_ERRERR = LUA_ERRERR,
	ERR_EXPECTED_ALPHA,
	ERR_EXPECTED_NUMERIC,
	ERR_EXPECTED_INDEX,
	ERR_NOT_MEMBER_KEY,
	ERR_STRICT_TYPECHECK,
	ERR_REQD_KEY_UNSPEC,
	ERR_VALIDATION,
	ERR_TYPE_HANDLER,
	ERR_UNSET_KEY,
	ERR_STACK_ALLOC,
	ERR_GLOBAL_TYPECHECK,
	ERR_UNEXPECTED_TYPE,
	ERR_CONVERTING_TYPE,
	ERR_GOT_MULTIBYTE,
};
  
/* EXPECTED, RECEIVED, NOT, IS, */ 
err_t EM(util)[] = {
	/* always a success code */
	ENTRY(ERR_NONE, "No error encountered.") 

	/* util */
	ENTRY(ERR_LUA_ERRRUN,       "Error running Lua function.")
	ENTRY(ERR_LUA_ERRMEM,       "Error allocating memory in stack.")
	ENTRY(ERR_LUA_ERRERR,       "General error in Lua runtime.") 
	ENTRY(ERR_EXPECTED_ALPHA,   "Expected string at index %n")
	ENTRY(ERR_EXPECTED_NUMERIC, "Expected key of %t at table. Got %t.")
	ENTRY(ERR_EXPECTED_INDEX,   "Stack index of table is not specified. (fm_t->index)")
	ENTRY(ERR_NOT_MEMBER_KEY,   "Key %s is not a requested member.")
	ENTRY(ERR_REQD_KEY_UNSPEC,  "Required key %s not specified.")
	ENTRY(ERR_STRICT_TYPECHECK, "Key %s is not the correct type.  Must be %t")
	ENTRY(ERR_VALIDATION,       "Value at key %s fails validation check.")
	ENTRY(ERR_TYPE_HANDLER,     "Type handler function failed.")
	ENTRY(ERR_UNSET_KEY,        "Required key %s not set.")
	ENTRY(ERR_STACK_ALLOC,      "Could not allocate space on Lua stack.")
	ENTRY(ERR_GLOBAL_TYPECHECK, "Expected %t at key %s.")
	ENTRY(ERR_UNEXPECTED_TYPE,  "Unexpected %i at key %s.")
	ENTRY(ERR_CONVERTING_TYPE,  "Could not convert value to requested type.")
	ENTRY(ERR_GOT_MULTIBYTE,    "Expected single character sequence.")
};


#ifdef KIRK_CRYPT
	/* crypto */
#endif

#ifdef KIRK_DATETIME
	/* datetime */
#endif

#ifdef KIRK_DRAGON
	/* dragon */
#endif

#ifdef KIRK_ENCODE
#endif

#ifdef KIRK_FILES
enum {
	/* files */
	ERR_FILEPATH_NOT_STRING,
	ERR_FILENAME_TOO_LONG,
};

err_t EM(files)[] = {
	/* Files */
	ENTRY(ERR_FILEPATH_NOT_STRING, "Filepath specified not a string.")
	ENTRY(ERR_FILENAME_TOO_LONG,   "Filename %s specified is greater than current limit %n.")
};
#endif

#ifdef KIRK_MACRO
#endif

#ifdef KIRK_RANDOM
#endif

#ifdef KIRK_SEARCH
#endif

#ifdef KIRK_SEARCH
#endif

#ifdef KIRK_SETTHEORY
#endif

#ifdef KIRK_SQLITE3
	/* sqlite */
enum {
   ERR_SQLITE_CANTOPEN = SQLITE_CANTOPEN,
   ERR_SQLITE_CORRUPT = SQLITE_CORRUPT,
   ERR_SQLITE_NOTADB = SQLITE_NOTADB,
   ERR_SQLITE_PERM = SQLITE_PERM,
   ERR_SQLITE_AUTH = SQLITE_AUTH,
   ERR_SQLITE_CONSTRAINT = SQLITE_CONSTRAINT,
   ERR_SQLITE_ABORT = SQLITE_ABORT,
   ERR_SQLITE_BUSY = SQLITE_BUSY,
   ERR_SQLITE_DONE = SQLITE_DONE,
   ERR_SQLITE_FULL = SQLITE_FULL,
   ERR_SQLITE_IOERR = SQLITE_IOERR,
   ERR_SQLITE_LOCKED = SQLITE_LOCKED,
   ERR_SQLITE_INTERRUPT = SQLITE_INTERRUPT,
   ERR_SQLITE_MISMATCH = SQLITE_MISMATCH,
   ERR_SQLITE_MISUSE  = SQLITE_MISUSE,
   ERR_SQLITE_NOLFS = SQLITE_NOLFS,
   ERR_SQLITE_NOMEM = SQLITE_NOMEM,
   ERR_SQLITE_ROW = SQLITE_ROW,
   ERR_SQLITE_READONLY = SQLITE_READONLY,
   ERR_SQLITE_TOOBIG = SQLITE_TOOBIG,
   ERR_SQLITE_PROTOCOL = SQLITE_PROTOCOL,
   ERR_SQLITE_RANGE = SQLITE_RANGE,
   ERR_SQLITE_SCHEMA = SQLITE_SCHEMA,
   ERR_SQLITE_ERROR  = SQLITE_ERROR,
// ERR_SQLITE_WARNING = SQLITE_WARNING,
};


/* vararg macro */
err_t EM(sqlite)[] = {
	/* sqlite3 */
	ENTRY(ERR_SQLITE_CANTOPEN, "Can't open database file %s")
	ENTRY(ERR_SQLITE_CORRUPT,  "Database %s is corrupt")
	ENTRY(ERR_SQLITE_NOTADB,   "File %s is not a SQLite3 database.")
	ENTRY(ERR_SQLITE_PERM,     "No permission to create database %s.")
	ENTRY(ERR_SQLITE_AUTH,     "Authorization error at database %s.")
	ENTRY(ERR_SQLITE_CONSTRAINT, "Your constraint is miswritten or something.")
	ENTRY(ERR_SQLITE_ABORT,    "Refer to sqlite3.org")
	ENTRY(ERR_SQLITE_BUSY,     "Concurrency or rewrite error occured.")
	ENTRY(ERR_SQLITE_DONE,     "")
	ENTRY(ERR_SQLITE_FULL,     "Cannot write to database file %s, disk is full.") 
	ENTRY(ERR_SQLITE_IOERR,    "Similar to disk full, but with more complexity")
	ENTRY(ERR_SQLITE_LOCKED,   "Shared cache can't be accessed")
	ENTRY(ERR_SQLITE_INTERRUPT, "Suspend all processing.  Usually called from elsewhere.")
	ENTRY(ERR_SQLITE_MISMATCH, "Column datatypes mismatch enough to be serious.")
	ENTRY(ERR_SQLITE_MISUSE,   "Always should be an exception.  Not a user error.")
	ENTRY(ERR_SQLITE_NOLFS,    "No large file system support, reached system file size limit.")
	ENTRY(ERR_SQLITE_NOMEM,    "No more memory can be allocated.")
//	ENTRY(ERR_SQLITE_ROW,      "Standard return code when new rows are available.")
	ENTRY(ERR_SQLITE_READONLY, "Can't modify database %s. File is read-only.")
	/* Change SQLITE_MAX_LENGTH or use sqlite3_limit() to modify the limit. */
	ENTRY(ERR_SQLITE_TOOBIG,   "Can't write string or blob to database %s. Item is too big.")
//	ENTRY(ERR_SQLITE_PROTOCOL, "When enabling Write Ahead logging, code against this")
	ENTRY(ERR_SQLITE_RANGE,    "Binding statement is out of range.")
//	ENTRY(ERR_SQLITE_SCHEMA,   "If you change columns on the fly, then v3.7+ SQLITE_MAX_SCHEMA_RETRY will let you change this.")
	ENTRY(ERR_SQLITE_ERROR,    "General error." )
//	ENTRY(ERR_SQLITE_WARNING, "General warning." )
};
#endif

#ifdef KIRK_SOCKET
	/* socket rules */
enum {
	ERR_SOCKET_CLOSE,
	ERR_NO_ACCEPT,
	ERR_DATA_INCOMPLETE,
	ERR_LISTENER_TROUBLE,  /* Use err code from socket.h */
	ERR_BIND_TROUBLE,
};

err_t EM(socket)[] = {
	ENTRY(ERR_SOCKET_CLOSE, "Check protocol stack handler...")
	ENTRY(ERR_NO_ACCEPT, "Could not accept connection.")
	ENTRY(ERR_DATA_INCOMPLETE, "Data did not make it through the socket...")
	ENTRY(ERR_BIND_TROUBLE, "Could not bind to socket...")
	ENTRY(ERR_LISTENER_TROUBLE, "Could not listen to socket traffic...")
};
#endif

#ifdef KIRK_SIGNAL
#endif

#ifdef KIRK_TABLE
#endif

#ifdef KIRK_TEST
enum { ERR_MALFORMED_TEST };

err_t EM(test)[] = {
	/* test */
	ENTRY(ERR_MALFORMED_TEST, "Your test sucks...")
};
#endif



