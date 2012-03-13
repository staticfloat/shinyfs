#include "ShinyFuse.h"
#include <base/Logger.h>
#include "../filesystem/ShinyMetaDir.h"
#include "../filesystem/ShinyMetaFile.h"
#include <sys/errno.h>
#include <stdarg.h>

ShinyFilesystem * ShinyFuse::fs;
zmq::context_t * ShinyFuse::ctx;
pthread_t ShinyFuse::brokerThread;

bool ShinyFuse::init( ShinyFilesystem * fs, zmq::context_t * ctx, const char * mountPoint ) {
    //First, setup the callbacks
    struct fuse_operations shiny_operations;
    memset( &shiny_operations, NULL, sizeof(shiny_operations) );

    shiny_operations.init = ShinyFuse::fuse_init;
    shiny_operations.destroy = ShinyFuse::fuse_destroy;

    shiny_operations.getattr = ShinyFuse::fuse_getattr;
    shiny_operations.readdir = ShinyFuse::fuse_readdir;
    
    shiny_operations.access = ShinyFuse::fuse_access;
    
    /*
    shiny_operations.open = ShinyFuse::fuse_open;
    shiny_operations.read = ShinyFuse::fuse_read;
    
    shiny_operations.write = ShinyFuse::fuse_write;
    shiny_operations.truncate = ShinyFuse::fuse_truncate;
    shiny_operations.create = ShinyFuse::fuse_create;
    shiny_operations.mknod = ShinyFuse::fuse_mknod;
    shiny_operations.mkdir = ShinyFuse::fuse_mkdir;
    
    shiny_operations.unlink = ShinyFuse::fuse_unlink;
    shiny_operations.rmdir = ShinyFuse::fuse_rmdir;
    
    shiny_operations.rename = ShinyFuse::fuse_rename;
    
    
    shiny_operations.chmod = ShinyFuse::fuse_chmod;
    shiny_operations.chown = ShinyFuse::fuse_chown;
    */
    
    // Load in our parameters
    ShinyFuse::fs = fs;
    ShinyFuse::ctx = ctx;
    
    // Make sure mount point is viable
    struct stat st;
    if( stat( mountPoint, &st ) != 0 ) {
        WARN( "Couldn't find mount point %s, attempting to create...", mountPoint );
        if( mkdir( mountPoint, S_IRWXU | S_IRWXG | S_IROTH ) != 0 ) {
            ERROR( "Could not create mount point %s!", mountPoint );
            return false;
        }
    }
    
    // Start worker thread to take care of all our requests; chatter from fuse will
    // go into the worker thread, which will then manipulate the ShinyFS object
    if( pthread_create( &ShinyFuse::brokerThread, NULL, &ShinyFuse::broker, NULL ) != 0 ) {
        ERROR( "Could not create broker thread!" );
        return false;
    }
    
    LOG( "Waiting for broker startup..." );
    zmq::socket_t * sock;
    while( (sock = getBroker()) == NULL )
        usleep( 10000 );
    delete( sock );
    
    // Start fuse reactor, now that we've defined all our callbacks
    try {
        char * argv[] = {
            (char *)"./shinyfs",
            (char *)mountPoint,
            (char *)"-f",
//            (char *)"-d",
        };
        fuse_main( sizeof(argv)/sizeof(char *), argv, &shiny_operations, NULL );
    } catch( ... ) {
        ERROR( "fuse_main() died!" );
    }
    return true;
}




