#include "ShinyFilesystemMediator.h"
#include "../util/zmqutils.h"
#include "ShinyFuse.h"
#include "base/Logger.h"
#include "../filesystem/ShinyMetaDir.h"
#include "../filesystem/ShinyMetaFile.h"
#include "../filesystem/ShinyMetaFileHandle.h"
#include <string.h>

// Extreme laziness function to send a NACK to the other side
void sendNACK( zmq::socket_t * sock, zmq::message_t * routing ) {
    // Send back failure, we couldn't find that node!
    zmq::message_t nackMsg;
    buildTypeMsg( ShinyFilesystemMediator::NACK, &nackMsg );
    
    zmq::message_t blankMsg;
    sendMessages(sock, 3, routing, &blankMsg, &nackMsg );
}

// Same thing for an ACK
void sendACK( zmq::socket_t *sock, zmq::message_t * routing ) {
    zmq::message_t ackMsg;
    buildTypeMsg( ShinyFilesystemMediator::ACK, &ackMsg );
    
    zmq::message_t blankMsg;
    sendMessages(sock, 3, routing, &blankMsg, &ackMsg );
}

void * mediatorThreadLoop( void * data ) {
    // Grab this guy from the data
    ShinyFilesystemMediator * sfm = (ShinyFilesystemMediator *) data;
    
    // Our ROUTER socket to deal with all incoming fuse noise
    zmq::socket_t * medSock = new zmq::socket_t( *sfm->ctx, ZMQ_ROUTER );
    medSock->bind( sfm->getZMQEndpointFuse() );
    
    // My list of zmq messages
    std::vector<zmq::message_t *> msgList;
    
    bool keepRunning = true;
    // Keep going as long as we don't get a DESTROY message.  :P
    while( keepRunning ) {
        // Check for messages from fuse threads
        recvMessages( medSock, msgList );
        
        // If we actually received anything
        if( msgList.size() ) {
            if( msgList.size() > 2 ) {
                // Now begins the real work.
                keepRunning = sfm->handleMessage( medSock, msgList );
            } else {
                WARN( "Malformed message to mediator! msgList.size() == %d", msgList.size() );
                for( int i = 0; i < msgList.size(); ++i ) {
                    WARN( "  %d) [%d] %.*s", i, msgList[i]->size(), msgList[i]->size() > 10 ? 10 : msgList[i]->size() );
                }
            }
            
            freeMsgList( msgList );
        }
    }
    
    // Cleanup the socket so that closing the context doesn't wait forever.  :P
    delete( medSock );
    return NULL;   
}



ShinyFilesystemMediator::ShinyFilesystemMediator( ShinyFilesystem * fs, zmq::context_t * ctx ) : fs( fs ), ctx( ctx ) {
    // Start up the mediator thread
    pthread_create( &this->thread, NULL, mediatorThreadLoop, this );
    
    // wait until the socket is available:
    if( !waitForEndpoint( this->ctx, ShinyFilesystemMediator::getZMQEndpointFuse() ) ) {
        throw "Could not create ZMQ endpoint for ShinyFilesystemMediator!";
    }
}

ShinyFilesystemMediator::~ShinyFilesystemMediator() {
    zmq::socket_t * killSock = this->getMediator();
    
    // build a message that says "destroy" and send it
    zmq::message_t destroyMsg; buildTypeMsg( ShinyFilesystemMediator::DESTROY, &destroyMsg );
    sendMessages(killSock, 1, &destroyMsg );
    
    if( pthread_join( this->thread, NULL ) != 0 ) {
        ERROR( "pthread_join() failed on destruction of mediator thread!" );
    }
    
    // Lol, can't believe I forgot this
    delete( killSock );
}


zmq::socket_t * ShinyFilesystemMediator::getMediator() {
    zmq::socket_t * sock = new zmq::socket_t( *this->ctx, ZMQ_REQ );
    try {
        sock->connect( getZMQEndpointFuse() );
    } catch(...) {
        ERROR( "Could not connect to %s! %d: %s", this->getZMQEndpointFuse(), errno, strerror(errno) );
        delete( sock );
        return NULL;
    }

    // purposefully set it as blocking, makes life easier for me with all those threads.
    uint64_t nonblock = 0;
    sock->setsockopt( ZMQ_NOBLOCK, &nonblock, sizeof(uint64_t));
    return sock;
}



