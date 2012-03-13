#include "ShinyFilesystem.h"
#include "ShinyMetaFile.h"
#include "ShinyMetaDir.h"
#include "ShinyMetaRootDir.h"
#include <base/Logger.h>

//Used to stat() to tell if the directory exists
#include <sys/stat.h>
#include <sys/fcntl.h>

/*
 ShinyFilesystem constructor, takes in path to cache location? I need to split this out into a separate cache object.....
 */
ShinyFilesystem::ShinyFilesystem( const char * filecache, zmq::context_t * ctx ) : ctx(ctx) {
    // Check if the filecache exists
    struct stat st;
    if( stat( filecache, &st ) != 0 ) {
        WARN( "filecache %s does not exist, creating a new one!", filecache );
        mkdir( filecache, 0700 );
    }
    
    // create socket for comms with shinycache
    cacheSock = new zmq::socket_t( *this->ctx, ZMQ_DEALER );
    cacheSock->bind( this->getZMQEndpointFileCache() );
    
    // Initialize cache object
    cache = new ShinyCache( filecache, ctx );
    
    
    
    TODO( "Replace this with unserializing the previous stuff!" );
    this->root = new ShinyMetaRootDir( this );
    this->root->addNode( new ShinyMetaFile( this, "test" ) );    
    
/*
    //serializedData would be nonzero in this case if we're constructing from net-based data...
    if( !serializedData ) {
        //Let's look and see if an fscache exists...
        char * fscache = new char[strlen(filecache)+1+8+1];
        sprintf( fscache, "%s/%s", this->filecache, FSCACHE_POSTFIX );
        if( stat( fscache, &st ) != 0 ) {
            WARN( "fscache %s does not exist, starting over....", fscache );
            root = new ShinyMetaRootDir(this);
        } else {
            int fd = open( fscache, O_RDONLY );
            uint64_t len = lseek(fd, 0, SEEK_END);
            lseek(fd,0,SEEK_SET);
            
            serializedData = new char[len];
            read(fd, (void *)serializedData, len);
            
            close(fd);
            
            unserialize( serializedData );
            delete( serializedData );
        }
        delete( fscache );
    } else
        unserialize( serializedData );
*/
    TODO( "Eventually, add back in the ability to unserialize a ShinyFS" );
}

ShinyFilesystem::~ShinyFilesystem() {
    // Remember, kill all sockets, otherwise zmq_term will just block!  D:
    delete( cacheSock );
    
    //Clear out the nodes (amazing how they just take care of themselves, so nicely and all!)
    delete( this->root );
}

//Searches a ShinyMetaDir's listing for a name, returning the child
ShinyMetaNode * ShinyFilesystem::findMatchingChild( ShinyMetaDir * parent, const char * childName, uint64_t childNameLen ) {
    const std::vector<ShinyMetaNode *> list = *parent->getNodes();
    for( uint64_t i = 0; i < list.size(); ++i ) {
        // Compare names
        if( strlen(list[i]->getName()) == childNameLen && memcmp( list[i]->getName(), childName, childNameLen ) == 0 ) {
            // If it works, return this index we iterated over
            return list[i];
        }
    }
    //If we made it all the way through without finding a match for that file, quit out
    return NULL;
}

ShinyMetaNode * ShinyFilesystem::findNode( const char * path ) {
    if( path[0] != '/' ) {
        WARN( "path %s is unacceptable, must be an absolute path!", path );
        return NULL;
    }
    
    ShinyMetaNode * currNode = (ShinyMetaNode *)this->root;
    unsigned long filenameBegin = 1;
    for( unsigned int i=1; i<strlen(path); ++i ) {
        if( path[i] == '/' ) {
            //If this one actually _is_ a directory, let's get its listing
            if( currNode->getNodeType() == ShinyMetaNode::TYPE_DIR || currNode->getNodeType() == ShinyMetaNode::TYPE_ROOTDIR ) {
                //Search currNode's children for a name match
                ShinyMetaNode * childNode = findMatchingChild( (ShinyMetaDir *)currNode, &path[filenameBegin], i - filenameBegin );
                if( !childNode )
                    return NULL;
                else {
                    currNode = childNode;
                    filenameBegin = i+1;
                }
            } else
                return NULL;
            
            //Update the beginning of the next filename to be the character after this slash
            filenameBegin = i+1;
        }
    }
    if( filenameBegin < strlen(path) ) {
        ShinyMetaNode * childNode = findMatchingChild( (ShinyMetaDir *)currNode, &path[filenameBegin], strlen(path) - filenameBegin );
        if( !childNode )
            return NULL;
        currNode = childNode;
    }
    return currNode;
}


