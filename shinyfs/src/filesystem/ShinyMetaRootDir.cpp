#include "ShinyMetaRootDir.h"
#include "ShinyFilesystem.h"
#include <base/Logger.h>

ShinyMetaRootDir::ShinyMetaRootDir( ShinyFilesystem * fs ) : ShinyMetaDir( "/" ), fs(fs) {
    //We set ourselves as our own parent.  How...... cute.  :P
    this->setParent( this );
}

ShinyMetaRootDir::ShinyMetaRootDir( const char ** serializedInput ) : ShinyMetaDir( "" ) {
    this->unserialize( serializedInput );
}

ShinyMetaRootDir::~ShinyMetaRootDir() {
    this->setParent( NULL );
}

// Serves as the base case for the recursive "getFS()" calls done by all other nodes
ShinyFilesystem * ShinyMetaRootDir::getFS( void ) {
    return this->fs;
}

// Please don't tell me I have to explain to you what this does.  Go read ShinyMetaNode.  Go.  Right now.
ShinyMetaNode::NodeType ShinyMetaRootDir::getNodeType( void ) {
    return ShinyMetaNode::TYPE_ROOTDIR;
}

//Always return true as our parent is ourself, therefore there's nothing special to be done here
bool ShinyMetaRootDir::check_parentHasUsAsChild( void ) {
    return true;
}

const char * ShinyMetaRootDir::getPath( void ) {
    return "/";
}

