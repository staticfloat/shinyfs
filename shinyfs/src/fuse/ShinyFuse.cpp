#include "ShinyFuse.h"
#include <base/Logger.h>
#include "../filesystem/ShinyMetaDir.h"
#include "../filesystem/ShinyMetaRootDir.h"
#include "../filesystem/ShinyMetaFile.h"
#include "ShinyFilesystemMediator.h"
#include "../util/zmqutils.h"
#include <sys/errno.h>
#include <stdarg.h>

ShinyFilesystemMediator * ShinyFuse::sfm;
ShinyFilesystem * ShinyFuse::fs;
zmq::context_t * ::ShinyFuse::ctx;

bool ShinyFuse::init( const char * mountPoint ) {
    //First, setup the callbacks
    struct fuse_operations shiny_operations;
    memset( &shiny_operations, NULL, sizeof(shiny_operations) );

    shiny_operations.init = ShinyFuse::fuse_init;
    shiny_operations.destroy = ShinyFuse::fuse_destroy;

    shiny_operations.getattr = ShinyFuse::fuse_getattr;
    shiny_operations.readdir = ShinyFuse::fuse_readdir;
    
    shiny_operations.open = ShinyFuse::fuse_open;
    shiny_operations.release = ShinyFuse::fuse_release;
    shiny_operations.read = ShinyFuse::fuse_read;

    shiny_operations.write = ShinyFuse::fuse_write;
    shiny_operations.truncate = ShinyFuse::fuse_truncate;

    shiny_operations.mknod = ShinyFuse::fuse_mknod;
    shiny_operations.mkdir = ShinyFuse::fuse_mkdir;
    shiny_operations.unlink = ShinyFuse::fuse_unlink;
    shiny_operations.rmdir = ShinyFuse::fuse_rmdir;

    //shiny_operations.access = ShinyFuse::fuse_access;
    shiny_operations.rename = ShinyFuse::fuse_rename;
    shiny_operations.chmod = ShinyFuse::fuse_chmod;
    //shiny_operations.chown = ShinyFuse::fuse_chown;
    
    ctx = new zmq::context_t( 1 );
    //fs = new ShinyFilesystem( "filecache.kct#dfunit=8" );
    fs = new ShinyFilesystem( "filecache" );
    sfm = new ShinyFilesystemMediator( fs, ctx );
    
    // Make sure mount point is viable
    struct stat st;
    if( stat( mountPoint, &st ) != 0 ) {
        WARN( "Couldn't find mount point %s, attempting to create...", mountPoint );
        if( mkdir( mountPoint, S_IRWXU | S_IRWXG | S_IROTH ) != 0 ) {
            ERROR( "Could not create mount point %s!", mountPoint );
            return false;
        }
    }
    
    // Start fuse reactor, now that we've defined all our callbacks
    try {
        char * argv[] = {
            (char *)"./shinyfs",
            (char *)mountPoint,
            (char *)"-f",
//          (char *)"-d",
        };
        fuse_main( sizeof(argv)/sizeof(char *), argv, &shiny_operations, NULL );
    } catch( ... ) {
        ERROR( "fuse_main() died!" );
    }
    return true;
}

void * ShinyFuse::fuse_init( struct fuse_conn_info * conn ) {
    LOG( "init" );
    return NULL;
}

void ShinyFuse::fuse_destroy( void * private_data ) {
    LOG( "destroy" );
    
    // Clean up the mediator first,
    delete( sfm );
    
    // clean up the fs!
    delete( fs );
    
    // Clean up the zmq context too!
    delete( ctx );

    LOG( "saved and sanitycheck'ed!" );
}

int ShinyFuse::fuse_getattr( const char *path, struct stat * stbuff ) {
    //LOG( "getattr: [%s]", path );
    // First, get a socket to the broker
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        // Build the messages we're going to send
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::GETATTR, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send those messages
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // receive a list of messages, hopefully 2 that we want
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        delete( sock );
        
        // Check to see that everythings okay
        if( msgList.size() == 3 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            // If it worked, then we win!  Unserialize!
            ShinyMetaNode::NodeType nodeType = (ShinyMetaNode::NodeType) *((uint8_t *)msgList[1]->data());
            
            // Parse out the node, given the nodeType that was sent
            ShinyMetaNode * node = parseNodeMsg( msgList[2], nodeType );
            
            const char * data = (const char *)msgList[2]->data();
            switch( nodeType ) {
                case ShinyMetaNode::TYPE_FILE:
                    stbuff->st_mode |= S_IFREG | node->getPermissions();
                    stbuff->st_nlink = 1;                    
                    stbuff->st_size = ((ShinyMetaFile *)node)->getLen();
                    break;
                case ShinyMetaNode::TYPE_DIR:
                case ShinyMetaNode::TYPE_ROOTDIR:
                    stbuff->st_mode |= S_IFDIR | node->getPermissions();
                    stbuff->st_nlink = ((ShinyMetaDir *)node)->getNumNodes();
                    break;
                default:
                    WARN( "Couldn't understand the node type of node %s! (%d)", path, nodeType );
                    break;
            }
            stbuff->st_uid = node->getUID();
            stbuff->st_gid = node->getGID();
        } else {
            // If it's not just tell the app that that file doesn't exist!
            return -ENOENT;
        }
        
        // free all messages from recvMessages()
        freeMsgList( msgList );
        return 0;
    }
    
    // If we can't connect to the broker, we're in deep doo-doo
    return -EIO;
}

