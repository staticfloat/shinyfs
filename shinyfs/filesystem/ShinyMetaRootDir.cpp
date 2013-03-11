#include "ShinyMetaRootDir.h"
#include "ShinyFilesystem.h"
#include <base/Logger.h>

ShinyMetaRootDir::ShinyMetaRootDir( ShinyFilesystem * fs ) : ShinyMetaDir( "", this ), fs(fs) {
    //We set ourselves as our own parent.  How...... cute.  :P
    //this->setParent( this );
}

ShinyMetaRootDir::ShinyMetaRootDir( const char ** serializedInput, ShinyFilesystem * fs ) : ShinyMetaDir( serializedInput, this ), fs(fs) {
    // Don't do ANYTHING!
}

ShinyMetaRootDir::~ShinyMetaRootDir() {
    // I love refactoring
}

/*
//Always return true as our parent is ourself, therefore there's nothing special to be done here
bool ShinyMetaRootDir::check_parentHasUsAsChild( void ) {
    return true;
}
*/

