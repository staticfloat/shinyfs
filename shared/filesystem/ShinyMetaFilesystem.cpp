#include "ShinyMetaFilesystem.h"
#include "ShinyMetaFile.h"
#include "ShinyMetaDir.h"
#include "ShinyMetaRootDir.h"
#include <base/Logger.h>

ShinyMetaFilesystem::ShinyMetaFilesystem( const char * serializedData ) : nextInode(1), root(NULL) {
    if( serializedData && serializedData[0] ) {
        unserialize( serializedData );
    } else {
        LOG( "serializedData empty, so just starting over...." );
        root = new ShinyMetaRootDir(this);
    }
}

ShinyMetaFilesystem::~ShinyMetaFilesystem() {
    //Clear out the nodes
    while( !this->nodes.empty() ) {
        delete( this->nodes.begin()->second );
        nodes.erase(this->nodes.begin());
    }
}

#define min( x, y ) ((x) < (y) ? (x) : (y))
//Searches a ShinyMetaDir's listing for a name, returning the child
ShinyMetaNode * ShinyMetaFilesystem::findMatchingChild( ShinyMetaDir * parent, const char * childName, uint64_t childNameLen ) {
    const std::list<inode_t> * list = parent->getListing();
    for( std::list<inode_t>::const_iterator itty = list->begin(); itty != list->end(); ++itty ) {
        ShinyMetaNode * sNode = findNode( (*itty) );
        
        //Compare sNode's filename with the section of path inbetween
        if( memcmp( sNode->getName(), childName, min( strlen(sNode->getName()), childNameLen ) ) ) {
            //If it works, then we return sNode
            return sNode;
        }
    }
    //If we made it all the way through without finding a match for that file, quit out
    return NULL;
}

ShinyMetaNode * ShinyMetaFilesystem::findNode( const char * path ) {
    if( path[0] != '/' ) {
        ERROR( "path %s is unacceptable, must be an absolute path!", path );
        return NULL;
    }
    
    ShinyMetaNode * currNode = root;
    unsigned long filenameBegin = 1;
    for( unsigned int i=1; i<strlen(path); ++i ) {
        if( path[i] == '/' ) {
            //If this one actually _is_ a directory, let's get its listing
            if( currNode->getNodeType() == SHINY_NODE_TYPE_DIR || currNode->getNodeType() == SHINY_NODE_TYPE_ROOTDIR ) {
                //Search currNode's children for a name match
                ShinyMetaNode * childNode = findMatchingChild( (ShinyMetaDir *)currNode, &path[filenameBegin], i - filenameBegin );
                if( !childNode ) {
                    ERROR( "Cannot resolve path %s, as the node %s does not have next node inside of it", path, currNode->getName() );
                    return NULL;
                } else
                    currNode = childNode;
            } else {
                ERROR("Cannot resolve path %s, as the node %s is not a directory", path, currNode->getName() );
                return NULL;
            }
            
            //Update the beginning of the next filename to be the character after this slash
            filenameBegin = i+1;
        }
    }
    return currNode;
}

ShinyMetaNode * ShinyMetaFilesystem::findNode( uint64_t inode ) {
    std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = this->nodes.find( inode );
    if( itty != this->nodes.end() )
        return (*itty).second;
    return NULL;
}

void ShinyMetaFilesystem::setDirty( void ) {
    this->dirty = true;
}

bool ShinyMetaFilesystem::isDirty( void ) {
    return this->dirty;
}

void ShinyMetaFilesystem::setClean( void ) {
    this->dirty = false;
}

bool ShinyMetaFilesystem::sanityCheck( void ) {
    bool retVal = true;
    //Call sanity check on all of them.
    //The nodes will return false if there is an error they cannot correct (right now that cannot happen)
    for( std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = this->nodes.begin(); itty != this->nodes.end(); ++itty ) {
        if( (*itty).second ) {
            if( !(*itty).second->sanityCheck() )
                retVal = false;
        } else {
            WARN( "inode [%d] is not in the nodes map!" );
            this->nodes.erase( itty++ );
        }
    }
    return retVal;
}