int ShinyFuse::fuse_readdir(const char *path, void *output, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
    LOG( "readdir: [%s]", path );

    // First, get a socket to the broker
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        // Build the messages we're going to send
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::READDIR, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send those messages
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // receive a list of messages, hopefully 2 that we want
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        delete( sock );
        
        if( msgList.size() >= 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            filler( output, ".", NULL, 0 );
            filler( output, "..", NULL, 0 );
            
            for( uint64_t i=1; i<msgList.size(); ++i ) {
                char * childName = parseStringMsg(msgList[i]);
                filler( output, childName, NULL, 0 );
                delete( childName );
            }
        } else {
            // If it's not just a single NACK, there's a problem! (if it is, the dir just can't be found, no biggie)
            if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || !msgList.size() )
                WARN( "Unknown error in communication!" );
            return -ENOENT;
        }
        return 0;        
    }    
    // If we can't connect to the broker, we're in deep
    return -EIO;
}

int ShinyFuse::fuse_open( const char *path, struct fuse_file_info *fi ) {
    LOG( "open:    [%s]", path );
    
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        // Just send in the path, make sure we get an ACK back (file CREATION does not happen here)
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::OPEN, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send 'em
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for the response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // cleanup!
        delete( sock );
        
        // If it's an ACK, we win!
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
}

int ShinyFuse::fuse_read(const char *path, char *buffer, size_t len, off_t offset, struct fuse_file_info * fi ) {
    LOG( "read:    [%s]", path );
    
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::READREQ, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // ACK, and node waiting to be parsed
        if( msgList.size() == 2 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            // parse out the node
            const char * data = (const char *) msgList[1]->data();
            ShinyMetaFileHandle * fh = new ShinyMetaFileHandle( &data, fs, path );
            
            // Go out to the cache and read!
            uint64_t retval = fh->read( offset, buffer, len );
            
            // send out the READDONE
            buildTypeMsg( ShinyFilesystemMediator::READDONE, &typeMsg );
            buildStringMsg( path, &pathMsg );
            zmq::message_t nodeMsg; buildNodeMsg( fh, &nodeMsg );
            
            // Send
            sendMessages( sock, 3, &typeMsg, &pathMsg, &nodeMsg );
            
            // wait for response?  no need!
            delete( sock );

            // return the number of bytes read!
            return retval;
        }
        
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 )
            WARN( "Unknown error in communication!" );
    }
    return -ENOENT;
}

int ShinyFuse::fuse_write( const char * path, const char * buffer, size_t len, off_t offset, struct fuse_file_info * fi ) {
    LOG( "write:   [%s] [%llu]", path, len );
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::WRITEREQ, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // ACK, and node waiting to be parsed
        if( msgList.size() == 2 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            // parse out the node
            const char * data = (const char *) msgList[1]->data();
            ShinyMetaFileHandle * fh = new ShinyMetaFileHandle( &data, fs, path );
            
            // Go out to the cache and read!
            uint64_t retval = fh->write( offset, buffer, len );
            
            // send out the WRITEDONE
            buildTypeMsg( ShinyFilesystemMediator::WRITEDONE, &typeMsg );
            buildStringMsg( path, &pathMsg );
            zmq::message_t nodeMsg; buildNodeMsg( fh, &nodeMsg );
            
            // Send
            sendMessages( sock, 3, &typeMsg, &pathMsg, &nodeMsg );
            
            // wait for response?  no need!
            delete( sock );
            
            // return the number of bytes written!
            return retval;
        }
        
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 )
            WARN( "Unknown error in communication!" );
    }
    return -ENOENT;
}