bool ShinyFilesystemMediator::handleMessage( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList ) {
    zmq::message_t * fuseRoute = msgList[0];
    zmq::message_t * blankMsg = msgList[1];
    
    // Following the protocol (briefly) laid out in ShinyFuse.h;
    uint8_t type = parseTypeMsg(msgList[2]);
    switch( type ) {
        case ShinyFilesystemMediator::DESTROY: {
            // No actual response data, just sending response just to be polite
            sendACK( sock, fuseRoute );
            // return false as we're signaling to mediator that it's time to die.  >:}
            return false;
        }
        case ShinyFilesystemMediator::GETATTR: {
            // Let's actually get the data fuse wants!
            char * path = parseStringMsg( msgList[3] );
            
            // If the node even exists;
            ShinyMetaNode * node = fs->findNode( path );
            if( node ) {
                // we're just going to serialize it and send it on it's way!
                // Opportunity for zero-copy here!
                uint64_t len = node->serializedLen();
                char * buff = new char[len];
                node->serialize(buff);
                
                zmq::message_t ackMsg; buildTypeMsg( ShinyFilesystemMediator::ACK, &ackMsg );
                zmq::message_t nodeTypeMsg; buildTypeMsg( node->getNodeType(), &nodeTypeMsg );
                zmq::message_t nodeMsg; buildDataMsg( buff, len, &nodeMsg );
                
                sendMessages( sock, 5, fuseRoute, blankMsg, &ackMsg, &nodeTypeMsg, &nodeMsg );
                delete( buff );
            } else
                sendNACK( sock, fuseRoute );
            
            // cleanup after the parseStringMsg()
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::SETATTR: {
            char * path = parseStringMsg( msgList[3] );
            
            ShinyMetaNode * node = fs->findNode( path );
            if( node ) {
                const char * data = (const char *) msgList[4]->data();
                
                // Apply the data to the node
                node->unserialize( &data );
                
                // Send back ACK
                sendACK( sock, fuseRoute );
            } else
                sendNACK( sock, fuseRoute );
            
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::READDIR: {
            char * path = parseStringMsg( msgList[3] );
            
            // If the node even exists;
            ShinyMetaNode * node = fs->findNode( path );
            if( node && (node->getNodeType() == ShinyMetaNode::TYPE_DIR || node->getNodeType() == ShinyMetaNode::TYPE_ROOTDIR) ) {
                const std::vector<ShinyMetaNode *> * children = ((ShinyMetaDir *) node)->getNodes();
                
                // Here is my crucible, to hold data to be pummeled out of the networking autocannon, ZMQ
                std::vector<zmq::message_t *> list( 1 + 1 + 1 + children->size() );
                list[0] = fuseRoute;
                list[1] = blankMsg;
                
                zmq::message_t ackMsg; buildTypeMsg( ShinyFilesystemMediator::ACK, &ackMsg );
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
        case ShinyFilesystemMediator::OPEN: {
            // Grab the path, shove it into an std::string for searching openFiles
            char * path = parseStringMsg( msgList[3] );
            std::string pathStr( path );
            
            // This is our file that we'll eventually send back, or not, if we can't find the file
            ShinyMetaNode * node = NULL;
            
            // First, make sure that there is not already an OpenFileInfo corresponding to this path:
            std::map<std::string, OpenFileInfo *>::iterator itty = this->openFiles.find( pathStr );
            
            // If there isn't, let's get one! (if it exists)
            if( itty == this->openFiles.end() ) {
                node = fs->findNode( path );
                if( node && node->getNodeType() == ShinyMetaNode::TYPE_FILE ) {
                    // Create the new OpenFileInfo, initialize it to 1 opens, so only 1 close required to
                    // flush this data out of the map
                    OpenFileInfo * ofi = new OpenFileInfo();
                    ofi->file = (ShinyMetaFile *) node;
                    ofi->opens = 1;
                    
                    // Aaaand, put it into the list!
                    this->openFiles[pathStr] = ofi;
                }
            } else {
                // Check to make sure this guy isn't on death row 'cause of an unlink()
                if( !(*itty).second->shouldDelete ) {
                    // Otherwise, it's in the list, so let's return the cached copy!
                    node = (ShinyMetaNode *) (*itty).second->file;
                    
                    // Increment the number of times this file has been opened...
                    (*itty).second->opens++;
                    
                    // If it was going to be closed, let's stop that from happening now
                    (*itty).second->shouldClose = false;
                }
            }
            
            // If we were indeed able to find the file; write back an ACK, otherwise, NACK it up!
            if( node )
                sendACK( sock, fuseRoute );
            else
                sendNACK( sock, fuseRoute );
            
            // Don't forget this too!
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::CLOSE: {
            // Grab the path, just like in open
            char * path = parseStringMsg( msgList[3] );
            std::string pathStr( path );

            // This time, we _only_ check openFiles
            std::map<std::string, OpenFileInfo *>::iterator itty = this->openFiles.find( pathStr );
            
            // If it's there,
            if( itty != this->openFiles.end() ) {
                OpenFileInfo * ofi = (*itty).second;
                
                // decrement the opens!
                ofi->opens--;
                
                // Should we purge this ofi from the cache?
                if( ofi->opens == 0 ) {
                    // If we are currently write locked, or we have reads ongoing, we must
                    // wait 'till those are exuahsted, and so we will just mark ourselves as suicidal
                    if( ofi->writeLocked || ofi->reads > 0 ) {
                        // This will cause it to be closed once all READs and WRITEs are finished
                        ofi->shouldClose = true;
                    } else
                        this->closeOFI( itty );
                }
                
                // Aaaand, send an ACK, just for fun
                sendACK( sock, fuseRoute );
            } else {
                // NACK!  NACK I SAY!
                sendNACK( sock, fuseRoute );
            }
            
            // You just gotta free it up, fre-fre-free it all up now, 'come on!
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::READREQ:
        case ShinyFilesystemMediator::WRITEREQ:
        case ShinyFilesystemMediator::TRUNCREQ: {
            // Grab the path, and the itty
            char * path = parseStringMsg( msgList[3] );
            std::string pathStr( path );
            std::map<std::string, OpenFileInfo *>::iterator itty = this->openFiles.find( pathStr );
            
            // If it is in openFiles,
            if( itty != this->openFiles.end() ) {
                OpenFileInfo * ofi = (*itty).second;
                
                // first, we put it into our queue of file operations,
                zmq::message_t * savedRoute = new zmq::message_t();
                savedRoute->copy( fuseRoute );
                
                // Queue this request for later
                ofi->queuedFileOperations.push_back( QueuedFO( savedRoute, type ) );
                
                // Then, we try to start up more file operations. the subroutine will check to see
                // if we actually can or not.  It's amazing!  Magical, even.
                this->startQueuedFO( sock, ofi );
            } else
                sendNACK( sock, fuseRoute );
            
            // darn all these paths.  :P
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::READDONE:
        case ShinyFilesystemMediator::WRITEDONE:
        case ShinyFilesystemMediator::TRUNCDONE: {
            // Grab the path, and the itty
            char * path = parseStringMsg( msgList[3] );
            std::string pathStr( path );
            std::map<std::string, OpenFileInfo *>::iterator itty = this->openFiles.find( pathStr );

            // If it is in openFiles,
            if( itty != this->openFiles.end() ) {
                OpenFileInfo * ofi = (*itty).second;
                
                if( type == ShinyFilesystemMediator::WRITEDONE || type == ShinyFilesystemMediator::TRUNCDONE ) {
                    // We're not writelocked!  (at least, util we start writing again. XD)
                    ofi->writeLocked = false;
                } else if( type == ShinyFilesystemMediator::READDONE ) {
                    // Decrease the number of concurrently running READS!
                    ofi->reads--;
                }
                
                // Update the file with the serialized version sent back
                //LOG( "Updating %s due to a %s", path, (type == READDONE ? "READDONE" : (type == WRITEDONE ? "WRITEDONE" : "TRUNCDONE")) );
                const char * data = (const char *) msgList[4]->data();
                ofi->file->unserialize(&data);
                //delete( fs );
                
                if( !ofi->writeLocked || ofi->reads == 0 ) {
                    // Check to see if there's stuff queued, and if the conditions are right, start that queued stuff!
                    if( !ofi->queuedFileOperations.empty() )
                        this->startQueuedFO( sock, ofi );
                    else {
                        // If there is nothing queued, and we should close this file, CLOSE IT!
                        if( ofi->shouldClose )
                            this->closeOFI( itty );
                    }
                }
            }
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::CREATEFILE:
        case ShinyFilesystemMediator::CREATEDIR: {
            // Grab the path,
            char * path = parseStringMsg( msgList[3] );
            
            // See if the file already exists
            ShinyMetaNode * node = fs->findNode( path );
            if( node ) {
                // If it does, I can't very well create a file here, now can I?
                sendNACK( sock, fuseRoute );
            } else {
                // If not, check that the parent exists;
                ShinyMetaDir * parent = fs->findParentNode(path);
                
                if( !parent ) {
                    // If it doesn't, send a NACK!
                    sendNACK( sock, fuseRoute );
                } else {
                    // Otherwise, let's create the dir/file
                    if( type == ShinyFilesystemMediator::CREATEFILE )
                        node = new ShinyMetaFile( ShinyMetaNode::basename( path ), parent );
                    else
                        node = new ShinyMetaDir( ShinyMetaNode::basename( path), parent );
                    
                    // If they have included it, set the permissions away from the defaults
                    if( msgList.size() > 4 ) {
                        uint16_t mode = *((uint16_t *) parseDataMsg( msgList[4] ));
                        node->setPermissions( mode );
                    }
                    
                    // And send back an ACK
                    sendACK( sock, fuseRoute );
                }
            }
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::DELETE: {
            // Grab the path,
            char * path = parseStringMsg( msgList[3] );
            
            // Check to make sure the file exists
            ShinyMetaNode * node = fs->findNode( path );
            if( !node ) {
                // If it doesn'y, I can't very well delete it, can I?
                sendNACK( sock, fuseRoute );
            } else {
                // Since it exists, let's make sure it's not open right now
                std::string pathStr( path );
                std::map<std::string, OpenFileInfo *>::iterator itty = this->openFiles.find( pathStr );
                
                // If it is open, queue the deletion for later
                if( itty != this->openFiles.end() ) {
                    OpenFileInfo * ofi = (*itty).second;
                    ofi->shouldDelete = true;
                } else {
                    // Tell the db to delete him, if it's a file
                    if( node->getNodeType() == ShinyMetaNode::TYPE_FILE )
                        ((ShinyMetaFile *)node)->setLen( 0 );
                    
                    // actually delete the sucker
                    delete( node );
                }
            
                // AFFLACK.  AFFFFFLAACK.
                sendACK( sock, fuseRoute );
            }
            delete( path );
            break;
        }
        case ShinyFilesystemMediator::RENAME: {
            // Grab the path, and the new path
            char * path = parseStringMsg( msgList[3] );
            char * newPath = parseStringMsg( msgList[4] );
            
            // Check that their parents actually exist
            ShinyMetaDir * oldParent = fs->findParentNode( path );
            ShinyMetaDir * newParent = fs->findParentNode( newPath );
            
            if( oldParent && newParent ) {
                // Now that we know the parents are real, find the child
                const char * oldName = ShinyMetaNode::basename( path );
                const char * newName = ShinyMetaNode::basename( newPath );
                ShinyMetaNode * node = oldParent->findNode( oldName );
                
                if( node ) {
                    // Check to make sure we need to move it at all
                    if( oldParent != newParent ) {
                        oldParent->delNode( node );
                        newParent->addNode( node );
                    }
                    
                    // Don't setName to the same thing we had before, lol
                    if( strcmp( oldName, newName) != 0 )
                        node->setName( newName );
                    
                    // Send an ACK, for a job well done
                    sendACK( sock, fuseRoute );
                } else {
                    // We cannae faind tha node cap'n!
                    sendNACK( sock, fuseRoute );
                }

            } else {
                // Oh noes, we couldn't find oldParent or we couldn't find newParent!
                sendNACK( sock, fuseRoute );
            }
            
            delete( path );
            delete( newPath );
            break;
        }
        case ShinyFilesystemMediator::CHMOD: {
            // Grab the path, and the new path
            char * path = parseStringMsg( msgList[3] );
            uint16_t mode = *((uint16_t *) parseDataMsg( msgList[4] ));
            
            // Find node
            ShinyMetaNode * node = fs->findNode( path );
            if( node ) {
                // Set the permissionse
                node->setPermissions( mode );
                
                // ACK
                sendACK( sock, fuseRoute );
            } else
                sendNACK( sock, fuseRoute );

            delete( path );
            break;
        }
        default: {
            WARN( "Unknown ShinyFuse message type! (%d) Sending NACK:", type );
            sendNACK( sock, fuseRoute );
            break;
        }
    }
    
    return true;
}

// Utility function to send an ACK and a node, routed to fuseRoute
void ShinyFilesystemMediator::sendACK_Node( zmq::socket_t *sock, zmq::message_t *fuseRoute, ShinyMetaNode * node ) {
    zmq::message_t blankMsg;
    zmq::message_t ackMsg; buildTypeMsg( ShinyFilesystemMediator::ACK, &ackMsg );
    zmq::message_t nodeMsg; buildNodeMsg( node, &nodeMsg );

    sendMessages( sock, 4, fuseRoute, &blankMsg, &ackMsg, &nodeMsg );
}

void ShinyFilesystemMediator::startQueuedFO( zmq::socket_t *sock, OpenFileInfo *ofi ) {
    std::list<QueuedFO>::iterator itty = ofi->queuedFileOperations.begin();
    
    // Check to make sure that there's actually stuff queued
    if( itty != ofi->queuedFileOperations.end() ) {
        // If the first one is a WRITE or TRUNC, then that's that, we can only start that WRITE or TRUNC
        // Also make sure that we _can_ start one of these, 
        if( ((*itty).second == ShinyFilesystemMediator::WRITEREQ ||
             (*itty).second == ShinyFilesystemMediator::TRUNCREQ) &&
             (!ofi->writeLocked && ofi->reads == 0) ) {
            // Send out the ACK for that thread that has been so patiently waiting.....
            this->sendACK_Node( sock, (*itty).first, ofi->file );
            
            // We are writelocked again.  :P
            ofi->writeLocked = true;
            
            // Delete the fuseRoute
            delete( (*itty).first );
            
            // remove from the list
            ofi->queuedFileOperations.pop_front();
        } else {
            // Otherwise, if it is a READ, send out as many reads as possible
            while( itty != ofi->queuedFileOperations.end() && (*itty).second == ShinyFilesystemMediator::READREQ ) {
                // Send out the ACK for that thread that has been so patiently waiting.....
                this->sendACK_Node( sock, (*itty).first, ofi->file );
                
                // cleanup route
                delete( (*itty).first );
                
                // Increment the number of concurrent reads happening right now
                ofi->reads++;
                
                // Pop this guy, and then move on to the next
                ofi->queuedFileOperations.pop_front();
                itty++;
            }
        }
    }
}

void ShinyFilesystemMediator::closeOFI( std::map<std::string, OpenFileInfo *>::iterator itty ) {
    OpenFileInfo * ofi = (*itty).second;
    
    // remove it from the map of open files
    this->openFiles.erase( itty );
    
    // If we should delete the file, because an unlink() was called against it
    // while some other process had it open....
    if( ofi->shouldDelete ) {
        ofi->file->setLen( 0 );
        delete( ofi->file );
    }
    
    // purge the heretic! (Also the OpenFileInfo struct)
    delete( ofi );
}

const char * ShinyFilesystemMediator::getZMQEndpointFuse() {
    return "inproc://mediator.fuse";
}
