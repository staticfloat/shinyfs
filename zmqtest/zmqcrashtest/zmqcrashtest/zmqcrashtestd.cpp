#include <stdio.h>
#include <zmq.hpp>
#include <stdlib.h>

#define ADDR "tcp://127.0.0.1:5040"

int main (int argc, const char * argv[])
{
    zmq::context_t ctx(1);
    zmq::socket_t sock( ctx, ZMQ_ROUTER );
    sock.setsockopt( ZMQ_IDENTITY, ADDR, strlen(ADDR) );
    sock.bind( ADDR );

    while( true ) {
        zmq::message_t msg;
        sock.recv( &msg );
        printf(".");
        fflush(stdout);
    }
    return 0;
}

