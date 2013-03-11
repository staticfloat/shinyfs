#include "ShinyMetaRootDirSnapshot.h"

ShinyMetaRootDirSnapshot::ShinyMetaRootDirSnapshot( const char ** serializedInput, ShinyFilesystem * fs ) : ShinyMetaDirSnapshot( serializedInput, this ) {
    this->unserialize( serializedInput );
}

ShinyMetaRootDirSnapshot::ShinyMetaRootDirSnapshot() {
    // Don't do anything here. :P
}

ShinyMetaRootDirSnapshot::~ShinyMetaRootDirSnapshot() {
    this->parent = NULL;
}

// Serves as the base case for the recursive "getFS()" calls done by all other nodes
ShinyFilesystem * const ShinyMetaRootDirSnapshot::getFS( void ) {
    return this->fs;
}

// Please don't tell me I have to explain to you what this does.  Go read ShinyMetaNode.  Go.  Right now.
ShinyMetaNodeSnapshot::NodeType ShinyMetaRootDirSnapshot::getNodeType( void ) {
    return ShinyMetaNodeSnapshot::TYPE_ROOTDIR;
}

const char * ShinyMetaRootDirSnapshot::getPath( void ) {
    // This is kind of a weird optimization, but there we go.
    return "/";
}
