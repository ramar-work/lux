/* ------------------------------------------- * 
 * zmime.h
 * ======
 * 
 * Summary 
 * -------
 * Header file for functions and data dealing 
 * with mimetypes.
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
#include <inttypes.h>
#include <string.h>
#include <strings.h>

#ifndef ZMIME_H
#define ZMIME_H
struct mime_t { 
	const char *extension; 
	const char *mimetype; 
};

const struct mime_t * zmime_get_by_extension ( const char * );
const struct mime_t * zmime_get_by_mime ( const char * );
const struct mime_t * zmime_get_default ();
char * zmime_get_extension ( const char * );

#define zmime_get_by_filename( ff ) \
	zmime_get_by_extension( zmime_get_extension( ff ) )

#if 0
#define zmime_mimetype( ff ) \
	zmime_get_by_extension( rindex( ff, '.' ) )->mimetype

#define zmime_extension( ff ) \
	zmime_get_by_extension( rindex( ff, '.' ) )->extension
#endif
#endif
