//ctx-return.h

//Define a datatype that can return a sink and a function
//that can advance through said sink

//Because of large sizes, this is needed for some contexts
struct ctxreturn_t {

	void *result;
	void *(*advance)( int );

}