// Returns true if the socket has more to be read
bool checkMore( zmq::socket_t * sock ) {
    int64_t opt;
    size_t optLen = sizeof(opt);
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
            // if recv() is false, signifies EAGAIN
            if( sock->recv( msg, ZMQ_NOBLOCK ) == false ) {
                delete( msg );
                return;
            }
        } catch(...) {
            if( errno != EINTR )
                WARN( "sock->recv() failed! %d: %s", errno, strerror(errno) );
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


// Parses a string out of a zmq message; note you must free the string yourself!
char * parseStringMsg( zmq::message_t * msg ) {
    uint64_t size = msg->size();
    char * str = new char[size+1];
    memcpy( str, msg->data(), size );
    str[size] = NULL;
    return str;
}

// Parses a uint8_t out of a zmq message
uint8_t parseTypeMsg( zmq::message_t * msg ) {
    return ((uint8_t*)msg->data())[0];
}

// Extreme laziness function to send a NACK to the other side
void sendNACK( zmq::socket_t * sock, zmq::message_t * routing ) {
    // Send back failure, we couldn't find that node!
    zmq::message_t nackMsg;
    buildTypeMsg( ShinyFuse::NACK, &nackMsg );
    
    zmq::message_t blankMsg;
    sendMessages(sock, 3, routing, &blankMsg, &nackMsg );
}




void * ShinyFuse::broker( void *data ) {
    // Our ROUTER socket to deal with all incoming fuse noise
    zmq::socket_t * brokerSock = new zmq::socket_t( *ShinyFuse::ctx, ZMQ_ROUTER );
    brokerSock->bind( ShinyFuse::getZMQEndpointFuse() );
    
    // Make it nonblock and awesome
//    uint64_t noblock = 1;
//    brokerSock->setsockopt( ZMQ_NOBLOCK, &noblock, sizeof(uint64_t) );

    // My list of zmq messages
    std::vector<zmq::message_t * > msgList;

    bool keepRunning = true;
    // Keep going as long as we don't get a DESTROY message.  :P
    while( keepRunning ) {
        // Check for messages from fuse threads
        recvMessages( brokerSock, msgList );
        
        // If we actually received anything
        if( msgList.size() ) {
            // Now begins the real work.
            keepRunning = ShinyFuse::handleMessage( brokerSock, msgList );
            
            freeMsgList( msgList );
        }
        
        // Sleep for just a little bit.  :D
        usleep(1000);
    }
    
    // Cleanup the socket so that closing the context doesn't wait forever.  :P
    delete( brokerSock );
    return NULL;
}

bool ShinyFuse::handleMessage( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList ) {
    zmq::message_t * fuseRoute = msgList[0];
    zmq::message_t * blankMsg = msgList[1];
    
    // Following the protocol (briefly) laid out in ShinyFuse.h;
    switch( parseTypeMsg(msgList[2]) ) {
        case ShinyFuse::DESTROY:
        {
            // No actual response data, just sending response just to be polite
            zmq::message_t ackMsg; buildTypeMsg( ShinyFuse::ACK, &ackMsg );
            
            sendMessages( sock, 2, fuseRoute, &ackMsg );
            
            // return false as we're signaling to broker that it's time to die.  >:}
            return false;
        }
        case ShinyFuse::GETATTR:
        {
            // Let's actually get the data fuse wants!
            char * path = parseStringMsg( msgList[3] );
            
            // If the node even exists;
            ShinyMetaNode * node = fs->findNode( path );
            if( node ) {
                // This is the struct we're eventually going to send back
                struct stat stbuff;
                
                // Let's fill it up with data
                stbuff.st_mode = node->getPermissions();
                switch( node->getNodeType() ) {
                    case ShinyMetaNode::TYPE_DIR:
                    case ShinyMetaNode::TYPE_ROOTDIR:
                        stbuff.st_mode |= S_IFDIR;
                        stbuff.st_nlink = ((ShinyMetaDir *)node)->getNumNodes();
                        break;
                    case ShinyMetaNode::TYPE_FILE:
                        stbuff.st_mode |= S_IFREG;
                        stbuff.st_nlink = 1;
                        
                        stbuff.st_size = ((ShinyMetaFile *)node)->getLen();
                        break;
                    default:
                        WARN( "Couldn't understand the node type of node %s! (%d)", path, node->getNodeType() );
                        break;
                }
                
                zmq::message_t ackMsg; buildTypeMsg( ShinyFuse::ACK, &ackMsg );
                zmq::message_t stbuffMsg; buildDataMsg( &stbuff, sizeof(struct stat), &stbuffMsg );
                
                sendMessages( sock, 4, fuseRoute, blankMsg, &ackMsg, &stbuffMsg );
            } else
                sendNACK( sock, fuseRoute );
            
            // cleanup after the parseStringMsg()
            delete( path );
            break;
        }
        case ShinyFuse::READDIR:
        {
            char * path = parseStringMsg( msgList[3] );
            
            // If the node even exists;
            ShinyMetaNode * node = fs->findNode( path );
            if( node && (node->getNodeType() == ShinyMetaNode::TYPE_DIR || node->getNodeType() == ShinyMetaNode::TYPE_ROOTDIR) ) {
                const std::vector<ShinyMetaNode *> * children = ((ShinyMetaDir *) node)->getNodes();
                
                // Here is my crucible for data to be pummeled out of my autocannon, ZMQ
                std::vector<zmq::message_t *> list( 1 + 1 + 1 + children->size() );
                list[0] = fuseRoute;
                list[1] = blankMsg;
                
                zmq::message_t ackMsg; buildTypeMsg( ShinyFuse::ACK, &ackMsg );
                list[2] = &ackMsg;
                
                for( uint64_t i=0; i<children->size(); ++i ) {
                    zmq::message_t * childMsg = new zmq::message_t(); buildStringMsg( (*children)[i]->getName(), childMsg );
                    list[3+i] = childMsg;
                }
                
                sendMessages( sock, list );
                
                // Free up those childMsg structures and ackMsg while we're at it (ackMsg is list[1])
                for( uint64_t i=3; i<list.size(); ++i ) {
                    delete( list[i] );
                }
            } else
                sendNACK( sock, fuseRoute );

            // cleanup, cleanup everybody everywhere!
            delete( path );
            break;
        }
        default:
        {
            WARN( "Unknown ShinyFuse message type! (%d) Sending NACK:", parseTypeMsg(msgList[2]) );
            sendNACK( sock, fuseRoute );
            break;
        }
    }
    
    return true;
}








const char * ShinyFuse::getZMQEndpointFuse() {
    return "inproc://broker.fuse";
}

zmq::socket_t * ShinyFuse::getBroker() {
    zmq::socket_t * sock = new zmq::socket_t( *ctx, ZMQ_REQ );
    try {
        sock->connect( getZMQEndpointFuse() );
    } catch(...) {
        ERROR( "Could not connect to %s! %d: %s", getZMQEndpointFuse(), errno, strerror(errno) );
        delete( sock );
        return NULL;
    }
    
    uint64_t nonblock = 0;
    sock->setsockopt( ZMQ_NOBLOCK, &nonblock, sizeof(uint64_t));
    return sock;
}


void * ShinyFuse::fuse_init( struct fuse_conn_info * conn ) {
    LOG( "init" );
    return NULL;
}

void ShinyFuse::fuse_destroy( void * private_data ) {
    LOG( "destroy" );
    zmq::socket_t * sock = getBroker();
    
    zmq::message_t destroyMsg; buildTypeMsg( ShinyFuse::DESTROY, &destroyMsg );
    sendMessages(sock, 1, &destroyMsg );
    
    // clean this one up too, lol
    delete( sock );
    
    if( pthread_join( ShinyFuse::brokerThread, NULL ) != 0 ) {
        ERROR( "pthread_join() failed on destruction of brokerThread!" );
    }
    //fs->save();
    //fs->sanityCheck();
    
    // Gotta cleanup after ourselves, even though fs was created in main()
    delete( fs );

    LOG( "saved and sanitycheck'ed!" );
}

int ShinyFuse::fuse_getattr( const char *path, struct stat * stbuff ) {
    LOG( "getattr: [%s]", path );
    
    // First, get a socket to the broker
    zmq::socket_t * sock = getBroker();
    if( sock ) {
        // Build the messages we're going to send
        zmq::message_t typeMsg; buildTypeMsg( ShinyFuse::GETATTR, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send those messages
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // receive a list of messages, hopefully 2 that we want
        std::vector<zmq::message_t *> msgList;
        while( msgList.size() == 0 ) {
            recvMessages( sock, msgList );
            usleep( 100 );
        }
        
        delete( sock );
        
        // Check to see that everythings okay
        if( msgList.size() == 2 && parseTypeMsg(msgList[0]) == ShinyFuse::ACK ) {
            // If it worked, then we win!  Copy the data into stat!
            memcpy( stbuff, msgList[1]->data(), sizeof(struct stat) );
        } else {
            // If it didn't, then print out a warning, and just tell the app that that file doesn't exist!
            //WARN( "failed on [%s]! (%d, %d)", path, msgList.size(), msgList.size() ? parseTypeMsg(msgList[0]) : -1 );
            return -ENOENT;
        }
        
        // free all messages from recvMessages()
        freeMsgList( msgList );
        return 0;
    }
    
    // If we can't connect to the broker, we're in deep
    return -EIO;
}

int ShinyFuse::fuse_readdir(const char *path, void *output, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
    LOG( "readdir [%s]", path );
    
    // First, get a socket to the broker
    zmq::socket_t * sock = getBroker();
    if( sock ) {
        // Build the messages we're going to send
        zmq::message_t typeMsg; buildTypeMsg( ShinyFuse::READDIR, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send those messages
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // receive a list of messages, hopefully 2 that we want
        std::vector<zmq::message_t *> msgList;
        while( msgList.size() == 0 ) {
            recvMessages( sock, msgList );
            usleep( 100 );
        }
        delete( sock );
        
        if( msgList.size() > 1 && parseTypeMsg(msgList[0]) == ShinyFuse::ACK ) {
            filler( output, ".", NULL, 0 );
            filler( output, "..", NULL, 0 );
            
            for( uint64_t i=1; i<msgList.size(); ++i ) {
                char * childName = parseStringMsg(msgList[i]);
                filler( output, childName, NULL, 0 );
                delete( childName );
            }
        } else {
            WARN( "Could not find dir %s", path );
            return -ENOENT;
        }
        return 0;        
    }
    
    // If we can't connect to the broker, we're in deep
    return -EIO;
}

int ShinyFuse::fuse_open( const char *path, struct fuse_file_info *fi ) {
    /*
    LOG( "open [%s]", path );
    
    ShinyMetaNode * node = fs->findNode(path);
    if( node ) {
        LOG( "Check for permissions issues!" );
        //Check if we're asking for any kind of write access
        if( fi->flags & (O_WRONLY | O_RDWR) ) {
            LOG( "Here's where I ask for a write-lock" );
            return 0;
        } else {
            //It has to be either O_WRONLY, O_RDWR or O_RDONLY
            return 0;
        }
    } else
        return -ENOENT;
     */
    return -ENOENT;
}

int ShinyFuse::fuse_read(const char *path, char *buffer, size_t len, off_t offset, struct fuse_file_info * fi ) {
    /*
    LOG( "read [%s]", path );
    
    ShinyMetaFile * node = (ShinyMetaFile *) fs->findNode(path);
    if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
        return (int)node->read( offset, buffer, len );
    } else 
        return -ENOENT;
     */
}

int ShinyFuse::fuse_write( const char * path, const char * buffer, size_t len, off_t offset, struct fuse_file_info * fi ) {
    /*
    LOG( "write [%s]", path );

    ShinyMetaFile * node = (ShinyMetaFile *) fs->findNode(path);
    if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
        return (int)node->write( offset, buffer, len );
    } else 
        return -ENOENT;
     */
}

int ShinyFuse::fuse_truncate(const char * path, off_t len ) {
    /*
    LOG( "truncate [%s]", path );
    
    ShinyMetaFile * node = (ShinyMetaFile *) fs->findNode(path);
    if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
        return (int)node->truncate( len );
    } else 
        return -ENOENT;
     */
}

int ShinyFuse::fuse_create( const char * path, mode_t permissions, struct fuse_file_info * fi ) {
    /*
    LOG( "create [%s]", path );
    if( fs->findNode( path ) )
        return -EEXIST;

    ShinyMetaDir * parent = fs->findParentNode( path );
    if( !parent )
        return -ENOENT;

    uint64_t start = strlen(path);
    while( path[start-1] != '/' )
        start--;
    new ShinyMetaFile( fs, path + start, parent );
     */
    return 0;
}

int ShinyFuse::fuse_mknod( const char *path, mode_t permissions, dev_t device ) {
    LOG( "mknod [%s]", path );
    //Just sub out to create()
    return fuse_create( path, permissions, NULL );
}

int ShinyFuse::fuse_mkdir( const char * path, mode_t permissions ) {
    /*
    LOG( "mkdir [%s]", path );
    if( fs->findNode( path ) )
        return -EEXIST;
    
    ShinyMetaDir * parent = fs->findParentNode( path );
    if( !parent )
        return -ENOENT;
    
    uint32_t start = (uint32_t) strlen(path) - 1;
    while( path[start-1] != '/' )   
        start--;
    new ShinyMetaDir( fs, path + start, parent );
     */
    return 0;
}

int ShinyFuse::fuse_unlink( const char * path ) {
    /*
    LOG( "unlink [%s]", path );
    ShinyMetaNode * node = fs->findNode( path );
    if( node ) {
        delete( node );
    } else
        return -ENOENT;
     */
    return 0;
}

int ShinyFuse::fuse_rmdir( const char * path ) {
    LOG( "rmdir [%s]", path );
    return fuse_unlink( path );
}

int ShinyFuse::fuse_rename( const char * path, const char * newPath ) {
    /*
    LOG( "rename [%s -> %s]", path, newPath );
    ShinyMetaNode * node = fs->findNode( path );
    //Return 404 if it doesn't exist
    if( !node )
        return -ENOENT;

    //Return error if the target already exists
    if( fs->findNode( newPath ) )
        return -EEXIST;

    //Return error if we can't find the dir we're supposed to move to
    ShinyMetaDir * newParent = fs->findParentNode( newPath );
    if( !newParent )
        return -ENOENT;
    
    ShinyMetaDir * parent = (ShinyMetaDir *) fs->findNode( node->getParent() );
    if( !parent ) {
        WARN( "Couldn't find parent of node %s with inode [%llu]", path, node->getParent() );
        return -EIO;
    }
    
    //Move the node over, call it all good
    if( parent != newParent ) {
        parent->delNode( node );
        newParent->addNode( node );
    }
    
    //Change the name
    uint32_t start = (uint32_t) strlen(newPath) - 1;
    while( newPath[start-1] != '/' )
        start--;
    node->setName( newPath + start );
     */
    return 0;
}

int ShinyFuse::fuse_access( const char *path, int mode ) {
    /*
    LOG( "access [%s]", path );
    
    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;
    
    ERROR( "Need to solidify user-control stuffage here!" );
     */
    return 0;
}

int ShinyFuse::fuse_chmod( const char *path, mode_t mode ) {
    /*
    LOG( "chmod [%s]", path );

    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;

    node->setPermissions( mode );
     */
    return 0;
}

int ShinyFuse::fuse_chown( const char *path, uid_t uid, gid_t gid ) {
    /*
    LOG( "chown [%s]", path );

    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;

    ERROR( "Need to finish thinking about user management!!!!" );
     */
    return 0;
}