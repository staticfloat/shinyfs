#include "ShinyMetaRootDir.h"
#include "ShinyMetaFilesystem.h"
#include <base/Logger.h>

ShinyMetaRootDir::ShinyMetaRootDir( ShinyMetaFilesystem * fs ) : ShinyMetaDir( fs, "" ) {
    //We set ourselves as our own parent.  How...... cute.  :P
    this->addParent( this->inode );
}

ShinyMetaRootDir::ShinyMetaRootDir( const char * serializedInput, ShinyMetaFilesystem * fs ) : ShinyMetaDir( serializedInput, fs ) {
}

ShinyNodeType ShinyMetaRootDir::getNodeType( void ) {
    return SHINY_NODE_TYPE_ROOTDIR;
}

//Always return true as our parent is ourself, therefore there's nothing special to be done here
bool ShinyMetaRootDir::check_parentsHaveUsAsChild( void ) {
    return true;
}

const char * ShinyMetaRootDir::getPath( void ) {
    char * path = new char[2];
    path[0] = '/';
    path[1] = 0;
    return path;
}