ShinyMetaDir * ShinyFilesystem::findParentNode( const char *path ) {
    //Start at the end of the string
    uint64_t len = strlen( path );
    uint64_t end = len-1;
    
    //Move back another char if the last one is actually a slash
    if( path[len-1] == '/' )
        end--;
    
    //Move back until we get _another_ slash
    while( end > 1 && path[end] != '/' )
        end--;
    
    //Copy over until the end to get just the parent path
    char * newPath = new char[end + 1];
    memcpy( newPath, path, end );
    newPath[end] = 0;
    
    //Find it and return
    ShinyMetaDir * ret = (ShinyMetaDir *)this->findNode( newPath );
    delete( newPath );
    return ret;
}

const char * ShinyFilesystem::getZMQEndpointFileCache( void ) {
    return "inproc://shinyfs.filecache";
}

/*
ShinyMetaNode * ShinyFilesystem::findNode( inode_t inode ) {
    std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = this->nodes.find( inode );
    if( itty != this->nodes.end() )
        return (*itty).second;
    return NULL;
}*/



bool ShinyFilesystem::sanityCheck( void ) {
    bool retVal = true;
    //Call sanity check on all of them.
    //The nodes will return false if there is an error
    
    /*
    for( std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = this->nodes.begin(); itty != this->nodes.end(); ++itty ) {
        if( (*itty).second ) {
            if( !(*itty).second->sanityCheck() )
                retVal = false;
        } else {
            WARN( "inode [%d] is not in the nodes map!" );
            this->nodes.erase( itty++ );
        }
    }
     */
    return retVal;
}

size_t ShinyFilesystem::serialize( char ** store ) {
    //First, we're going to find the total length of this serialized monstrosity
    //We'll start with the version number
    /*
    size_t len = sizeof(uint16_t);
    
    //Next, what ShinyFilesystem takes up itself
    //       nextInode        numNodes
    len += sizeof(inode_t) + sizeof(uint64_t);
    
    for( std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = nodes.begin(); itty != nodes.end(); ++itty ) {
        //Add an extra uint8_t for the node type
        len += (*itty).second->serializedLen() + sizeof(uint8_t);
    }
    
    //Now, reserve buffer space
    char * output = new char[len];
    *store = output;
    
    //Now, actually write into that buffer space
    //First, version number
    *((uint16_t *)output) = this->getVersion();
    output += sizeof(uint16_t);
    
    //Next, nextInode, followed by the number of nodes we're writing out
    *((inode_t *)output) = this->nextInode;
    output += sizeof(inode_t);
    
    *((size_t *)output) = nodes.size();
    output += sizeof(size_t);

    //Iterate through all nodes, writing them out
    for( std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = nodes.begin(); itty != nodes.end(); ++itty ) {
        //We're adding this in output here, because it's something the filesystem needs to be aware of,
        //not something the node should handle itself
        *((uint8_t *)output) = (*itty).second->getNodeType();
        output += sizeof(uint8_t);
        
        //Now actually serialize the node
        (*itty).second->serialize( output );
        output += (*itty).second->serializedLen();
    }
    
    //this->dirty = false;
    return len;
     */
}

