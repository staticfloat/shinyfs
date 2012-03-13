#include <zmq.hpp>
#include "protocol/ShinyMsg.h"
#include "base/Logger.h"
#include "ShinyTracker.h"
#include "../shared/filesystem/ShinyMetaDir.h"
#include "../shared/filesystem/ShinyMetaFile.h"
#include "../shared/filesystem/ShinyMetaNode.h"
#include "../shared/filesystem/ShinyMetaRootDir.h"

#define TRACKER_VERSION     "ShinyFS Tracker v0.1"

//This is the tracker, therefore we will have TWO external sockets.
//One PUB for connecting to the nodes and invalidating files, etc.
//One REP for listening to the nodes and performing things like write locks, etc.


/*
//Would be cool to implement zmq::device and dynamically change the number of threads available
//#define NUM_THREADS         1

void *worker_routine (void *arg) {
    zmq::context_t *context = (zmq::context_t *) arg;
    
    //Connect back into the main thread that doles out work
    zmq::socket_t socket (*context, ZMQ_REP);
    socket.connect("inproc://workers");
    
    while (true) {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv (&request);
        printf( "Thread %lu Received request: [%s]\n", (unsigned long)pthread_self(), request.data() );
        
        //  Do some 'work'
        sleep (1);
        
        //  Send reply back to client
        zmq::message_t reply (6);
        memcpy ((void *) reply.data(), "World", 6);
        socket.send (reply);
    }
    return NULL;
}
/**/


int main( int argc, const char * argv[] ) {
    getGlobalLogger()->setPrintLoc( 0 );
    LOG( "%s starting up....", TRACKER_VERSION );
    
    // Load in filesystem database (contains file listing, write locks, 
    ShinyTracker * tracker = new ShinyTracker( "tracker_state" );
    
    //  Prepare our context and sockets
    zmq::context_t context (1);
    
    // This is the REP socket for nodes to connect to
    LOG( "Is there a reason this shouldn't be ROUTER?" );
    zmq::socket_t nodes( context, ZMQ_REP );
    nodes.bind( "tcp://*:5041" );
    
    // This is the PUB socket to broadcast to nodes
    zmq::socket_t pub( context, ZMQ_PUB );
    pub.bind( "tcp://*:5040" );

    // NABIL: Need to change this to something more meaningful, lol
    void * senderData = NULL;
    
    
    LOG( "Beginning test!" );
    ShinyMetaFilesystem * newFs = new ShinyMetaFilesystem( NULL );
    newFs->print();
    //get the root
    ShinyMetaDir * rootdir = (ShinyMetaDir *) newFs->findNode( (inode_t)0 );
    
    ShinyMetaFile * test_file = new ShinyMetaFile( newFs, "test" );
    rootdir->addNode(test_file);
    
    ShinyMetaDir * sub_dir = new ShinyMetaDir( newFs, "sub" ); 
    rootdir->addNode( sub_dir );
    
    ShinyMetaFile * woohoo_file = new ShinyMetaFile( newFs, "woohoo" );
    sub_dir->addNode( woohoo_file );
    
    newFs->print();
    
    LOG("Now do serialization test" );
    char * serialized = NULL;
    uint64_t serializedLen = newFs->serialize(&serialized);
    printf( "%s\n", serialized );

    ShinyMetaFilesystem * undoFs = new ShinyMetaFilesystem( serialized );
    undoFs->print();
    return 0;
    

    // NABIL: Need to make a better ending condition, LOL
    while( true ) {
        //Get a new message from the nodes
        ShinyMsg request( &nodes );
        
/*        LOG( "Got a message! Type %d, ", newMsg.getMsgType() );
        LOG( "Datalen %llu: ", newMsg.getDataLen() );
        LOG( "Message: %s\n", newMsg.getData() );
 */
        
        switch( request.getMsgType() ) {
            case ShinyMsg::FS_REQ: {
                char * serializedData;
                uint64_t len = tracker->serializeMetaTree( &serializedData );
                ShinyMsg reply( senderData, ShinyMsg::FS_DATA, serializedData, len );
                break; }
            default:
                break;
        }
        
        
        ShinyMsg reply( NULL, 0, "The cake is a lie", 18 );
        reply.send( &nodes );
    }
    
    /*
    // This is to talk to the worker threads
    zmq::socket_t workers( context, ZMQ_DEALER );
    workers.bind("inproc://workers");
    
    // Launch pool of worker threads
    for (int thread_nbr = 0; thread_nbr < NUM_THREADS; thread_nbr++) {
        pthread_t worker;
        pthread_create (&worker, NULL, worker_routine, (void *) &context);
    }
    // Connect work threads to client threads via a queue
    zmq::device (ZMQ_QUEUE, clients, workers);
    */
    return 0;
}


