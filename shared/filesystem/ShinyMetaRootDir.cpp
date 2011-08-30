#include "ShinyMetaRootDir.h"
#include "ShinyFilesystem.h"
#include <base/Logger.h>

ShinyMetaRootDir::ShinyMetaRootDir( ShinyFilesystem * fs ) : ShinyMetaDir( fs, "" ) {
    //We set ourselves as our own parent.  How...... cute.  :P
    this->setParent( this->inode );
}

ShinyMetaRootDir::ShinyMetaRootDir( const char * serializedInput, ShinyFilesystem * fs ) : ShinyMetaDir( serializedInput, fs ) {
}

ShinyNodeType ShinyMetaRootDir::getNodeType( void ) {
    return SHINY_NODE_TYPE_ROOTDIR;
}

//Always return true as our parent is ourself, therefore there's nothing special to be done here
bool ShinyMetaRootDir::check_parentHasUsAsChild( void ) {
    return true;
}

const char * ShinyMetaRootDir::getPath( void ) {
    this->path = new char[2];
    path[0] = '/';
    path[1] = 0;
    return path;
}

