#pragma once
#ifndef ShinyMetaRootDirSnapshot_H
#define ShinyMetaRootDirSnapshot_H
#include "ShinyMetaDirSnapshot.h"

class ShinyMetaRootDirSnapshot : virtual public ShinyMetaDirSnapshot {
public:
    // Note that we can't unserialize off of _solely_ a serializedInput, as we are responsibly for managing the poiner
    // to fs, therefore we require that it is passed in when we load up as well.
    ShinyMetaRootDirSnapshot( const char ** serializedInput, ShinyFilesystem * fs );
    
    // Deconstruct, Dalek-style!
    ~ShinyMetaRootDirSnapshot();
    
    ShinyMetaNodeSnapshot::NodeType getNodeType( void );
    
    // Override this to actually return the fs object, so's we can provde it for all of our starving children
    ShinyFilesystem * const getFS();
    
    // Override this just as a performance boost to always return '/'
    const char * getPath( void );
protected:
    ShinyMetaRootDirSnapshot();

    // LOL, Override this guy so that we don't check if our parent (us) has us as a child
    // Refactor: don't think I need this anymore
    //virtual bool check_parentHasUsAsChild( void );
    
private:
    // The most excellent ShinyFilesystem that we are forever passing out to peoples
    ShinyFilesystem * fs;
};


#endif // ShinyMetaRootDirSnapshot_H