uint64_t ShinyMetaFilesystem::serialize( char ** store ) {
    //First, we're going to find the total length of this serialized monstrosity
    //We'll start with the version number
    uint64_t len = sizeof(uint16_t);
    
    //Next, what ShinyMetaFilesystem takes up itself
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
    *((uint16_t *)output) = ShinyMetaFilesystem::VERSION;
    output += sizeof(uint16_t);
    
    //Next, nextInode, followed by the number of nodes we're writing out
    *((inode_t *)output) = this->nextInode;
    output += sizeof(inode_t);
    
    *((uint64_t *)output) = nodes.size();
    output += sizeof(uint64_t);

    //Iterate through all nodes, writing them out
    for( std::tr1::unordered_map<inode_t, ShinyMetaNode *>::iterator itty = nodes.begin(); itty != nodes.end(); ++itty ) {
        //We're adding this in out here, because it's something the filesystem needs to be aware of,
        //not something the node should handle itself
        *((uint8_t *)output) = (*itty).second->getNodeType();
        output += sizeof(uint8_t);
        
        //Now actually serialize the node
        (*itty).second->serialize( output );
        output += (*itty).second->serializedLen();
    }
    
    setClean();
    return len;
}

void ShinyMetaFilesystem::unserialize(const char *input) {
    if( nodes.size() )
        ERROR( "Stuff was in nodes when unserializing!!!!" );
    
    if( *((uint16_t *)input) != ShinyMetaFilesystem::VERSION ) {
        WARN( "Warning:  Serialized filesystem is of version %d, whereas we are running version %d!", *((uint16_t *)input), ShinyMetaFilesystem::VERSION );
    }
    input += sizeof(uint16_t);
    
    this->nextInode = *((inode_t *)input);
    input += sizeof(inode_t);
    
    uint64_t numInodes = *((uint64_t *)input);
    input += sizeof(uint64_t);
    
    for( uint64_t i=0; i<numInodes; ++i ) {
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
}


inode_t ShinyMetaFilesystem::genNewInode() {
    inode_t probeNext = this->nextInode + 1;
    //Just linearly probe for the next open inode
    while( this->nodes.find(probeNext) != this->nodes.end() ) {
        if( ++probeNext == this->nextInode ) {
            ERROR( "ShinyMetaFilesystem->genNewInode(): inode space exhausted!\n" );
            throw "inode space exhausted!";
        }
    }
    inode_t toReturn = this->nextInode;
    this->nextInode = probeNext;
    return toReturn;
}

void ShinyMetaFilesystem::printDir( ShinyMetaDir * dir, const char * prefix ) {
    //prefix contains the current dir's name
    LOG( "[%llu] %s/\n", dir->getInode(), prefix );
    
    //Iterate over all children
    const std::list<inode_t> * listing = dir->getListing();
    for( std::list<inode_t>::const_iterator itty = listing->begin(); itty != listing->end(); ++itty ) {
        inode_t childInode = *itty;
        const char * childName = this->nodes[childInode]->getName();
        char * newPrefix = new char[strlen(prefix) + 2 + strlen(childName) + 1];
        sprintf( newPrefix, "%s/%s", prefix, childName );
        if( this->nodes[*itty]->getNodeType() == SHINY_NODE_TYPE_DIR ) {
            this->printDir( (ShinyMetaDir *)this->nodes[*itty], newPrefix );
        } else {
            //Only print it out here if it's not a directory, because directories print themselves
            LOG( "[%llu] %s\n", childInode, newPrefix );
        }
        delete( newPrefix );
    }
}

void ShinyMetaFilesystem::print( void ) {
    printDir( (ShinyMetaDir *)root, "" );
}