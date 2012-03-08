#include <stdio.h>
#include <string.h>
#include <zmq.hpp>
#include <list>
#include <stdlib.h>
#include <time.h>

/*
unsigned short getNewPort( void ) { 
    unsigned short port = 0;
 
    while( port == 0 ) {
        printf( "Enter port number: " );
        scanf( "%hd", &port );
        if( !port )
            printf( "Invalid port number!\n" );
    }
    return port;
}

void myfree( void * data, void * hint ) {
    free( data );
}

void sendMessages( zmq::context_t & ctx, std::list<unsigned short> & ports ) {
    zmq::socket_t router( ctx, ZMQ_ROUTER );
    for( std::list<unsigned short>::iterator itty = ports.begin(); itty != ports.end(); ++itty ) {
        try {
            char * addr = (char *) malloc(32);
            sprintf( addr, "tcp://127.0.0.1:%d", *itty );
            
            router.connect( addr );
            usleep( 10000 );
            
            printf( "Attempting to contact %s...\n", &addr[6] );

            zmq::message_t msg( addr, strlen(addr), NULL );
            router.send( msg, ZMQ_SNDMORE );
            
            //zmq::message_t blankmsg(0);
            //router.send( blankmsg, ZMQ_SNDMORE );
            zmq::message_t dataMsg( addr, strlen(addr), myfree );
            router.send( dataMsg );
        } catch( zmq::error_t e ) {
            printf( "Couldn't connect to %d!\n", *itty );
        }
    }
    zmq::message_t msg;
    router.recv( &msg );
    printf("Done!\n" );
}


void printHelp( void ) {
    printf( "commands:\n" );            
    printf( "  [space] - send packets to all nodes\n" );
    printf( "  [a]     - add new node by port number\n" );
    printf( "  [d]     - remove port by value\n" );
    printf( "  [tab]   - dump loaded ports\n" );
    printf( "  [h]     - print help\n" );
}
*/

bool checkMore( zmq::socket_t * sock ) {
    int64_t opt;
    size_t optlen = sizeof(opt);
    sock->getsockopt( ZMQ_RCVMORE, &opt, &optlen );
    return opt;
}

void printMsg( zmq::message_t &msg ) {
    char * data = (char *) msg.data();
    for( int i=0; i<msg.size(); ++i ) {
        if( isprint(data[i]) )
            printf( "%c", data[i] );
        else
            printf( "?", data[i] );
    }
    printf("\n");
}

int main (int argc, const char * argv[])
{
    zmq::context_t ctx(1);
    
    if( argc == 1 || strcmp( argv[1], "-s" ) != 0 ) {
        //If we're in "client" mode, create a ZMQ_REQ
        zmq::socket_t sock( ctx, ZMQ_DEALER );
        sock.connect( "tcp://127.0.0.1:5040" );
        
        //Send some random 
        char * sendAsLabel = "localhost:5040";
        zmq::message_t labelMsg( (void *)sendAsLabel, strlen(sendAsLabel) + 1, NULL );
        sock.send( labelMsg, ZMQ_SNDMORE );
        
        zmq::message_t blankMsg;
        sock.send( blankMsg, ZMQ_SNDMORE );
        
        //Send whatever random junk is on the stack as data
        char sendAsData[80];
        zmq::message_t dataMsg( (void *)&sendAsData[0], sizeof(sendAsData), NULL );
        sock.send( dataMsg );

        zmq::message_t recvMsg;
        sock.recv(&recvMsg);
        printMsg( recvMsg );
    } else {
        zmq::socket_t sock( ctx, ZMQ_ROUTER );
        
        char * addr = "tcp://127.0.0.1:5040";
        sock.setsockopt( ZMQ_IDENTITY, addr, strlen(addr) );
        sock.bind( addr );
        while( true ) {
            zmq::message_t msg;
            sock.recv(&msg);
            
            while( checkMore(&sock) ) {
                zmq::message_t dummyMsg;
                sock.recv(&dummyMsg);
                printMsg(dummyMsg);
            }
            
            sock.send(msg, ZMQ_SNDMORE);
            
            zmq::message_t dataMsg( (void *)"DATA", 4, NULL );
            sock.send( dataMsg );
            printf( "--------------------\n" );
        }
    }
    return 1;
}

