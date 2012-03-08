#include <stdio.h>
#include <zmq.hpp>

#define ADDR "tcp://127.0.0.1:5040"

void sendMessages( zmq::context_t & ctx ) {
    zmq::socket_t router( ctx, ZMQ_ROUTER );
    printf( "Connecting to %s....\n", ADDR );
    router.connect( ADDR );
    usleep( 10*1000 );
    
    zmq::message_t msg( (void *)ADDR, strlen(ADDR), NULL );
    router.send( msg, ZMQ_SNDMORE );
    router.send( msg );
    printf("Done!\n" );
}


int main (int argc, const char * argv[])
{
    zmq::context_t ctx(1);
    while( true ) {
        sendMessages( ctx );
    }
    return 0;
}

