/* ------------------------------------- *
	nw.h
	----

 	The simple network loop.

	Summary
	-------
	nw is a lighteweight library using poll 
	to assist in quickly setting up servers
	capable of handling non blocking sockets.
	It is written in such a way that handler
	functions can be inserted like a game loop 	

	Usage
	-----
	Using nw is pretty simple as the backend
	to your server application is pretty 
	simple:

	1. open a socket 

	Defines
	-------
	NW_BEATDOWN_MODE - Enables some test friendly options, like stopping after
										 a certain number of requests
	NW_CATCH_SIGNAL  - Catch signals or not.  Will define free_selector by default.
	NW_VERBOSE       - Be verbose
	NW_FOLLOW        - Dump a message when moving through each part of the program.
	NW_SKIP_PROC     - Compile without a "processing" part of the server loop.
	NW_KEEP_ALIVE    - Define whether or not the connection should stay open after 
										 reaching the write portion of the connection's life cycle.
	NW_LOCAL_USERDAT - Define whether or not to use local userdata.
	NW_GLOBAL_USERDA - Define whether or not to use global userdata.
	NW_STREAM_TYPE   - Define which way to read requests...

	Customization
	-------------
	Here is an example of using a header file 
	to define settings. We'll call our file
	whatever.h:

  //Contents of "whatever.h"
 #define MIN_ACCEPTABLE_READ 64   	//Kick requests that fail to read this much
 #define NW_MAX_BUFFER_SIZ   8192 	//Read no more than this much to buffer
 #define NW_MAX_BUFFER_SIZ   8192 	//Write no more than this much to buffer
 #define MAX_OPEN_FILES      256  	//How many files can I have open at once?
 #define READ_TO_FILE        0    	//When reading, use a file and stream
 #define READ_TO_BUF         1    	//When reading, use a buffer

	Bugs
	----
	Please notify Antonio R. Collins II at
	ramar.collins@gmail.com.

	
 * ------------------------------------- */
#include "single.h"
#include <poll.h>

#define LITE_BUFFER

#ifndef NW_H
#define NW_H

/*nw has a few sensible defines*/
#define NW_MIN_ACCEPTABLE_READ      32
#define NW_MIN_ACCEPTABLE_WRITE     32
#define NW_MAX_ACCEPTABLE_READ    4096
#define NW_MAX_ACCEPTABLE_WRITE   4096
#define NW_RETRY_READ                3
#define NW_RETRY_WRITE               3
#define NW_MAX_EVENTS              100
#define NW_MAX_BUFFER_SIZE       64000 
#define NW_STREAM_TYPE             "?"
//#define NW_DISABLE_LOCAL_USERDATA    0 
//#define NW_DISABLE_GLOBAL_USERDATA   0 

/*nw's "static" defines, so I don't forget what goes where*/
enum { NW_RECV = 0, NW_SEND };

/*stream selections*/
typedef enum 
{ 
	NW_STREAM_BUF = 0, 
	NW_STREAM_FD,
	NW_STREAM_PIPE
} Stream;

#if 0 /*begin fork test*/
test -d tests || mkdir tests
for n in `seq 1 32`; do
	wget -o tests/index.${n}.html http://localhost:2000 &
done 
#endif /* end fork test */

#if 0 /*begin post test*/
curl --verbose --request POST \
	--form my_file=@tests/red52x35.jpg \
	--form paragraph="Flame flame flame" \
	--form author="Antonio R. Collins II" \
	http://localhost:2000 
#endif /* end post test*/

