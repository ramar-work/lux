#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "hypno/zmq.h"


int main (void) {
           void *ctx = zmq_ctx_new ();
           assert (ctx);
           /* Create ZMQ_STREAM socket */
           void *socket = zmq_socket (ctx, ZMQ_STREAM);
           assert (socket);
           int rc = zmq_bind (socket, "tcp://*:8080");
           assert (rc == 0);
           /* Data structure to hold the ZMQ_STREAM routing id */
           uint8_t routing_id [256];
           size_t routing_id_size = 256;
           /* Data structure to hold the ZMQ_STREAM received data */
           uint8_t raw [256];
           size_t raw_size = 256;
           while (1) {
                   /*  Get HTTP request; routing id frame and then request */
                   routing_id_size = zmq_recv (socket, routing_id, 256, 0);
                   assert (routing_id_size > 0);
                   do {
                           raw_size = zmq_recv (socket, raw, 256, 0);
                           assert (raw_size >= 0);
                   } while (raw_size == 256);


                   /* Prepares the response */
                   char http_response [] =
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 13\r\n"
                           "\r\n"
                           "Hello, World!";
                   /* Sends the routing id frame followed by the response */
                   zmq_send (socket, routing_id, routing_id_size, ZMQ_SNDMORE);
                   zmq_send (socket, http_response, strlen (http_response), 0);
         	}
          zmq_close (socket);
          zmq_ctx_destroy (ctx);
}
