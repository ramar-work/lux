/* ------------------------------------------- * 
 * filter-echo.h
 * ===========
 * 
 * Summary 
 * -------
 * Header file for functions comprising the echo filter for stress testing 
 * Hypno's capabilities. 
 *
 * Usage
 * -----
 * filter-echo.c forces hypno to simply echo back what was sent.  This is mostly
 * for testing and has little use for anything but diagnostics.  It can safely
 * be disabled in production.
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
#include "../vendor/zhttp.h"
#include "mime.h"
#include "util.h"
#include "server.h"

#ifndef FILTER_ECHO_H
#define FILTER_ECHO_H

const int filter_echo ( int, struct HTTPBody *, struct HTTPBody *, struct cdata * );

#endif
