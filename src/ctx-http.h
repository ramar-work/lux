/* ------------------------------------------- * 
 * ctx-http.h
 * ========
 * 
 * Summary 
 * -------
 * Functions for dealing with HTTP contexts.
 *
 * Usage
 * -----
 * Compile me with: 
 * gcc -ldl -llua -o config vendor/single.o config.c luabind.c && ./config
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
#include "socket.h"
#include "server.h"
#include "../vendor/zhttp.h"
#include <signal.h>

void create_notls ( void **p ); 
int read_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);
int write_notls ( int, struct HTTPBody *, struct HTTPBody *, void *);

//Added 12-08-20, signal handler for IO
//int read_ready ( int sn, const struct rt_sigaction *na, struct rt_sigaction *oa );
