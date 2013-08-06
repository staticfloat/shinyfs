#pragma once
#ifndef ShinyMetaRootDir_H
#define ShinyMetaRootDir_H
#include "ShinyMetaDir.h"
#include "ShinyMetaRootDirSnapshot.h"

class ShinyMetaRootDir : public ShinyMetaDir {
public:
    // Creates a new one
    ShinyMetaRootDir( ShinyFilesystem * fs );
    
    // Note that we can't unserialize off of _solely_ a serializedInput, as we are responsibly for managing the poiner
    // to fs, therefore we require that it is passed in when we load up as well.
    ShinyMetaRootDir( const char ** serializedInput, ShinyFilesystem * fs );
    
    // some trickery is to be done here, setting our parent to "NULL" so that ~ShinyMetaNode() doesn't do dumb things
    ~ShinyMetaRootDir();
    
    virtual ShinyMetaNodeSnapshot::NodeType getNodeType( void );
    
    // Override this to actually return the fs object, so's we can provde it for all of our starving children
    virtual ShinyFilesystem * getFS();
    
    // Override this just as a performance boost to always return '/'
    virtual const char * getPath( void );
protected:
    // LOL, Override this guy so that we don't check if our parent (us) has us as a child
    //virtual bool check_parentHasUsAsChild( void );
    
private:
    // The most excellent ShinyFilesystem that we are forever passing out to peoples
    ShinyFilesystem * fs;
};


#endif // ShinyMetaRootDir_H