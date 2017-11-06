/*
//hypno-chain.c 
Hypno's power is based off of a chain of data, this 
just runs the chain/evaluator part. 

Technically, the thing can keep running things in 
whatever order.  But CC_VIEW should always be the 
last thing evaluated.

Rendering is VERY easy if all of the data is put into
Lua.  The data is converted to table and rendering is
done.

HOWEVER, 
- Rendering needs to be battle tested.
(Try for at least 50 and make sure that things work)

- BIG ASS TABLES need to be tested.
(You may have up 1000 keys., Also test this)

*/
#include "vendor/single.h"
#include "vendor/http.h"
#include "bridge.h"


Loader l[] = {
	{ CC_MODEL, "a" },
	{ CC_MODEL, "b" },
	{ 0, NULL }
};



int main (int argc, char *argv[])
{
	Loader *ll = l;
	while ( ll->type ) 
	{
		//Header
		fprintf( stderr, "Evaluating %s:\n%s\n", 
			printCCtype( ll->type ), "===================" );

		//Do based on ll->type 

		//Add to buffer depending on choice
		
		//Next one
		ll++;
	}
	return 0;
}
