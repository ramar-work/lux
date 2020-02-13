#include <inttypes.h>
#include <string.h>

#ifndef MIME_H
#define MIME_H
struct mime { const char *extension, *mimetype; };

const char *mmtref (const char *);
const char *mfiletype_from_mime (const char *);
const char *mmimetype_from_file (const char *);
#endif