int ShinyFuse::fuse_truncate(const char * path, off_t len ) {
    LOG( "truncate [%s]", path );
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::TRUNCREQ, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // ACK, and node waiting to be parsed
        if( msgList.size() == 2 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            // parse out the node
            const char * data = (const char *) msgList[1]->data();
            ShinyMetaFileHandle * fh = new ShinyMetaFileHandle( &data, fs, path );
            
            // Go out to the cache and truncate!
            fh->setLen( len );
            
            // send out the TRUNCDONE
            buildTypeMsg( ShinyFilesystemMediator::TRUNCDONE, &typeMsg );
            buildStringMsg( path, &pathMsg );
            zmq::message_t nodeMsg; buildNodeMsg( fh, &nodeMsg );
            
            // Send
            sendMessages( sock, 3, &typeMsg, &pathMsg, &nodeMsg );
            
            // wait for response?  no need!
            delete( sock );
            
            // return the number of bytes written!
            return 0;
        }
        
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 )
            WARN( "Unknown error in communication!" );
    }
    return -ENOENT;
}


int ShinyFuse::fuse_release(const char *path, struct fuse_file_info *fi) {
    LOG( "close [%s]", path );
    
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::CLOSE, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );

        // cleanup before all the return's
        delete( sock );
        
        // yay, it's an ACK, and we win
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
}

int ShinyFuse::fuse_mknod( const char *path, mode_t mode, dev_t device ) {
    LOG( "mknod [%s]", path );
    
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::CREATEFILE, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        zmq::message_t modeMsg; buildDataMsg( (unsigned char *)&mode, sizeof(mode_t), &modeMsg );
        
        // Send
        sendMessages( sock, 3, &typeMsg, &pathMsg, &modeMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // cleanup before all the return's
        delete( sock );

        // yay, it's an ACK, and we win
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
}

int ShinyFuse::fuse_mkdir( const char * path, mode_t permissions ) {
    LOG( "mkdir [%s]", path );
    
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::CREATEDIR, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // cleanup before all the return's
        delete( sock );
        
        // yay, it's an ACK, and we win
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
}

int ShinyFuse::fuse_unlink( const char * path ) {
    LOG( "unlink [%s]", path );
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::DELETE, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        
        // Send
        sendMessages( sock, 2, &typeMsg, &pathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // cleanup before all the return's
        delete( sock );
        
        // yay, it's an ACK, and we win
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
}

int ShinyFuse::fuse_rmdir( const char * path ) {
    LOG( "rmdir [%s]", path );
    return fuse_unlink( path );
}

int ShinyFuse::fuse_rename( const char * path, const char * newPath ) {
    LOG( "rename [%s -> %s]", path, newPath );
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::RENAME, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        zmq::message_t newPathMsg; buildStringMsg( path, &newPathMsg );
        
        // Send
        sendMessages( sock, 3, &typeMsg, &pathMsg, &newPathMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // cleanup before all the return's
        delete( sock );
        
        // yay, it's an ACK, and we win
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
}

int ShinyFuse::fuse_access( const char *path, int mode ) {
    LOG( "access [%s] [%d]", path, mode );
/*    
    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;
    
    ERROR( "Need to solidify user-control stuffage here!" );
     */
    return 0;
}

int ShinyFuse::fuse_chmod( const char *path, mode_t mode ) {
    LOG( "chmod [%s] [%d]", path, mode );
    
    zmq::socket_t * sock = sfm->getMediator();
    if( sock ) {
        zmq::message_t typeMsg; buildTypeMsg( ShinyFilesystemMediator::CHMOD, &typeMsg );
        zmq::message_t pathMsg; buildStringMsg( path, &pathMsg );
        zmq::message_t modeMsg; buildDataMsg( (unsigned char *)&mode, sizeof(mode_t), &modeMsg );
        
        // Send
        sendMessages( sock, 3, &typeMsg, &pathMsg, &modeMsg );
        
        // wait for response
        std::vector<zmq::message_t *> msgList;
        recvMessages( sock, msgList );
        
        // cleanup before all the return's
        delete( sock );
        
        // yay, it's an ACK, and we win
        if( msgList.size() == 1 && parseTypeMsg(msgList[0]) == ShinyFilesystemMediator::ACK ) {
            return 0;
        }
        
        // Otherwise, if it's not just a single NACK, we're in trouble
        if( (msgList.size() == 1 && parseTypeMsg(msgList[0]) != ShinyFilesystemMediator::NACK) || msgList.size() != 1 ) {
            WARN( "Unknown error in communication!" );
        }
    }
    return -ENOENT;
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