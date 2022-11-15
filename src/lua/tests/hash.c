#include "../hash.h"
#include <gnutls/crypto.h>

int main (int argc, char *argv[]) {

	//Generate all the hashes/sums 
	gnutls_hash_hd_t gd = {0};
	gnutls_hash_init( &gd
#if 0
	,	GNUTLS_MAC_SHA1
	,	GNUTLS_MAC_SHA224
	,	GNUTLS_MAC_SHA256
	,	GNUTLS_MAC_SHA384
#endif
	,	GNUTLS_MAC_SHA512
	);
#if 0
	gnutls_hmac( );
	gnutls_hmac_output( );
	gnutls_hmac_deinit( );
#endif
	return 0;
}
