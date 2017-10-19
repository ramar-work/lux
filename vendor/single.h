/*single.h*/
//Start with includes, not all modules need all headers
#ifndef _WIN32
 #define _POSIX_C_SOURCE 200809L
#endif 

#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#ifndef SINGLE_H
#define SINGLE_H

#ifndef SOCKET_H 
 #include <netinet/in.h>
 #include <fcntl.h>
 #include <sys/socket.h>
 #include <sys/un.h>
 #include <arpa/inet.h>
 #include <netdb.h>
#endif


#if 0
 //Test suite stuff
 #include "http-test.c"
#endif

/*Start with amalgamating all of the defines*/
#define FILES_H 
#define SINW_H

//Print string using name of variable as key
#define nsprintf(v) \
	fprintf(stderr, "%-30s: '%s'\n", #v, v)

//Print number using name of variable as key
#define niprintf(v) \
	fprintf(stderr, "%-30s:  %d\n", #v, v)

//Print long using name of variable as key
#define nlprintf(v) \
	fprintf(stderr, "%-30s: %ld\n", #v, v)

//Print float using name of variable as key
#define nfprintf(v) \
	fprintf(stderr, "%-30s: %f\n", #v, v)

//Print pointer using name of variable as key
#define npprintf(v) \
	fprintf(stderr, "%-30s: %p\n", #v, (void *)v)

//Print binary data (in hex) using name of variable as key
#define nbprintf(v, n) \
	fprintf (stderr,"%-30s: ", k); \
	for (int i=0; i < n; i++) fprintf( stderr, "%02x", v[i] ); \
	fprintf (stderr, "\n")

//Print strings
#define stprintf(k, v) \
	fprintf(stderr, "%-30s: '%s'\n", k, v)

//Print numbers
#define nmprintf(k, v) \
	fprintf(stderr, "%-30s:  %d\n", k, v)

//Print large numbers 
#define ldprintf(k, v) \
	fprintf(stderr, "%-30s: %ld\n", k, v)

//Print float / double 
#define fdprintf(k, v) \
	fprintf(stderr, "%-30s: %f\n", k, v)

//Print pointers 
#define spprintf(k, v) \
	fprintf(stderr, "%-30s: %p\n", k, (void *)v)

//Print binary data (in hex)
#define bdprintf(k, v, n) \
	fprintf (stderr,"%-30s: ", k); \
	for (int i=0; i < n; i++) fprintf( stderr, "%02x", v[i] ); \
	fprintf (stderr, "\n")

//Finally, print the size of things
#define szprintf(k) \
	fprintf(stderr, "Size of %-22s: %ld\n", #k, sizeof(k));

#ifndef ERR_H
 #if 1
  #define err(n, ...) (( fprintf(stderr, __VA_ARGS__) ? 0 : 0 ) || fprintf( stderr, "\n" ) ? n : n)
  #define berr(n, c) fprintf(stderr, "%s\n", __errors[ c ] ) ? n : n
  #define perr(n, ...) ( fprintf(stderr, "%s: ", PROGRAM_NAME ) ? 0 : 0 ) || (( fprintf(stderr, __VA_ARGS__ ) ? 0 : 0 ) || fprintf( stderr, "\n" ) ? n : n )
 #else
  #define err(n, ...) 0
  #define berr(n, ...) 0
  #define perr(n, c) fprintf(stderr, PROGRAM_NAME, fprintf(stderr, "%s\n", __errors[ c ] ) ? n : n
 #endif
#endif

#ifndef DEBUG_H
 #if 1
	#define SUSP(...) \
		fprintf( stderr, "%s, %d: ", __FILE__, __LINE__ ); fprintf( stderr, __VA_ARGS__ ); getchar() 

  #define STEP(...) do { \
   fprintf( stderr, "%20s [ %s: %d ]", __func__, __FILE__, __LINE__ ); \
   getchar(); } while (0)
 #else
  #define STEP(...) do { \
   fprintf( stderr, __VA_ARGS__ ); getchar(); } while (0)
 #endif

 //A define to help dump data
 #define SHOWDATA(...) do { \
  fprintf(stderr, "%-30s [ %s %d ] -> ", __func__, __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ ); \
  fprintf( stderr, "\n"); } while (0)

 //Dump binary data
 #define SHOWBDATA(a,b, ...) do { \
  fprintf(stderr, "%-30s [ %s %d ] -> ", __func__, __FILE__, __LINE__); \
  fprintf( stderr, __VA_ARGS__ ); \
	write( 2, a, b ); \
  fprintf( stderr, "\n"); } while (0)


 //Encapsulate for testing
 #define ENCAPS( d, len ) \
  write( 2, "'", 1 ); \
  write( 2, d, len ); \
  write( 2, "'", 1 ); \
  write( 2, "\n", 1 )
#else
	#define SUSP(...)
  #define STEP(...) 
  #define SHOWDATA(...)
	#define SHOWBDATA(a,b, ...) 
	#define ENCAPS( d, len )
#endif


#ifndef UTIL_H
 #define READLENGTH 64
 #define hash_str( dest, src, len ) \
  hash_long( dest, (unsigned char *)src, strlen((char *)src), len )
#endif

#ifndef PARSELY_H
 #define PARSER_MAXMATCH_LEN 127
 #define PARSER_MOD 31 
 #define PARSER_MATCH_CATCH 1 << 1
 #define PARSER_NEGATE_MATCH 1 << 0
#endif

#ifndef TAB_H
 #define LT_POLYMORPH_BUFLEN 2048
 #define LT_MAX_HASH 7 
 #define lt_blob_at( t, i ) \
 	lt_ret( t, LITE_BLB, i )->vblob
 #define lt_blobdata_at( t, i ) \
 	lt_ret( t, LITE_BLB, i )->vblob.blob
 #define lt_blobsize_at( t, i ) \
 	lt_ret( t, LITE_BLB, i )->vblob.size
 #define lt_int_at( t, i ) \
 	lt_ret( t, LITE_INT, i )->vint
 #define lt_float_at( t, i ) \
 	lt_ret( t, LITE_FLT, i )->vfloat
 #define lt_text_at( t, i ) \
 	lt_ret( t, LITE_TXT, i )->vchar
 #define lt_userdata_at( t, i ) \
 	lt_ret( t, LITE_USR, i )->vusrdata
 #define lt_table_at( t, i ) \
 	lt_ret( t, LITE_TBL, i )->vtable
 #define lt_blob( t, key ) \
 	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vblob
 #define lt_blobdata( t, key ) \
 	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vblob.blob
 #define lt_blobsize( t, key ) \
 	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vblob.size
 #define lt_int( t, key ) \
 	lt_ret( t, LITE_INT, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vint
 #define lt_float( t, key ) \
 	lt_ret( t, LITE_FLT, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vfloat
 #define lt_text( t, key ) \
 	lt_ret( t, LITE_TXT, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vchar
 #define lt_userdata( t, key ) \
 	lt_ret( t, LITE_USR, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vusrdata
 #define lt_table( t, key ) \
 	lt_ret( t, LITE_TBL, lt_get_long_i(t, (uint8_t *)key, strlen(key)) )->vtable
 #define lt_lblob( t, key, len ) \
 	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, len) )->vblob
 #define lt_lblobdata( t, key, len ) \
 	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, len) )->vblob.blob
 #define lt_lblobsize( t, key, len ) \
 	lt_ret( t, LITE_BLB, lt_get_long_i(t, (uint8_t *)key, len) )->vblob.size
 #define lt_lint( t, key, len ) \
 	lt_ret( t, LITE_INT, lt_get_long_i(t, (uint8_t *)key, len) )->vint
 #define lt_lfloat( t, key, len ) \
 	lt_ret( t, LITE_FLT, lt_get_long_i(t, (uint8_t *)key, len) )->vfloat
 #define lt_ltext( t, key, len ) \
 	lt_ret( t, LITE_TXT, lt_get_long_i(t, (uint8_t *)key, len) )->vchar
 #define lt_luserdata( t, key, len ) \
 	lt_ret( t, LITE_USR, lt_get_long_i(t, (uint8_t *)key, len) )->vusrdata
 #define lt_ltable( t, key, len ) \
 	lt_ret( t, LITE_TBL, lt_get_long_i(t, (uint8_t *)key, len) )->vtable
 #define lt_ascend( t ) \
 	lt_move( t, 1 )
 #define lt_descend( t ) \
 	lt_move( t, 0 )
 #define lt_geti( t, key ) \
 	lt_get_long_i( t, (unsigned char *)key, strlen((char *)key))
 #define lt_keytype( t ) \
 	lt_rettype( t, 0, (t)->index )
 #define lt_valuetype( t ) \
 	lt_rettype( t, 1, (t)->index )
 #define lt_keytypename( t ) \
 	lt_rettypename( t, 0, (t)->index )
 #define lt_valuetypename( t ) \
 	lt_rettypename( t, 1, (t)->index )
 #define lt_keytypeat( t, i ) \
 	lt_rettype( t, 0, i )
 #define lt_valuetypeat( t, i ) \
 	lt_rettype( t, 1, i )
 #define lt_keytypenameat( t, i ) \
 	lt_rettypename( t, 0, i )
 #define lt_valuetypenameat( t, i ) \
 	lt_rettypename( t, 1, i )
 #define lt_kt( t ) \
 	lt_rettype( t, 0, (t)->index )
 #define lt_vt( t ) \
 	lt_rettype( t, 1, (t)->index )
 #define lt_ktn( t ) \
 	lt_rettypename( t, 0, (t)->index )
 #define lt_vtn( t ) \
 	lt_rettypename( t, 1, (t)->index )
 #define lt_kta( t, i ) \
 	lt_rettype( t, 0, i )
 #define lt_vta( t, i ) \
 	lt_rettype( t, 1, i )
 #define lt_ktna( t, i ) \
 	lt_rettypename( t, 0, i )
 #define lt_vtna( t, i ) \
 	lt_rettypename( t, 1, i )
 #define lt_addintkey(t, v) \
 	lt_add(t, 0, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
 #define lt_addintvalue(t, v) \
 	lt_add(t, 1, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
 #define lt_addtextkey(t, v) \
 	lt_add(t, 0, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
 #define lt_addtextvalue(t, v) \
 	lt_add(t, 1, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
 #define lt_addblobdkey(t, v, vlen) \
 	lt_add(t, 0, LITE_TXT, 0, 0, 0, (uint8_t *)v, vlen, 0, 0, NULL)
 #define lt_addblobdvalue(t, v, vlen) \
 	lt_add(t, 1, LITE_TXT, 0, 0, 0, (uint8_t *)v, vlen, 0, 0, NULL)
 #define lt_addblobkey(t, vblob, vlen) \
 	lt_add(t, 0, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
 #define lt_addblobvalue(t, vblob, vlen) \
 	lt_add(t, 1, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
 #define lt_addfloatvalue(t, v) \
 	lt_add(t, 1, LITE_FLT, 0, v, 0, 0, 0, 0, 0, NULL)
#ifdef LITE_NUL
 #define lt_addnullvalue(t) \
 	lt_add(t, 1, LITE_NUL, 0, 0, 0, 0, 0, 0, 0, NULL)
#endif
 #define lt_addudvalue(t, v) \
 	lt_add(t, 1, LITE_USR, 0, 0, 0, 0, 0, v, 0, NULL)
 #define lt_addik(t, v) \
 	lt_add(t, 0, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
 #define lt_addiv(t, v) \
 	lt_add(t, 1, LITE_INT, v, 0, 0, 0, 0, 0, 0, NULL)
 #define lt_addtk(t, v) \
 	lt_add(t, 0, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
 	//lt_add(t, 0, LITE_TXT, 0, 0, v, 0, 0, 0, 0, NULL)
 #define lt_addtv(t, v) \
 	lt_add(t, 1, LITE_TXT, 0, 0, 0, (uint8_t *)v, strlen(v), 0, 0, NULL)
 	//lt_add(t, 1, LITE_TXT, 0, 0, v, 0, 0, 0, 0, NULL)
 #define lt_addbk(t, vblob, vlen) \
 	lt_add(t, 0, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
 #define lt_addbv(t, vblob, vlen) \
 	lt_add(t, 1, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, NULL)
 #define lt_addfv(t, v) \
 	lt_add(t, 1, LITE_FLT, 0, v, 0, 0, 0, 0, 0, NULL)
 #define lt_addnv(t) \
 	lt_add(t, 1, LITE_NUL, 0, 0, 0, 0, 0, 0, 0, NULL)
 #define lt_adduv(t, v) \
 	lt_add(t, 1, LITE_USR, 0, 0, 0, 0, 0, v, 0, NULL)
 #define lt_addtbk(t, str, vblob, vlen) \
 	lt_add(t, 0, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, str)
 #define lt_addtbv(t, str, vblob, vlen) \
 	lt_add(t, 1, LITE_BLB, 0, 0, 0, vblob, vlen, 0, 0, str)
 #define lt_items(t, str) \
 	lt_items_i(t, (uint8_t*)str, strlen((char *)str))
 #define lt_iitems(t, ind) \
 	lt_items_by_index(t, ind )
 #define lt_countall( t ) \
 	lt_counti( t, 0 );
 #define lt_within( t, str ) \
 	lt_within_long( t, (uint8_t *)str, strlen(str))
#endif

#ifndef RENDER_H
 #ifndef RENDER_MAX_BUF_SIZE
  #define RENDER_MAX_BUF_SIZE 2048
 #endif

 #ifdef RENDER_VERBOSE
	#define print_pause(...) \
		print_uint8t (__VA_ARGS__); \
		getchar()
	#define print_lk(...) \
		print_uint8t (__VA_ARGS__)
	#define print_err(...) \
		(snprintf( r->errbuf, 4095, __VA_ARGS__ ) && print_uerr ( r->errbuf )) ? 1 : 1
 #else
	#define print_pause(...)
	#define print_lk(...)
	#define print_err(...)
 #endif

 #define MARK( m, n ) \
	b->markers[ m ] = n

 #define SET( m ) \
	b->markers[ m ]

 #define REALLOC(src, dest) \
	raw = realloc( r->markers, ++follow * sizeof(Mark)); \
	if ( !raw ) \
		return 0; \
	else  \
	{ \
		r->markers = raw; \
		memset( &r->markers[ (follow - 1)], 0, sizeof(Mark) ); \
		ct = &r->markers[ (follow - 1) ]; \
	}
#endif

#ifndef SQROOGE_H
 #ifndef SQLITE3_PATH
  #include "sqlite3.h"
 #else
  #include SQLITE3_PATH
 #endif
 
 /*This is because C sucks... :) (sometimes) */
 #define init_buf(dest, src, len) \
	char src[len + 1]; memset(src, 0, len + 1); memcpy(src, dest, len); src[len] = '\0' 

 #define sq_free( db ) sq_close( db )
 #if 0
  //Macros that may (or may not be) easier than duplicating so much code
  #define sq_create_oneshot(fn, sql) __sq_run(fn, sql, 0, __sq_create__)
  #define sq_insert_oneshot(fn, sql) __sq_run(fn, sql, 1, __sq_insert__)
  #define sq_read_oneshot(fn, sql) __sq_run(fn, sql, 1, __sq_read__)
 #endif
#endif


#ifndef TIMER_H
 #include <time.h>
#endif

#ifndef MEM_H
 #define strwalk(a,b,c) \
	memwalk(a, (uint8_t *)b, (uint8_t *)c, strlen(b), strlen((char *)c))

 #define meminit(mems, p, m) \
	Mem mems; \
	memset(&mems, 0, sizeof(Mem)); \
	mems.pos = p; \
	mems.it = m; 


#endif

#ifndef JSON_H
 #define JSON_MAX_DEPTH 100
 #ifdef DEBUG_H 
  #define qprintf( ... ) \
   fprintf( stderr, __VA_ARGS__ );
  #define dump( ... )
 #else	
  #define qprintf( ... )
  #define dump(...) 
 #endif
#endif

#ifndef RAND_H
 #define RAND_BUF_SIZE 64 
#endif

#ifndef SOCKET_H 
 #define DEFAULT_HOST "localhost"

 #define set_sockopts(type,dom,prot) \
	sock->conntype = type, \
	sock->domain = dom, \
	sock->protocol = prot, \
	sock->hostname = (sock->server) ? ((!sock->hostname) ? DEFAULT_HOST : sock->hostname) : NULL, \
	sock->_class = (sock->server) ? 's' : 'c'

 #define domain_type(domain) \
	(domain == AF_INET) ? "ipv4" : "ipv6"

 #define type_type(type) \
		(type == SOCK_STREAM) ? "SOCK_STREAM" : \
		(type == SOCK_DGRAM) ? "SOCK_DGRAM" : \
		(type == SOCK_SEQPACKET) ? "SOCK_SEQPACKET" : \
		(type == SOCK_RAW) ? "SOCK_RAW" : \
		(type == SOCK_RDM) ? "SOCK_RDM" : "UNKNOWN SOCKET TYPE"

 #define proto_type(proto) \
	struct protoent *__mp; \
	char *tb = !(__mp=getprotobynumber(proto)) ? "unknown" : __mp->p_name; \
	endprotoent();

 #define class_type(sockclass) \
	(sockclass == 'c') ? "client" : \
	(sockclass == 'd') ? "server (child)" : "server (parent)"

 #define buffer_type(sockclass)
#endif

#ifndef SINW_H
 #define NW_MIN_ACCEPTABLE_READ      32
 #define NW_MIN_ACCEPTABLE_WRITE     32
 #define NW_MAX_ACCEPTABLE_READ    4096
 #define NW_MAX_ACCEPTABLE_WRITE   4096
 #define NW_RETRY_READ                3
 #define NW_RETRY_WRITE               3
 #define NW_MAX_EVENTS              100
 #define NW_MAX_BUFFER_SIZE       64000 
 #define NW_STREAM_TYPE             "?"

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

 /*Call logging function*/
 #ifdef NW_VERBOSE 
  #define nw_log(...) \
 	fprintf(stderr, __VA_ARGS__);
  #define nw_error_log(map, code) \
 	write(2, map[code], strlen(map[code]))
 #else
  #define nw_log(...)
  #define nw_error_log(map, code)
 #endif
 
 //Deduce the stage of the request
 #define GETSTAGE(i) \
 	(i == NW_AT_WRITE) ? "write" : (i == NW_AT_READ ) ? "read" : (i == NW_AT_PROC) ? "proc" : "completed"
 
 /*Handle errors via the nw_error_map function pointer table.*/
 #define handle(ERRCODE) { \
 	nw_error_log(nw_error_map, ERRCODE); \
 	if (!(&s->errors[ERRCODE])->exe(r, s->global_ud, (&s->errors[ERRCODE])->err)) { \
 		switch ((&s->errors[ERRCODE])->action) { \
 			case NW_CONTINUE: \
 				continue; \
 			case NW_NOTHING: \
 				0; break; \
 			case NW_RETURN: \
 				return (&s->errors[ERRCODE])->status || 0; \
 			case NW_EXIT: \
 				exit((&s->errors[ERRCODE])->status || 0); \
 		} \
 	} \
 }
 
 /*Handle errors via the rwp_error_map function pointer table.*/
 #define uhandle(CODE) \
 	if (NW_CALL( ( r->status = (&s->runners[CODE])->exe(r, s->global_ud, (&s->runners[CODE])->err) ) )) { \
 		/*Success*/ \
 		nw_log("%s successful at %s %d.\n", #CODE, __FILE__, __LINE__); \
 		switch ((&s->runners[CODE])->action) { \
 			case NW_CONTINUE: \
 				continue; \
 			case NW_RETURN: \
 				return (&s->runners[CODE])->status || 0; \
 			case NW_EXIT: \
 				exit((&s->runners[CODE])->status || 0); \
 			case NW_NOTHING: \
 				0; \
 		} \
 	} \
 	else { \
 		nw_log("%s failed at %s %d.\n", #CODE, __FILE__, __LINE__); \
 		switch ((&s->runners[CODE])->action) { \
 			case NW_CONTINUE: \
 				continue; \
 			case NW_RETURN: \
 				return (&s->runners[CODE])->status || 0; \
 			case NW_EXIT: \
 				exit((&s->runners[CODE])->status || 0); \
 			case NW_NOTHING: \
 				0; \
 		} \
 	}
 
 /*Print a message as we move through branches within the program flow*/
 #ifdef NW_VERBOSE
  #define NW_LOG(c) \
 	(c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
 #else
  #define NW_CALL(c) \
 	c
 #endif
#endif


/*Then continue with amalgamating all of the errors.*/
enum 
{
  ERR_NONE,
#ifndef BUFF_H
  /*Buffer*/
  ERR_BUFF_ALLOC_FAILURE,
  ERR_BUFF_REALLOC_FAILURE,
  ERR_BUFF_OUT_OF_SPACE,
#endif
#ifndef TAB_H 
	ERR_LT_ALLOCATE,
	ERR_LT_OUT_OF_SPACE,
	ERR_LT_INVALID_VALUE,
	ERR_LT_INVALID_TYPE,
	ERR_LT_INVALID_INDEX,
	ERR_LT_OUT_OF_SLICE,
	ERR_LT_INDEX_MAX,
#endif
#ifndef SQROOGE_H 
	ERR_DB_OPEN,
	ERR_DB_CLOSE,
	ERR_DB_PREPARE_STMT,
	ERR_DB_BIND_VALUE,
	ERR_DB_STEP,
	ERR_DB_BIND_LONG,
#endif
#ifndef MEM_H
#endif
#ifndef JSON_H
#endif
#ifndef TIMER_H
	ERR_LITE_TIMER_ERROR,
#endif
#ifndef OPTION_H 
	ERR_LITE_OPT_EXPECTED_ANY,
	ERR_LITE_OPT_EXPECTED_STRING,
	ERR_LITE_OPT_EXPECTED_NUMBER,
#endif
#ifndef SOCKET_H 
	ERR_LITE_SOCKET_EXPECTED_ANY,
	ERR_LITE_SOCKET_EXPECTED_STRING,
	ERR_LITE_SOCKET_EXPECTED_NUMBER,
#endif
#ifndef SINW_H
	ERR_POLL_INITIAL_ALLOCATOR,
	ERR_POLL_TOO_MANY_FILES,
	ERR_POLL_RECVD_SIGNAL, 
	ERR_SPAWN_ACCEPT,
	ERR_SPAWN_NON_BLOCK_SET,
	ERR_SPAWN_MAX_CLIENTS,
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
	/*ERR_BUFF_REALLOC_FAILURE,
	ERR_BUFF_OUT_OF_SPACE,*/
	ERR_TIMEOUT_CONN,
	ERR_END_OF_CHAIN,
#endif
#ifndef UTIL_H
	ERR_FILE_NOT_FOUND,
	ERR_FILE_READ_ERROR,
	ERR_FILE_CLOSE,
#endif
	ERR_ERR_INDEX_MAX,
};




#ifndef MIME_H
typedef struct 
{ 
  const char *filetype, 
	     *mimetype; 
} Mime; 
#endif

#ifndef BUFF_H
typedef struct 
{
  uint8_t *buffer;
  int size;
  int written;
  int fixed;
  int error;
} Buffer;
#endif

#ifndef PARSELY_H
typedef struct 
{
  unsigned int   marker ;    //Keep track of bit flags
  unsigned char  founds
                 [255]  ;    //Each char goes here when building
  unsigned char  matchbuf    //buffer for matched tokens
                 [PARSER_MAXMATCH_LEN];     
  unsigned int   find;       //?
  unsigned int   end;        //Did we reach the end?
  unsigned char *match;      //ptr to matched token 
  unsigned char *word;       //ptr to found word
  unsigned int   tokenSize,  //size of found token
  	   size,       //size of block between last match (or start) and current match
  	   prev,       //previous match
  	   next;       //next match
  struct words { 
       char *word,       //Find this
             *catch,      //If you found *word, skip all other characters until you find this 
                *negate;     //If you found *word, skip *catch until you find this
  }              words[31];
} Parser;

#endif

#ifndef TAB_H
//Define a type polymorph with a bunch of things to help type inference
typedef struct LiteValue LiteValue;
typedef struct LiteBlob LiteBlob;
typedef struct LiteTable LiteTable;
typedef struct LiteKv LiteKv;
typedef union  LiteRecord LiteRecord;
//typedef struct LiteNode LiteNode;
//Table for table values
typedef enum 
{
  LITE_NON = 0, //Uninitialized values
  LITE_INT,     //Integer
  LITE_FLT,     //FLoat
  LITE_TXT,     //Text
  LITE_BLB,     //Blobs (strings that don't terminate come back as blobs too)
  LITE_NUL,     //Null
  LITE_USR,     //Userdata
  LITE_TBL,     //A "table"
  LITE_TRM,     //Table terminator (NULL alone can't be described)
  LITE_NOD,     //A node
} LiteType;

struct LiteTable 
{
  uint32_t  count;
  long      ptr;
  LiteTable *parent;
};

//
typedef struct 
{
  unsigned int  total  ,   //Size allocated (the bound)
                modulo ,   //Optimal modulus value for hashing
                index  ,   //Index to current element
                count  ,   //Elements in table
                *rCount;   //Elements in current table 
  int           error  ;   //An error occurred, read it...
  int           mallocd;   //An error occurred, read it...
  int           srcmallocd;   //An error occurred, read it...
  int           size   ;   //Size of newly trimmed key or pointer
  int           cptr;    //Table will stop here
  int           start  ,   //Table bounds are here if "lt_within" is used
                end    ,
                buflen ;
  unsigned char *src   ;   //Source for when you need it
  unsigned char *buf   ;   //Pointer for trimmed keys and values
  LiteKv        *head  ;   //Pointer to the first element
  LiteTable     *current;   //Pointer to the first element
  
} Table;


struct LiteBlob 
{
  int size;
  uint8_t *blob;
};


union LiteRecord 
{
  int         vint;
  float       vfloat;
  char       *vchar;
#ifdef LITE_NUL
  void       *vnull;
#endif
  void       *vusrdata;
  LiteBlob    vblob;
  LiteTable   vtable;
  long        vptr;
#if 0
  LiteNode    vnode;
#endif
};


struct LiteValue 
{
  LiteType    type;
  LiteRecord  v;
};


struct LiteKv 
{
  int hash[LT_MAX_HASH];
  LiteKv *parent;  
  LiteValue key; 
  LiteValue value;
};
#endif


#ifndef OPT_H
typedef struct Value Value;
struct Value {
	#if 0
	union { int32_t n; char *s; char c; void *v; } value;
	#endif
	int32_t n; char *s; char c; void *v; 
};

typedef struct Option Option;
struct Option
{
	const char  *sht,
              *lng,
	            *description;
	char  type; /*n, s or c*/	
	Value v;
	_Bool (*callback)(char **av, Value *v, char *err);
	_Bool set;  /*If set is not bool, it can define the order of demos*/
	_Bool sentinel;
};
#endif


#ifndef RENDER_H
enum 
{ /*...*/
	RAW = 0,
	NEGLOOP,
	POSLOOP,
	ENDLOOP,
	STUB,
	DIRECT
};

typedef struct
{ 
  uint8_t *blob, *parent;
  int      size, action, psize, type, index;
} Mark;  

typedef struct
{
  Table   *srctable;
  uint8_t *src;
  int      srclen;
  int      destlen;
  int      depth;
  int      maxbuf;
  int      loop;
  int      written;
  Mark    *markers;	
  Table   *renderers;  //This thing should handle different rendering functs 
  Buffer   dest;
  
  Parser  *p;
  char     buf[RENDER_MAX_BUF_SIZE];
 #ifdef RENDER_VERBOSE
  char     errbuf[ 4096 ];
 #endif
} Render;
#endif

#ifndef SQROOGE_H
enum {
	SQL_WRITE,
	SQL_NOOP,
	SQL_JUICY,
};


#if 0
static const char *sq_names[] = {
	"uninitialized",
	[SQ_INT] = "int",
	[SQ_FLT] = "float",
	[SQ_TXT] = "text",
	[SQ_BLB] = "blob",
	[SQ_NUL] = "null",
	[SQ_DTE] = "date",
};

static const char *sqltypes[] = {
	[SQLITE_INTEGER] = "integer",
	[SQLITE_FLOAT]   = "float",
	[SQLITE_TEXT]    = "text",
	[SQLITE_BLOB]    = "blob",
	[SQLITE_NULL]    = "null",
	[SQ_DTE]         = "date",
};

#endif
typedef struct 
{ 
  char *s; /*A set of results go here, this is a big hash table really*/
  int p;
} SQRecord ;


typedef union 
{
  char    *c;
  uint8_t *d; 
  int      n;
  float    f;
} SQUnion;


typedef enum 
{
  SQ_INT = SQLITE_INTEGER, /*Commits an integer*/
  SQ_FLT = SQLITE_FLOAT  , /*Commits a float*/
  SQ_TXT = SQLITE_TEXT   , /*Commits text*/
  SQ_BLB = SQLITE_BLOB   , /*Commits a blob*/
  SQ_NUL = SQLITE_NULL   , /*Commits a null value*/
  SQ_DTE                   /*Commits a date in a chosen format*/
} SQType;


typedef struct
{
  SQType  type    ;  
  SQUnion v       ;
  int     len     ;

  /*This shouldn't be it...*/  
  char    *c;
  uint8_t *d; 
  int      n;
  float    f;
  _Bool   sentinel; 
} SQWrite ;

typedef struct 
{ 
	int (*fp)(sqlite3_stmt *, int, uint8_t *); 
} SQReader ;

typedef struct 
{
	_Bool (*fp)(sqlite3_stmt *stmt, int i, const SQWrite *w);
} SQInsert ;

typedef struct 
{
  const char   *filename;
  const char   *sql;
  const char   *table;
  char   *qname;  //Name of query
  sqlite3      *db;
  sqlite3_stmt *stmt;
  int error;
  /*Try to start using FLAGS*/
  _Bool         read_started;
  Buffer        header;  
  Buffer        results;  
  Table         kvt;
} Database;

#endif

#ifndef RAND_H
typedef struct 
{
	struct timespec rand_ts;
	char   buf[RAND_BUF_SIZE];
} Random;
#endif


#ifndef TIMER_H 
//Different time types
typedef enum 
{
	LITE_NSEC = 0,
	LITE_USEC,
	LITE_MSEC,
	LITE_SEC,
} LiteTimetype;


typedef struct 
{	
	clockid_t   clockid;     
	int         linestart,   
              lineend;	  
	struct 
  timespec    start,     
              end;      
	const char *label; 
 #ifdef CV_VERBOSE_TIMER 
  const char *file;        
 #endif
	LiteTimetype  type;     
} Timer;
#endif

#ifndef MEM_H
/*Custom memory datatype*/
typedef struct 
{
	int     pos,  //Position
         next,  //Next position
         size,  //Size of something
	         it;
	uint8_t chr;  //Character found
} Mem ;
#endif


#ifndef SOCKET_H
typedef struct
{
	_Bool server    ;
	char *hostname  ;
	char *proto     ;
	char *service   ;    //A string representation of service 
	char  opened    ;
	char  _class    ;    //Type of socket
	char  portstr[6];
	char  address
[INET6_ADDRSTRLEN];    //Address gets stored here

	int port       ;
	int fd         ;
	int clifd      ;
	int domain     ;     //AF_INET, AF_INET6, AF_UNIX
	int protocol   ;     //Transport protocol to use.
	int conntype   ;     //TCP, UDP, UNIX, etc. (sock_stream)
	int err        ;
	int connections;     //How many people have tried to connect to you?
	int bufsz      ;     //Size of read buffer 
	int backlog    ;     //Number of queued connections. 
	int waittime   ;     //There is a way to query socket, but do this for now to ensure your sanity.

#if 0
	Buffer *buffer;      //A buffer
#endif
	struct addrinfo 
            hints,     //Hints on addresses
	           *res;     //Results of address call.

	/* Server information -  used by bind() */
	struct sockaddr 
         *srvaddr,     // Server address structure */ 
	       *cliaddr;     // Client address structure */
	struct sockaddr_in 
      tmpaddrinfo,
     *cliaddrinfo,     //Client's address information 
     *srvaddrinfo;     //Server's address information 
	socklen_t 
	     cliaddrlen,     //Client
       srvaddrlen;     //Server
	size_t addrsize;     //Address information size
	void *ssl_ctx  ;                  /* If SSL is in use, use this */
} 
Socket;
#endif

#ifndef SINW_H
enum { NW_RECV = 0, NW_SEND };

/*stream selections*/
typedef enum 
{ 
	NW_STREAM_BUF = 0, 
	NW_STREAM_FD,
	NW_STREAM_PIPE
} Stream;

typedef enum 
{
	NW_AT_READ = 0,   /*The current connection is read ready.*/
	NW_AT_PROC,       /*The current connection is processing a response*/
	NW_AT_WRITE,      /*The current conenction is write ready*/
	NW_COMPLETED,     /*The current connection is done*/
	NW_AT_ACCEPT,     /*The current connection is about to be created*/
	NW_ERR            /*The current connection ran into some sort of error*/
} Stage;


typedef struct 
{
  Socket            child;
	Stage             stage;
	int                  rb, 
                       sb,  //Bytes received at an invocation of the "read loop"
                    recvd, 
                     sent;  //Total bytes sent or received
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
	struct timespec start;
	struct timespec end;
	int      status; 
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
#endif

#ifndef BUFF_H
Buffer *bf_init (Buffer *b, uint8_t *mem, int size);
void bf_setwsize (Buffer *b, int size); 
int bf_append (Buffer *b, uint8_t *s, int size); 
int bf_prepend (Buffer *b, uint8_t *s, int size); 
void bf_free (Buffer *b); 
const char *bf_err (Buffer *b);
int bf_written (Buffer *b) ;
uint8_t *bf_data (Buffer *b) ;
void bf_softinit (Buffer *b) ;
#endif


#ifndef PARSELY_H
void pr_prepare( Parser *p );
Parser *pr_next( Parser *p, unsigned char *src, int len );
void pr_print ( Parser *p );
#endif





#ifndef TAB_H
LiteType lt_add (Table *, int, LiteType, int, float, char *,
  unsigned char *, unsigned int , void *, Table *, char *);
Table *lt_init (Table *, LiteKv *, int) ;
void lt_printall ( Table *t );
void lt_finalize (Table *t) ;
void lt_dump (Table *t) ;
int lt_move(Table *t, int dir) ;
static void lt_printindex (LiteKv *tt, int ind);
LiteType lt_rettype( Table *t, int side, int index );
const char *lt_rettypename( Table *t, int side, int index );
void lt_lock (Table *t); 
int lt_get_long_i (Table *t, unsigned char *find, int len);
LiteKv *lt_next (Table *t);
void lt_reset (Table *t);
int lt_set (Table *t, int index);
LiteValue *lt_retany (Table *t, int index);
LiteRecord *lt_ret (Table *t, LiteType type, int index);
const char *lt_strerror (Table *t);
void lt_clearerror (Table *t);
void lt_setsrc (Table *t, void *src);
void lt_free (Table *t);
unsigned char *lt_trim (uint8_t *msg, char *trim, int len, int *nlen);
LiteKv *lt_items_i (Table *t, uint8_t *src, int len);
LiteKv *lt_items_by_index (Table *t, int ind);
int lt_count_elements ( Table *t, int index );
//Table *lt_copy (Table *t, int from, int to); 
int lt_exists (Table *t, int index);
int lt_counti ( Table *t, int index );
//Table *lt_within_long ( Table *t, uint8_t *src, int len );
Table *lt_within_long( Table *st, uint8_t *src, int len );
const char *lt_typename (int type);
 #ifdef DEBUG_H
  /*This is only enabled when debugging*/
  void lt_printt (Table *t);
 #endif
#endif


#ifndef RENDER_H  
int render_render ( Render *r );
int render_init ( Render *r, Table *t );
void render_free ( Render *r );
void render_set_srcdata (Render *r, uint8_t *src);
void render_set_srctable (Render *r, Table *t);
int render_map ( Render *r, uint8_t *src, int srclen );
Buffer *render_rendered (Render *r);
#endif


#ifndef SQROOGE_H
_Bool sq_init (Database *);
_Bool sq_open (Database *, const char *filename) ;
_Bool sq_exec (Database *, const char *sql) ;
_Bool sq_insert (Database *, const char *sql, const SQWrite *w);
_Bool sq_reader_start (Database *, const char *sql, const SQWrite *w) ;
_Bool sq_reader_continue (Database *) ;
int sq_reader_find (Database *, const char *colname) ;
_Bool sq_close (Database *);
int sq_find (Database *, const char *);
void print_db (Database *) ;
_Bool sq_create_oneshot (const char *, const char *) ;
int sq_insert_oneshot (const char *, const char *, const SQWrite *) ;
uint8_t *sq_read_oneshot(const char *, const char *, int) ;
int sq_get_query_size ( sqlite3_stmt *stmt ) ;
_Bool sq_setval (SQWrite *j, uint8_t *p, int len) ;
void sq_write_print (Database *, SQWrite *) ;
void sq_write_value_print (SQWrite *) ;
void sq_write_print (Database *, SQWrite *) ;
void sq_print_type(SQWrite *);
void print_database (Database *) ;
SQType sq_get_type (uint8_t *p, int len) ;
_Bool sq_serialize (Database *, const char *sql) ;
const SQWrite nullwriter;
int sq_save (Database *, const char *query, const char *name, const SQWrite *w );
const char *sq_strerror ( Database * );
 #ifdef SQROOGE_EXPERIMENTAL
  _Bool sq_read (Database *, const char *sql);
  _Bool sq_create (Database *, const char *);
 #endif
#endif

#ifndef MEM_H
_Bool memstr (const void * a, const void *b, int size);
int32_t memchrocc (const void *a, const char b, int32_t size);
int32_t memstrocc (const void *a, const void *b, int32_t size);
int32_t memstrat (const void *a, const void *b, int32_t size);
int32_t memchrat (const void *a, const char b, int32_t size);
int32_t memtok (const void *a, const uint8_t *tokens, int32_t rng, int32_t tlen);
int32_t memmatch (const void *a, const char *tokens, int32_t sz, char delim); 
char *memstrcpy (char *dest, const uint8_t *src, int32_t len);
_Bool memwalk (Mem *mm, uint8_t *data, uint8_t *tokens, int datalen, int toklen) ;
#endif

#ifndef JSON_H
int json_count ( uint8_t *src, int len );
int json_json ( Table *t, uint8_t *src, int len );
void json_free ( Table *t );
#endif


#ifndef MIME_H
const char *mtref (const char *) ;  //Always returns a reference
const char *mime_type_from_file (const char *) ;
const char *file_type_from_mime (const char *) ;
#endif

#ifndef RAND_H
 #ifdef MT_H
  char * rand_punct(Random *t, unsigned int length);
  char * rand_numbers(Random *t, unsigned int length);
  char * rand_chars(Random *t, unsigned int length);
  char * rand_alnum(Random *t, unsigned int length);
  char * rand_any(Random *t, unsigned int length);
  int rand_number(void);
  int between(Random *t, int x, int y);
  int range(int x, int y);
 #else
  char * rand_punct (int length);
  char * rand_numbers (int length);
  char * rand_chars (int length);
  char * rand_alnum (int length);
  char * rand_any(int length);
  int rand_number(void);
  int between(int x, int y);
  int range(int x, int y);
 #endif

 #ifdef EXPERIMENTAL_H 
  #ifndef MT_H
 void * rand_ptr(void *t, int length);
  #else
 void * rand_ptr(Random *t, void *t, unsigned int length);
  #endif
 #endif
void seed(void);
#endif

#ifndef FILES_H 
char file_type    (FileInfo *self, const char *path);
char * file_basename(FileInfo *self, const char *path);
char * file_dirname (FileInfo *self, const char *path);
char * file_realpath(FileInfo *self, const char *path);
 // LIST * list    (FileInfo *self, const char *path, file_t *file[]); 
List * file_list    (FileInfo *self, const char *path);
unsigned int file_exists  (FileInfo *self, const char *path);
unsigned int file_stat    (FileInfo *self, const char *path);
unsigned int file_chattr  (FileInfo *self, const char *path, unsigned int perms);
unsigned int file_mkdir   (FileInfo *self, const char *path, const char *mode);
unsigned int file_move    (FileInfo *self, const char *oldpath, const char *newpath);
unsigned int file_copy    (FileInfo *self, const char *oldpath, const char *newpath);
unsigned int file_remove  (FileInfo *self, const char *path);
unsigned int file_read_into (FileInfo *self, const char *path);
#endif

#ifndef SOCKET_H 
 /*Socket*/
 //void socket_info(struct SOCKET *self);
int socket_connect (Socket *self, const char *hostname, int port);
 void socket_free (Socket *self);
_Bool socket_open (Socket *sock);
 void socket_printf (Socket *sock);
_Bool socket_accept (Socket *sock, Socket *new);
//_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, uint32_t *len);
_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, int *len);
_Bool socket_tcp_rrecv (Socket *sock, uint8_t *msg, int get, uint32_t *len);
_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t len);
//_Bool socket_udp_recv (Socket *sock, uint8_t *msg, uint32_t *len);
//_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t len);

 //buffer_t * socket_recvd(Socket *self);
void socket_addrinfo(Socket *self);
_Bool socket_bind(Socket *self);
_Bool socket_listen(Socket *self);
//uri_t * socket_parsed(Socket *self);
_Bool socket_shutdown(Socket *self, char *type);
_Bool socket_close(Socket *self);
_Bool socket_recv(Socket *self); 
 //Socket * socket_accept(struct SOCKET *self);
_Bool socket_send(Socket *self, char *msg, unsigned int length); 
void socket_release(Socket *self);
#endif

#ifndef SINW_H
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

#ifndef UTIL_H
#define strcmbd( delim, ... ) \
	strcmbm( delim, __VA_ARGS__, NULL )
const char *shash_long ( const char *src, int len );
#define strhashd( str ) \
	shash_long( str, strlen( str ) )

int hashint( char *str );
char *strcmbm ( const char *delim, ... );
int load_file2 (uint8_t *dest, const char *file, int *size);
int load_file (Buffer *dest, const char *file, int *size);
unsigned char *trim (unsigned char *msg, char *trim, int len, int *nlen);
const char *str_combine (Buffer *b, const char *delim, ...);
const char *hash_long ( char *dest, const unsigned char *src, int len, int dlen );
#endif

#ifndef OPT_H
_Bool opt_usage (Option *, const char *, const char *, int);
_Bool opt_eval (Option *opts, int argc, char **argv);
 //union opt_value *get(Option *opts, const char *name);	
_Bool opt_set (Option *opts, const char *flag); 
Value opt_get (Option *opts, const char *flag); 

#endif
#endif
