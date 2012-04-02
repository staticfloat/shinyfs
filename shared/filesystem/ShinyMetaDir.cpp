#include "ShinyMetaDir.h"
#include "ShinyFilesystem.h"
#include "base/Logger.h"
#include <sys/stat.h>

ShinyMetaDir::ShinyMetaDir( const char * newName ) : ShinyMetaNode( newName ) {
    // Too bad polymorphism can't help us out here.  :P
    this->setPermissions( this->getDefaultPermissions() );
}

ShinyMetaDir::ShinyMetaDir( const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( newName ) {
    this->setPermissions( this->getDefaultPermissions() );
    parent->addNode( this );
}

ShinyMetaDir::ShinyMetaDir( const char ** serializedInput ) : ShinyMetaNode( "" ) {
    this->unserialize( serializedInput );
}

ShinyMetaDir::~ShinyMetaDir( void ) {
    while( !nodes.empty() ) {
        //If there _is_ a child that doesn't exist, that's fine, 'cause deleting NULL doesn't do squat!
        delete( nodes.front() );
    }
}


uint64_t ShinyMetaDir::serializedLen( void ) {
    // base amount
    uint64_t len = ShinyMetaNode::serializedLen();
    
    // retuuuuuuurrnnnnnn....... Retuuuuuurrrnnnnnn...... REEEETUUUUUURRRNNNN!!!!
    return len;
}

char * ShinyMetaDir::serialize( char * output ) {
    // First, resize the vector to exactly the size it should be to compact memory as much as possible
    // If already at optimal capacity, this function does nothing. Note that this is not necessary when
    // serializing, but since we serialize regularly anyway (to write out to disk and such) it saves memory
    this->nodes.reserve( this->nodes.size() );
    
    // And then, after that exhausting work, the nodal stuffage
    return ShinyMetaNode::serialize(output);
}

void ShinyMetaDir::unserialize(const char **input) {
    // Just call up the chain.  That is literally it
    ShinyMetaNode::unserialize(input);
}


void ShinyMetaDir::addNode( ShinyMetaNode *newNode ) {
    // Don't forget to reject NULL.  :P
    if( !newNode )
        return;
    
    if( newNode->getParent() ) {
        ERROR( "Cannot add a node that already has a parent!" );
        return;
    }
    
    TODO( "Perhaps change ShinyMetaDir to use some kind of map instead of a list, to allow for faster inserts?" );
         
    //Insert so that list is always sorted in ascending order of name
    int64_t i;
    for( i=0; i<this->nodes.size(); ++i ) {
        int ret = strcmp( this->nodes[i]->getName(), newNode->getName() );
        // This means we are in a position to insert our new node into the list
        if( ret == -1 ) {
            // Do a quick check against the next node (if there is one) to make sure they're no duplicates
            if( i < this->nodes.size() - 1 && strcmp( this->nodes[i+1]->getName(), newNode->getName() ) == 0 ) {
                WARN( "Cannot add node %s to %s again!  duplicates are not allowed!", newNode->getName(), this->getName() );
                return;
            }
            break;
        }
    }
    nodes.insert(nodes.begin() + i, newNode );
    newNode->setParent( this );
}

void ShinyMetaDir::delNode(ShinyMetaNode *delNode) {
    for( uint64_t i=0; i<this->nodes.size(); ++i ) {
        if( this->nodes[i] == delNode ) {
            this->nodes.erase( this->nodes.begin() + i );
            return;
        }
    }
}

const std::vector<ShinyMetaNode *> * ShinyMetaDir::getNodes() {
    return &nodes;
}

uint64_t ShinyMetaDir::getNumNodes( void ) {
    return nodes.size();
}

bool ShinyMetaDir::check_childrenHaveUsAsParent( void ) {
    bool retVal = true;
    //Iterate through all nodes
    for( uint64_t i = 0; i<this->nodes.size(); ++i ) {
        if( this->nodes[i]->getParent() != this ) {
            // Temp variable to make it a little easier to read this code
            const char * parentStr = this->nodes[i]->getParent() ? this->nodes[i]->getParent()->getName() : "<null>";
            WARN( "Child %s/%s had %s as its parent!", this->getName(), this->nodes[i]->getName(), parentStr );
            retVal = false;
        }
    }
    return retVal;
}

bool ShinyMetaDir::sanityCheck() {
    //First, do the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();
    
    //Check no dups in children
    retVal &= check_noDuplicates( &this->nodes, "nodes" );
    
    //Finally, check all nodes point to us
    retVal &= check_childrenHaveUsAsParent();
    
    return retVal;
}

ShinyMetaNode::NodeType ShinyMetaDir::getNodeType( void ) {
    return ShinyMetaNode::TYPE_DIR;
}

uint16_t ShinyMetaDir::getDefaultPermissions( void ) {
    // For a directory, give it executable permissions!
    return this->ShinyMetaNode::getDefaultPermissions() | S_IXUSR | S_IXGRP | S_IXOTH;
}