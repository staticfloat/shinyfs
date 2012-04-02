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
ShinyFilesystem::ShinyFilesystem( const char * filecache ) {
    // Check if the filecache exists
/*    struct stat st;
    if( stat( filecache, &st ) != 0 ) {
        WARN( "filecache %s does not exist, creating a new one!", filecache );
        mkdir( filecache, 0700 );
    }*/
    
    /*
    // create socket for comms with shinycache
    cacheSock = new zmq::socket_t( *this->ctx, ZMQ_DEALER );
    cacheSock->bind( this->getZMQEndpointFileCache() );
    
    // Initialize cache object
    cache = new ShinyCache( filecache, ctx );
     */
    
    if( !this->db.open( filecache, kyotocabinet::PolyDB::OWRITER | kyotocabinet::PolyDB::OCREATE ) ) {
        ERROR( "Unable to open filecache in %s", filecache );
        throw "Unable to open filcache";
    }

    
    TODO( "Replace this with unserializing from the db, if we can!" );
    this->root = new ShinyMetaRootDir( this );
    ShinyMetaFile * testf = new ShinyMetaFile( "test" );
    this->root->addNode( testf );
    
    const char * testdata = "this is a test\nawwwwww yeahhhhh\n";
    testf->write(0, testdata, strlen(testdata) );

    
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
    // Close the DB object
    this->db.close();
    
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

const char * ShinyFilesystem::getNodePath( ShinyMetaNode *node ) {
    // If we've cached the result from a previous call, then just return that!
    std::tr1::unordered_map<ShinyMetaNode *, const char *>::iterator itty = this->nodePaths.find( node );
    if( itty != this->nodePaths.end() ) {
        return (*itty).second;
    }
    
    // First, get the length:
    uint64_t len = 0;
    
    // We'll store all the paths in here so we don't have to traverse the tree multiple times
    std::list<const char *> paths;
    paths.push_front( node->getName() );
    len += strlen( node->getName() );
    
    // Now iterate up the tree, gathering the names of parents and shoving them into the list of paths
    ShinyMetaNode * currNode = node;
    while( currNode != (ShinyMetaNode *) this->root ) {
        // Move up in the chain of parents
        ShinyMetaDir * nextNode = currNode->getParent();

        // If our parental chain is broken, just return ?/name
        if( !nextNode ) {
            ERROR( "Parental chain for %s is broken at %s!", paths.back(), currNode->getName() );
            paths.push_front( "?" );
            break;
        }
        
        else if( nextNode != (ShinyMetaNode *)this->root ) {
            // Push node parent's path onto the list, and add its length to len
            paths.push_front( nextNode->getName() );
            len += strlen( nextNode->getName() );
        }
        currNode = nextNode;
    }
    
    // Add 1 for each slash in front of each path
    len += paths.size();
    
    // Add another 1 for the NULL character
    char * path = new char[len+1];
    path[len] = NULL;
    
    // Write out each element of [paths] preceeded by forward slashes
    len = 0;
    for( std::list<const char *>::iterator itty = paths.begin(); itty != paths.end(); ++itty ) {
        path[len++] = '/';
        strcpy( path + len, *itty );
        len += strlen( *itty );
    }
    
    // Save node result into our cached nodePaths map and return it;
    return this->nodePaths[node] = path;
}

/*
const char * ShinyFilesystem::getZMQEndpointFileCache( void ) {
    return "inproc://shinyfs.filecache";
}

zmq::context_t * ShinyFilesystem::getZMQContext( void ) {
    return this->ctx;
}*/

/*
ShinyMetaNode * ShinyFilesystem::findNode( inode_t inode ) {
    std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = this->nodes.find( inode );
    if( itty != this->nodes.end() )
        return (*itty).second;
    return NULL;
}*/

kyotocabinet::PolyDB * ShinyFilesystem::getDB() {
    return &this->db;
}

const char * ShinyFilesystem::getShinyFilesystemDBKey() {
    return "shinyfs.state";
}

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


uint64_t getTotalSerializedLen( ShinyMetaNode * start, bool recursive ) {
    // NodeType + length of actual node
    uint64_t len = sizeof(uint8_t) + start->serializedLen();
    
    if( start->getNodeType() == ShinyMetaNode::TYPE_DIR || start->getNodeType() == ShinyMetaNode::TYPE_ROOTDIR ) {
        // number of children following this brother
        len += sizeof(uint64_t);
        
        const std::vector<ShinyMetaNode *> * startNodes = ((ShinyMetaDir *)start)->getNodes();
        for( uint64_t i=0; i<startNodes->size(); ++i ) {
            //Don't have to check for TYPE_ROOTDIR here, because that's an impossibility!  yay!
            if( recursive && (*startNodes)[i]->getNodeType() == ShinyMetaNode::TYPE_DIR )
                len += getTotalSerializedLen( (*startNodes)[i], recursive );
            else {
                // NodeType + length of actual node, just like we did for start
                len += sizeof(uint8_t) + (*startNodes)[i]->serializedLen();
            }
        }
    }
    return len;
}


// Very similar in form to the above. Thus the liberal copypasta.
// returns output, shifted by total serialized length, so just use [output - totalLen]
char * serializeTree( ShinyMetaNode * start, bool recursive, char * output ) {
    // write out start first
    *((uint8_t *)output) = start->getNodeType();
    output += sizeof(uint8_t);

    start->serialize(output);
    output += start->serializedLen();
    
    // Next, check if we're a dir, and if so, do the serialization dance!
    if( start->getNodeType() == ShinyMetaNode::TYPE_DIR || start->getNodeType() == ShinyMetaNode::TYPE_ROOTDIR ) {
        // Write out the number of children
        *((uint64_t *)output) = ((ShinyMetaDir *)start)->getNumNodes();
        output += sizeof(uint64_t);
        
        //Write out all children
        const std::vector<ShinyMetaNode *> * startNodes = ((ShinyMetaDir *)start)->getNodes();
        for( uint64_t i=0; i<startNodes->size(); ++i ) {
            //Don't have to check for TYPE_ROOTDIR here, because that's an impossibility!  yay!
            if( recursive && (*startNodes)[i]->getNodeType() == ShinyMetaNode::TYPE_DIR ) {
                output = serializeTree( (*startNodes)[i], recursive, output );
            } else {
                output = (*startNodes)[i]->serialize(output);
            }
        }
    }
    return output;
}

uint64_t ShinyFilesystem::serialize( char ** store, ShinyMetaNode * start, bool recursive ) {
    // First, we're going to find the total length of this serialized monstrosity
    // We'll start with the itty bitty overhead of the version number
    uint64_t len = sizeof(uint16_t);

    // default to root (darn you C++, not allowing me to set a default value of this->root!)
    if( !start )
        start = this->root;
    
    // To find total length, walk the walk and talk the talk
    len = getTotalSerializedLen( start, recursive );
    
    // Now, reserve buffer space.  This is such an anti-climactic line, it makes me kind of sad.
    char * output = new char[len];
    
    // make sure that when we shove stuff into output, the user gets it in their variable
    // I create the "output" variable just so that I'm not doing 1253266246x pointer dereferences,
    // also so that the output is unaffected by the crazy shifting that goes on during the serialization
    *store = output;
    
    // First, write out the version number
    *((uint16_t *)output) = this->getVersion();
    output += sizeof(uint16_t);
    
    // Next, we'll iterate through all the nodes we're going to serialize, doing them one at a time;
    output = serializeTree( start, recursive, output );
    
    // and finally, return the length!
    return len;
}


ShinyMetaNode * ShinyFilesystem::unserializeTree( const char *input ) {
    uint8_t type = *((uint8_t *)input);
    input += sizeof(uint8_t);
    
    switch( type ) {
        case ShinyMetaNode::TYPE_ROOTDIR:
        case ShinyMetaNode::TYPE_DIR:
        {
            // First, get the (root)dir itself. In other news, WHY THE HECK DO I WRITE THINGS LIKE THIS?!
            ShinyMetaDir * newDir = (type == ShinyMetaNode::TYPE_ROOTDIR) ? new ShinyMetaRootDir( &input ) : new ShinyMetaDir( &input );
            
            // next, get the number of children that have been serialized
            uint64_t numNodes = *((uint64_t *)input);
            input += sizeof(uint64_t);
            
            // Now, iterating over all children of this dir, LOAD 'EM IN!
            for( uint64_t i=0; i<numNodes; ++i ) {
                // The cycle continues...... we pass our troubles onto our own children
                ShinyMetaNode * childNode = unserializeTree( input );
                
                // Add that newly-formed subtree (could be just a file!) to this guy's collection of children
                newDir->addNode( childNode );
            }
            return newDir;
        }
        case ShinyMetaNode::TYPE_FILE:
        {
            ShinyMetaFile * newFile = new ShinyMetaFile( &input );
            return newFile;
        }
        default:
            WARN( "Unknown node type (%d)!", type );
            break;
    }
    // When in doubt, return NULL!
    return NULL;
}

ShinyMetaNode * ShinyFilesystem::unserialize( const char *input ) {
    // First, a version check
    if( *((uint16_t *)input) != this->getVersion() ) {
        ERROR( "Serialized filesystem objects are of version %d, whereas we are compatible with version(s) %d!", *((uint16_t *)input), this->getVersion() );
        return NULL;
    }
    // now gracefully scoot past that short
    input += sizeof(uint16_t);
    
    // If a problem is too hard for you, push it off to another function!
    return unserializeTree( input );
}

void ShinyFilesystem::save() {
    // First, serialize everything out
    char * output;
    uint64_t len = this->serialize( &output );
    
    // Send this to the filecache to write out
    this->db.set( this->getShinyFilesystemDBKey(), strlen(this->getShinyFilesystemDBKey()), output, len );
    
    delete( output );
}


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