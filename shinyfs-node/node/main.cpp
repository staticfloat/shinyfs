#include <zmq.hpp>
#include <base/Logger.h>
#include "protocol/ShinyMsg.h"
#include "protocol/ShinyPartitioner.h"
#include "ShinyNode.h"

#include "ShinyMetaNode.h"
#include "ShinyMetaRootDir.h"
#include "ShinyMetaFile.h"
#include "ShinyMetaDir.h"

#define NODE_VERSION        "ShinyFS Node v0.2"

#define TRACKER_ADDRESS     "127.0.0.1"


int main (int argc, const char * argv[]) {
    getGlobalLogger()->setPrintLoc(1);
    LOG( "%s starting up....", NODE_VERSION );
/*
    //Create our context and sockets
    zmq::context_t context(1);
    
    //Create the req socket to talk to the tracker
    zmq::socket_t tracker( context, ZMQ_REQ );
    tracker.connect( "tcp://" TRACKER_ADDRESS ":5041" );
    
    //Create the SUB socket to listen to the tracker's updates
    zmq::socket_t sub( context, ZMQ_SUB );
    sub.connect( "tcp://" TRACKER_ADDRESS ":5040" );
*/
    ShinyMetaFilesystem fs;
    
    ShinyMetaRootDir * root = (ShinyMetaRootDir *) fs.findNode("/");
    
    ShinyMetaFile * f0 = new ShinyMetaFile( &fs, "f0", root );
    ShinyMetaDir * dir0 = new ShinyMetaDir( &fs, "dir0", root );
    new ShinyMetaFile( &fs, "f1", dir0 );
    ShinyMetaFile * f2 = new ShinyMetaFile( &fs, "f2", dir0 );
    new ShinyMetaFile( &fs, "f3", dir0 );
    new ShinyMetaFile( &fs, "f4", dir0 );
    new ShinyMetaFile( &fs, "f5", dir0 );
    ShinyMetaDir * dir1 = new ShinyMetaDir( &fs, "dir1", dir0 );
    dir1->addNode( f0 );
    dir1->addNode( f2 );

    fs.print();
    fs.sanityCheck();
    
    
    std::list<ShinyPeer *> peers;
    peers.push_back( new ShinyPeer() );
    peers.push_back( new ShinyPeer() );
    peers.push_back( new ShinyPeer() );
    ShinyPartitioner part( &fs, (ShinyMetaDir *)fs.findNode("/"), &peers, NULL );
    
    for( std::list<ShinyPeer *>::iterator itty = peers.begin(); itty != peers.end(); ++itty )
        delete( *itty );
    return 0;
}

