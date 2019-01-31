//  Minimal HTTP server in 0MQ
#include "czmq.h"

int main (void)
{
    zctx_t *ctx = zctx_new ();
    void *stream = zsocket_new (ctx, ZMQ_STREAM);
    int rc = zsocket_bind (stream, "tcp://*:8080");
    assert (rc != -1);
    
    while (true) {
        //  Get HTTP request
        zframe_t *handle = zframe_recv (stream);
        if (!handle)
            break;          //  Ctrl-C interrupt
        char *request = zstr_recv (stream);
        puts (request);     //  Professional Logging(TM)
        free (request);     //  We throw this away

        //  Send Hello World response
        zframe_send (&handle, stream, ZFRAME_MORE + ZFRAME_REUSE);
        zstr_send (stream,
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Hello, World!");

        //  Close connection to browser
        zframe_send (&handle, stream, ZFRAME_MORE);
        zmq_send (stream, NULL, 0, 0);
    }
    zctx_destroy (&ctx);
    return 0;
}