#ifdef NW_FOLLOW
 #define NW_CALL(c) \
	(c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#else
 #define NW_CALL(c) \
	c
#endif


/*Return error messages and a code at the same time.*/
#ifdef NW_VERBOSE
 #define nw_err(c, ...) \
	(fprintf(stderr, __VA_ARGS__) ? c : 0)
#else
 #define nw_err(c, ...) \
	c
#endif

/*Reset read event*/
#define nw_reset_read() \
	r->client->events = POLLRDNORM

/*Reset write event*/
#define nw_reset_write() \
	r->client->events = POLLWRNORM

/*Reset the receiver*/
#define nw_reset_recvr() \
	memset(r, 0, sizeof(Recvr)); close((&r->client)->fd); (&r->client)->fd = -1

/*Get fd without worrying about pollfd structure*/
#define nw_get_fd() \
	r->client->fd

/*Error map or a big struct?*/
enum {
	ERR_SO_HAPPY_THERE_ARE_NO_ERRORS = 0,

	/*Poll errors*/
	ERR_POLL_INITIAL_ALLOCATOR,
	ERR_POLL_TOO_MANY_FILES,
	ERR_POLL_RECVD_SIGNAL, 

	/*Socket child spawn*/	
	ERR_SPAWN_ACCEPT,
	ERR_SPAWN_NON_BLOCK_SET,
	ERR_SPAWN_MAX_CLIENTS,

	/*Accepted client select/poll loop*/
	ERR_READ_CONN_RESET,
	ERR_READ_EGAIN,
	ERR_READ_EBADF,
	ERR_READ_EFAULT,
	ERR_READ_EINVAL,
	ERR_READ_EINTR,
	ERR_READ_EISDIR,
	ERR_READ_CONN_CLOSED_BY_PEER,
	ERR_READ_BELOW_THRESHOLD,
	ERR_READ_MAX_READ_RETRY_REACHED,

	/*Processor errors go here*/
	ERR_WRITE_CONN_RESET,
	ERR_WRITE_EGAIN,
	ERR_OUT_OF_MEMORY,
	ERR_REQUEST_TOO_LARGE,
	ERR_WRITE_EBADF,
	ERR_WRITE_EFAULT,
	ERR_WRITE_EFBIG,
	ERR_WRITE_EDQUOT,
	ERR_WRITE_EINVAL,
	ERR_WRITE_EIO,
	ERR_WRITE_ENOSPC,
	ERR_WRITE_EINTR,
	ERR_WRITE_EPIPE,
	ERR_WRITE_EPERM,
	ERR_WRITE_EDESTADDREQ, /*For UDP*/
	ERR_WRITE_CONN_CLOSED_BY_PEER,
	ERR_WRITE_BELOW_THRESHOLD,
	ERR_WRITE_MAX_WRITE_RETRY_REACHED,

	/*Failure to allocate within the buffer*/
	/*ERR_BUFF_REALLOC_FAILURE,
	ERR_BUFF_OUT_OF_SPACE,*/
	ERR_TIMEOUT_CONN,
	ERR_END_OF_CHAIN,
};


typedef enum {
	NW_AT_READ = 0,   /*The current connection is read ready.*/
	NW_AT_PROC,       /*The current connection is processing a response*/
	NW_AT_WRITE,      /*The current conenction is write ready*/
	NW_COMPLETED,     /*The current connection is done*/
	NW_AT_ACCEPT,     /*The current connection is about to be created*/
	NW_ERR            /*The current connection ran into some sort of error*/
} Stage;



/*A buffer structure*/
#if 0
static const char *Buff_errors[] = {
	[ERR_BUFF_REALLOC_FAILURE] = "Buffer reallocation failure",
	[ERR_BUFF_OUT_OF_SPACE] = "Fixed buffer is out of space.",
};
#endif

#if 0
typedef struct Buffer Buffer;
struct Buffer {
	uint8_t *buffer;
	int size;
	int written;
	int fixed;
	int error;
};
#endif

/*Here is a similar structure*/
typedef struct 
{
  Socket            child;
	Stage             stage;
	int              rb, sb, 
              recvd, sent;  //Total bytes sent or received
  uint8_t          errbuf[2048];  //For error messages when everything may fail
#if 0
  int          request_fd;  
  int         response_fd;
#endif
#ifdef NW_BUFF_FIXED
  uint8_t        request_[NW_MAX_BUFFER_SIZE];
  uint8_t       response_[NW_MAX_BUFFER_SIZE];
#endif
  uint8_t        *request;
  uint8_t       *response;
	Buffer       _request;
	Buffer      _response;
	int      sretry, rretry;
	int          recv_retry;/*3*/
	int          send_retry;/*5*/
	int          *socket_fd;  //Pointer to parent socket?
	struct pollfd   *client;  //Pointer to currently being served client
#ifndef NW_DISABLE_LOCAL_USERDATA
	void          *userdata;
#endif
	//This allows nw to cut connections that have been on too long
	struct timespec start, end;
	int      status; 
#if 1
	int      connNo; 
	int     *bypass; 
#endif
} Recvr;


/*Structure to control event handlers*/
typedef struct { 
	_Bool (*exe)(Recvr *r, void *ud, char *err);
	enum {
		NW_CONTINUE = 0,
		NW_NOTHING,
		NW_RETURN,
		NW_EXIT
	}       action;
	int     status;
	char    err[2048]; 
} Executor;


/*Structure to control "stream" handlers*/
typedef struct {
	/*Should support writing to file or to memory*/ 
	//_Bool (*exe)(void *in, void *out);
	_Bool (*read) (Recvr *r);
	_Bool (*write)(Recvr *r);
} Streamer;


/*Structure for setting up the loop*/
typedef struct 
{
#if 0
	int       min[2]     ;  /*min read is first index 0, min write is second index 1*/
	int       max[2]     ;  /*max read is first index 0, max write is second index 1*/
	int       retry[2]   ;  /*read retry is first index, write is second*/
#else
	int       read_min   ;  /*How much data needs to be read at a time?*/
	int       write_min  ;  /*How much data needs to be written at a time?*/
	int8_t    recv_retry ;  /*Number of times to retry reading*/
	int8_t    send_retry ;  /*Number of times to retry sending*/
#endif
	Stream    stream     ;  /*Which stream to use*/
	Executor *errors,       /*Error handlers*/
           *runners    ;  /*Connection life cycle handlers*/
	int       run_limit  ;  /*A time limit to cut long running connections*/ 
	int       max_events ;  /*How many events should I allow to queue at a time?*/
	_Bool     keepalive  ;  /*Should the user be responsible for closing connections?*/

#ifndef NW_DISABLE_GLOBAL_USERDATA
	void     *global_ud  ;  /*Global userdata (not copied)*/
#endif

#ifndef NW_DISABLE_LOCAL_USERDATA
	int       lsize      ;  /*Size of one local userdata*/
	void    **local_ud   ;  /*Local userdata (this src doled out per max_events)*/
#endif

#ifdef NW_BEATDOWN_MODE
	int       stop_at    ;  /*Stop serving after x amount of requests*/
#endif

	//Stuff I'll never set
	struct    
   pollfd  *clients;
	int       tsize      ;  /*Total size of all local userdata (not touched)*/
	Socket   *parent     ;  /*The big daddy socket of whatever server you're running*/
	Recvr    *rarr       ;  /*Array to choose which "receiver" you want*/
} Selector;


extern Executor _nw_errors[ERR_END_OF_CHAIN + 1];
#if 0
Buffer *bf_init (Buffer *b, uint8_t *mem, int size);
void bf_setwsize (Buffer *b, int size); 
int bf_add (Buffer *b, uint8_t *s, int size); 
void bf_free (Buffer *b); 
const char *bf_err (Buffer *b);
#endif
_Bool initialize_selector (Selector *, Socket *);
void free_selector (Selector *s);
_Bool activate_selector (Selector *s); 
void print_selector (Selector *s);
_Bool executor (Recvr *r, void *ud, char *err); 
_Bool nw_close_fd (Recvr *r, void *ud, char *err);
_Bool reset_read_fd (Recvr *r, void *ud, char *err);
_Bool reset_write_fd (Recvr *r, void *ud, char *err);
_Bool reset_buffer (Recvr *r, void *ud, char *err);
_Bool nw_close_fd (Recvr *r, void *ud, char *err);
_Bool nw_finish_fd (Recvr *r, void *ud, char *err);
_Bool nw_reset_fd (Recvr *r, void *ud, char *err);
_Bool nw_add_read (Recvr *r, uint8_t *msg, int size);
_Bool nw_add_write (Recvr *r, uint8_t *msg, int size); 
#endif


#ifdef NW_DEBUG
void print_recvr (Recvr *r) ;
void print_selector (Selector *s) ;
#endif
