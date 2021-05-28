/* ------------------------------------------- * 
 * zmime.c
 * ======
 * 
 * Summary 
 * -------
 * Functions and data allowing Hypno to deal with different mimetypes.
 * messages. 
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
#include "zmime.h"

static const struct mime_t mime[] = {
	{"unknown","application/octet-stream"/*octet-stream*/},
	{"html","text/html"/*defaultcontent-type*/},
	{"htm","text/html"},
	{"7z","application/x-7z-compressed"},
	{"aac","application/x-aac"},
	{"abc","text/vnd.abc"},
	{"apk","application/vnd.android.package-archive.xul+xml"},
	{"a","text/vnd.a"},
	{"atom","application/atom+xml"},
	{"avi","video/avi"},
	{"caf","application/x-caf"},
	{"cmd","text/cmd"},
	{"css","text/css"},
	{"csv","text/csv"},
	{"dart","application/vnd.dart"},
	{"deb","application/vnd.debian.binary-package"},
	{"djvu","image/vnd.djvu"},
	{"doc","application/vnd.ms-word"},
	{"docx","application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
	{"dtd","application/xml-dtd"},
	{"dvi","application/x-dvi"},
	{"ecma","application/ecmascript"},
	{"eml","message/partial"},
	{"eml","message/rfc822"},
	{"flac","audio/flac"},
	{"flv","video/x-flv"},
	{"gif","image/gif"},
	{"gz","application/gzip"},
	{"http","message/http"},
	{"ico","image/vnd.microsoft.icon"},
	{"iges","model/iges"},
	{"imdn","message/imdn+xml"},
	{"javascript","text/javascript"},
	{"jpeg","image/jpeg"},
	{"jpg","image/jpeg"},
	{"js","application/javascript"},
	{"json","application/json"},
	{"js","text/javascript"},
	{"kml","application/vnd.google-earth.kml+xml"},
	{"kmz","application/vnd.google-earth.kmz+xml"},
	{"l24","audio/l24"},
	{"m3u8","application/x-mpegURL"},
	{"md","application/x-markdown"},
	{"mesh","model/mesh"},
	{"mht","message/rfc822"},
	{"mhtml","message/rfc822"},
	{"mime","message/rfc822"},
	{"mk3d","video/x-matroska"},
	{"mka","video/x-matroska"},
	{"mks","video/x-matroska"},
	{"mkv","video/x-matroska"},
	{"mp3","audio/mp3"},
	{"mp4","audio/mp4"},
	{"mp4","video/mp4"},
	{"mpeg","audio/mp3"},
	{"msh","model/mesh"},
	{"nacl","application/x-nacl"},
	{"odg","application/vnd.oasis.opendocument.graphics"},
	{"odp","application/vnd.oasis.opendocument.presentation"},
	{"ods","application/vnd.oasis.opendocument.spreadsheet"},
	{"odt","application/vnd.oasis.opendocument.text"},
	{"ogg","audio/ogg"},
	{"ogt","video/ogg"},
	{"opus","audio/opus"},
	{"pdf","application/pdf"},
	{"pkcs","application/x-pkcs12"},
	{"pnacl","application/x-pnacl"},
	{"png","image/png"},
	{"ppt","application/vnd.ms-powerpoint"},
	{"pptx","application/vnd.openxmlformats-officedocument.presentationml.presentation"},
	{"ps","application/postscript"},
	{"quicktime","video/quicktime"},
	{"ra","audio/vnd.rn-realaudio"},
	{"rar","application/x-rar-compressed"},
	{"rdf","application/rdf+xml"},
	{"rss","application/rss+xml"},
	{"rtf","text/rtf"},
	{"sit","application/x-stuffit"},
	{"smil","application/smil+xml"},
	{"soap","application/soap+xml"},
	{"svg","image/svg+xml"},
	{"swf","application/x-shockwave-flash"},
	{"tar","application/x-tar"},
	{"tex","application/x-latex"},
	{"tiff","image/tiff"},
	{"tif","image/tiff"},
	{"ttf","application/x-font-ttf"},
	{"txt","text/plain"},
	{"ulaw","audio/basic"},
	{"vcard","text/vcard"},
	{"vorbis","audio/vorbis"},
	{"vrml","model/vrml"},
	{"wav","audio/vnd.wave"},
	{"webm","audio/webm"},
	{"wmv","video/x-ms-wmv"},
	{"woff","application/font-woff"},
	{"woff","application/x-font-woff"},
	{"wrl","model/vrml"},
	{"x","application/EDIFACT"},
	{"x","application/EDI-X12"},
	{"xcf","application/x-xcf"},
	{"xhtml","application/xhtml+xml"},
	{"xls","application/vnd.ms-excel"},
	{"xlsx","application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
	{"xml","application/xml"},
	{"xml","text/xml"},
	{"xop","application/xop+xml"},
	{"xps","application/vnd.ms-xpsdocument"},
	{"xul","application/vnd.mozilla.xul+xml"},
	{"zip","application/zip"},
	{NULL,NULL}
};


const struct mime_t *zmime_get_default() {
	return mime;
}


char * zmime_get_extension ( const char *filename ) {
	char *f = NULL;
	return ( ( f = rindex( filename, '.' ) ) ) ? ++f : NULL; 
}


const struct mime_t * zmime_get_by_extension ( const char *extension ) {
	if ( extension ) {
		for ( const struct mime_t *list = mime; list->extension; list++ ) {
			if ( strcmp( list->extension, extension ) == 0 ) {
				return list;
			}
		} 
	}
	return NULL;
}


const struct mime_t * zmime_get_by_mime ( const char *mimetype ) {
	for ( const struct mime_t *list = mime; list->mimetype; list++ ) {
		if ( strcmp( list->mimetype, mimetype ) == 0 ) {
			return list;
		}
	} 
	return NULL;
}


#if 0
//Searching for a mime starts here
uint8_t mime_search (const Mime *ml, const char *t, _Bool is_filetype) {
	for (int c = 0; c<(sizeof(mime)/sizeof(Mime)); c++) {
		if (strcmp(t, is_filetype ? ml[c].filetype : ml[c].mimetype) == 0)
			return c;
	}
	return 0;
}
#endif
