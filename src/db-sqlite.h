/* ------------------------------------------- * 
 * db-sqlite.h
 * -----------
 * Header file for functions for dealing with data interchange between zTable 
 * and sqlite3.
 *
 * Usage
 * -----
 * gcc -ldl -llua -o database vendor/single.o database.c && ./config
 * 
 *
 * LICENSE
 * -------
 * Copyright 2020 Tubular Modular Inc. dba Collins Design
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or 
 * sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 * CHANGELOG 
 * ---------
 * 
 * ------------------------------------------- */
#include "../vendor/zwalker.h"
#include "../vendor/zhasher.h"
#include "luabind.h"
#include <sqlite3.h>

#ifndef HDATABASE_H
#define HDATABASE_H

void *db_open( const char *, char *, int ) ;
void *db_close( void **, char *, int );
zTable *db_exec( void *, const char *, void **, char *, int ) ;
zTable *db_to_table ( const char *filename, const char *query );

#endif
