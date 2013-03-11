#include "zmqutils.h"
#include "../filesystem/ShinyMetaNode.h"
#include "../filesystem/ShinyMetaFile.h"
#include "../filesystem/ShinyMetaFileHandle.h"
#include "../filesystem/ShinyMetaDir.h"
#include "../filesystem/ShinyMetaRootDir.h"
#include "base/Logger.h"
#include <stdarg.h>

// Returns true if the socket has more to be read
bool checkMore( zmq::socket_t * sock ) {
    int64_t opt;
    size_t optLen = sizeof(int64_t);
    sock->getsockopt(ZMQ_RCVMORE, &opt, &optLen);
    return opt;
}

// Returns all linked messages on a socket in a vector
void recvMessages( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList ) {
    //Try and receive a message
    zmq::message_t * msg;
    do {
        //Check for new message
        msg = new zmq::message_t();
        try {
            // if recv() is false, it's EAGAIN
            if( sock->recv( msg ) == false ) {
                delete( msg );
                return;
            }
        } catch(...) {
            if( errno != EINTR )
                WARN( "sock->recv() failed! %d: %s", errno, strerror(errno) );
            else {
                // don't give garbage messages
                return;
            }
        }
        
        //Push it onto the vector
        msgList.push_back(msg);
        
        //LOG( "Received: %d bytes", msg->size() );
    } while( checkMore( sock ) );
}

// deletes all messages in a vector of msgs
void freeMsgList( std::vector<zmq::message_t *> & msgList ) {
    for( uint64_t i=0; i<msgList.size(); ++i )
        delete( msgList[i] );
    msgList.clear();
}

// Probably gonna have to deal with ugly windows stuff here
void sendMessages( zmq::socket_t * sock, uint64_t numMsgs, ... ) {
    va_list ap;
    va_start( ap, numMsgs );
    for( uint64_t i=0; i<numMsgs - 1; ++i ) {
        zmq::message_t * msg = (va_arg(ap, zmq::message_t *));
        try { 
            sock->send( *msg, ZMQ_SNDMORE );
        } catch(...) {
            WARN( "sock->send() failed! %d: %s", errno, strerror(errno) );
            va_end(ap);
            return;
        }
    }
    
    try {
        sock->send( *(va_arg(ap, zmq::message_t *)) );
    } catch( ... ) {
        WARN( "sock->send() failed (and on the last one)! %d: %s", errno, strerror(errno) );
    }
    va_end( ap );
}

// And a mimic version with a vector, so that if we don't _know_ how many beforehand, we can do that too!
void sendMessages( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList ) {
    for( uint64_t i=0; i<msgList.size()-1; ++i ) {
        try { 
            sock->send( *(msgList[i]), ZMQ_SNDMORE );
        } catch(...) {
            WARN( "sock->send() failed! %d: %s", errno, strerror(errno) );
            return;
        }
    }
    try {
        sock->send( *(msgList[msgList.size()-1]) );
    } catch( ... ) {
        WARN( "sock->send() failed (and on the last one)! %d: %s", errno, strerror(errno) );
    }
}

// builds a zmq::message_t for the type
void buildTypeMsg( const uint8_t type, zmq::message_t * msg ) {
    msg->rebuild( sizeof(uint8_t) );
    ((uint8_t *)msg->data())[0] = type;
}

// more generally, build a message around some data
void buildDataMsg( const void * data, uint64_t len, zmq::message_t * msg ) {
    msg->rebuild( len );
    memcpy( msg->data(), data, len );
}

// builds a msg for a string (remember, no NULL char!)
void buildStringMsg( const char * string, zmq::message_t * msg ) {
    uint64_t len = strlen( string );
    buildDataMsg( string, len, msg );
}

// builds a message by serializing a node into a buffer
void buildNodeMsg( ShinyMetaNode * node, zmq::message_t * msg ) {
    uint64_t len = node->serializedLen();
    char * buff = new char[len];
    node->serialize(buff);
    
    // Let's make him do all the work.  :P
    buildDataMsg( buff, len, msg );
}

// Parses a uint8_t out of a zmq message
uint8_t parseTypeMsg( zmq::message_t * msg ) {
    return ((uint8_t*)msg->data())[0];
}

// Parses data out, basically a conveninece function to save myself two lines of code. :P
char * parseDataMsg( zmq::message_t * msg ) {
    uint64_t size = msg->size();
    char * data = new char[size];
    memcpy( data, msg->data(), size );
    return data;
}

// Parses a string out of a zmq message; note you must free the string yourself!
char * parseStringMsg( zmq::message_t * msg ) {
    uint64_t size = msg->size();
    char * str = new char[size+1];
    memcpy( str, msg->data(), size );
    str[size] = 0;
    return str;
}

// Unserializes a serialized node out of the message, and yes, needs apriori info about the type
ShinyMetaNode * parseNodeMsg( zmq::message_t * msg, ShinyMetaNodeSnapshot::NodeType type, ShinyFilesystem * fs ) {
    const char * data = (const char *) msg->data();
    switch( type ) {
        case ShinyMetaNodeSnapshot::TYPE_FILE: {
            return new ShinyMetaFile( &data, NULL );
        }
        case ShinyMetaNodeSnapshot::TYPE_DIR: {
            return new ShinyMetaDir( &data, NULL );
        }
        case ShinyMetaNodeSnapshot::TYPE_ROOTDIR: {
            return new ShinyMetaRootDir( &data, fs );
        }
        default: {
            return new ShinyMetaNode( &data, NULL );
        }
    }
}

// Waits for a ZMQ endpoint to become connect()'able, failing out after 1 second
bool waitForEndpoint( zmq::context_t * ctx, const char * endpoint ) {
    // Keep track of the start of this function, so we know when to cut our losses
    uint64_t start = clock();
    
    // Max timeout, 1 second
    while( clock() - start < CLOCKS_PER_SEC ) {
        try {
            zmq::socket_t testSock( *ctx, ZMQ_REQ );
            testSock.connect( endpoint );
            testSock.close();
            return true;
        } catch( ... ) {
            usleep( 10*1000 );
        }
    }
    ERROR( "Timeout waiting for %s to become available!", endpoint );
    return false;
}
