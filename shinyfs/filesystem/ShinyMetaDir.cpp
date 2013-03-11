#include "ShinyMetaDir.h"
#include "ShinyFilesystem.h"
#include "base/Logger.h"


ShinyMetaDir::ShinyMetaDir( const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( newName, parent ) {
    this->setPermissions( ShinyMetaDirSnapshot::getDefaultPermissions() );
}

ShinyMetaDir::ShinyMetaDir( const char ** serializedInput, ShinyMetaDir * parent ) : ShinyMetaNode(serializedInput, parent), ShinyMetaDirSnapshot( serializedInput, parent ) {
    this->unserialize( serializedInput );
}

ShinyMetaDir::~ShinyMetaDir( void ) {
    // Once again, do nothing
}

const std::vector<ShinyMetaNode *> * ShinyMetaDir::getNodes() {
    return &this->nodes;
}

void ShinyMetaDir::addNode(ShinyMetaNode *newNode) {
    ShinyMetaDirSnapshot::addNode( newNode );
    this->set_mtime();
}

void ShinyMetaDir::delNode(ShinyMetaNode *delNode) {
    ShinyMetaDirSnapshot::delNode( delNode );
    this->set_mtime();
}

/*

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
*/