void ShinyFilesystem::unserialize(const char *input) {
    /*
    if( !nodes.empty() ) {
        ERROR( "We're trying to unserialize when we already have %llu nodes!", nodes.size() );
        throw "Stuff was in nodes!";
    }
    
    if( *((uint16_t *)input) != this->getVersion() ) {
        WARN( "Warning:  Serialized filesystem is of version %d, whereas we are running version %d!", *((uint16_t *)input), this->getVersion() );
    }
    input += sizeof(uint16_t);
    
    LOG( "I should be able to infer this at load time" );
    this->nextInode = *((inode_t *)input);
    input += sizeof(inode_t);
    
    size_t numInodes = *((size_t *)input);
    input += sizeof(size_t);
    
    for( size_t i=0; i<numInodes; ++i ) {
        //First, read in the uint8_t of type information;
        uint8_t type = *((uint8_t *)input);
        input += sizeof(uint8_t);
        
        switch( type ) {
            case SHINY_NODE_TYPE_DIR: {
                ShinyMetaDir * newNode = new ShinyMetaDir( input, this );
                input += newNode->serializedLen();
                break; }
            case SHINY_NODE_TYPE_FILE: {
                ShinyMetaFile * newNode = new ShinyMetaFile( input, this );
                input += newNode->serializedLen();
                break; }
            case SHINY_NODE_TYPE_ROOTDIR: {
                ShinyMetaRootDir * newNode = new ShinyMetaRootDir( input, this );
                input += newNode->serializedLen();
                this->root = newNode;
                break; }
            default:
                WARN( "Stream sync error!  unknown node type %d!", type );
                break;
        }
    }
     */
}

void ShinyFilesystem::save() {
    // First, serialize everything out
    char * output;
    size_t len = this->serialize( &output );
    
    // Send this to the filecache to write out
    TODO( "Change this to \"serializeAndSave()\" instead of \"flush()\"" );
    TODO( "send this output out to be written out to the cache object!" );

    /*
    //Open it for writing, write it, and close
    char * fscache = new char[strlen(filecache)+1+8+1];
    sprintf( fscache, "%s/%s", this->filecache, FSCACHE_POSTFIX );

    int fd = open( fscache, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG | S_IROTH );
    if( !fd ) {
        ERROR( "Could not flush serialized output to %s!", fscache );
    } else {
        write( fd, output, len );
        close( fd );
    }
    delete( fscache );
     */
}


/*
inode_t ShinyFilesystem::genNewInode() {
    TODO( "Get rid of inodes, just use ShinyNode* things, and when (un)serializing generate the indoes and stuff on the fly, and then never use them outside of the disk" );
    inode_t probeNext = this->nextInode + 1;
    //Just linearly probe for the next open inode
    while( this->nodes.find(probeNext) != this->nodes.end() ) {
        ++probeNext;
    }
    inode_t toReturn = this->nextInode;
    this->nextInode = probeNext;
    return toReturn;
}*/


void ShinyFilesystem::printDir( ShinyMetaDir * dir, const char * prefix ) {
    //prefix contains the current dir's name
    LOG( "%s/\n", prefix );
    
    //Iterate over all children
    const std::vector<ShinyMetaNode *> nodes = *dir->getNodes();
    for( uint64_t i=0; i<nodes.size(); ++i ) {
        const char * childName = nodes[i]->getName();
        char * newPrefix = new char[strlen(prefix) + 2 + strlen(childName) + 1];
        sprintf( newPrefix, "%s/%s", prefix, childName );
        if( nodes[i]->getNodeType() == ShinyMetaNode::TYPE_DIR ) {
            this->printDir( (ShinyMetaDir *)nodes[i], newPrefix );
        } else {
            //Only print it out here if it's not a directory, because directories print themselves
            LOG( "%s\n", newPrefix );
        }
        delete( newPrefix );
    }
}

void ShinyFilesystem::print( void ) {
    printDir( (ShinyMetaDir *)root, "" );
}

const uint64_t ShinyFilesystem::getVersion() {
    return SHINYFS_VERSION;
}