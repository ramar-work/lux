#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ztable.h>
#include <zwalker.h>

#ifndef ZJSON_H
#define ZJSON_H

#ifndef ZJSON_MAX_DEPTH
 #define ZJSON_MAX_DEPTH 100
#endif
 
#ifndef ZJSON_MAX_LENGTH
 #define ZJSON_MAX_LENGTH 2048
#endif

zTable * zjson_decode ( const char *, int, char *, int );
char * zjson_encode ( zTable *, char *, int ) ;
int zjson_check ( const char *, int, char *, int );
unsigned char *zjson_trim ( unsigned char *, char *, int , int * ) ;

#endif
