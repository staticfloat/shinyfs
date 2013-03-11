#include "ShinyMetaDirSnapshot.h"
#include <base/Logger.h>
#include <sys/stat.h>

ShinyMetaDirSnapshot::ShinyMetaDirSnapshot( const char ** serializedInput, ShinyMetaDirSnapshot * parent ) : ShinyMetaNodeSnapshot( serializedInput, parent ) {
    this->unserialize( serializedInput );
}

ShinyMetaDirSnapshot::~ShinyMetaDirSnapshot() {
    // Deletes all nodes this guy contains.  Nodes automagically remove themselves from us
    while( !nodes.empty() ) {
        delete( nodes.front() );
    }
}

/*
char * ShinyMetaDirSnapshot::serialize( char * output ) {
    // First, resize the vector to exactly the size it should be to compact memory as much as possible
    // If already at optimal capacity, this function does nothing. Note that this is not strictly
    // necessary, but as we serialize regularly anyway (to write out to disk and such) it saves memory
    TODO("Move this into a 'compact' or somesuch method");
    this->nodes.reserve( this->nodes.size() );
    
    // And then, after that exhausting work, call up the inheritance chain
    return ShinyMetaNodeSnapshot::serialize(output);
}*/

const std::vector<ShinyMetaNodeSnapshot *> * ShinyMetaDirSnapshot::getNodes() {
    return &nodes;
}

void ShinyMetaDirSnapshot::addNode( ShinyMetaNodeSnapshot *newNode ) {
    // Don't forget to reject NULL.  :P
    if( !newNode )
        return;
    
    if( newNode->getParent() != this ) {
        ERROR( "Cannot add a node whose parent is not us!" );
        return;
    }
    
    //Insert so that list is always sorted in ascending order of name
    int64_t i;
    for( i=0; i<this->nodes.size(); ++i ) {
        // This means we are in a position to insert our new node into the list
        if( strcmp( this->nodes[i]->getName(), newNode->getName() ) == -1 ) {
            // Do a quick check against the next node (if there is one) to make sure they're no duplicates
            if( i < this->nodes.size() - 1 && strcmp( this->nodes[i+1]->getName(), newNode->getName() ) == 0 ) {
                WARN( "File %s already exists in %s! Duplicates are not allowed!", newNode->getName(), this->getName() );
                return;
            }
            break;
        }
    }
    nodes.insert(nodes.begin() + i, newNode );
}

void ShinyMetaDirSnapshot::delNode(ShinyMetaNodeSnapshot *delNode) {
    for( uint64_t i=0; i<this->nodes.size(); ++i ) {
        if( this->nodes[i] == delNode ) {
            this->nodes.erase( this->nodes.begin() + i );
            return;
        }
    }
}

ShinyMetaNodeSnapshot * ShinyMetaDirSnapshot::findNode( const char *name ) {
    for( uint64_t i=0; i<this->nodes.size(); ++i ) {
        if( strcmp(this->nodes[i]->getName(), name) == 0 )
            return this->nodes[i];
    }
    return NULL;
}

uint64_t ShinyMetaDirSnapshot::getNumNodes( void ) {
    return nodes.size();
}

ShinyMetaNodeSnapshot::NodeType ShinyMetaDirSnapshot::getNodeType( void ) {
    return ShinyMetaNodeSnapshot::TYPE_DIR;
}

uint16_t ShinyMetaDirSnapshot::getDefaultPermissions( void ) {
    // For a directory, give it executable permissions!
    return ShinyMetaNodeSnapshot::getDefaultPermissions() | S_IXUSR | S_IXGRP | S_IXOTH;
}
