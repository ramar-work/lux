#ifndef CTXDNS_H
#define CTXDNS_H
const int write_dns ( int, zhttp_t *, zhttp_t *, struct cdata * );
const int read_dns ( int, zhttp_t *, zhttp_t *, struct cdata * );
void create_dns ( void **p );
#endif
