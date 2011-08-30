#pragma once
#ifndef SHINYMETAROOTDIR_H
#define SHINYMETAROOTDIR_H
#include "ShinyMetaDir.h"

class ShinyMetaRootDir : public ShinyMetaDir {
public:
    ShinyMetaRootDir( ShinyFilesystem * fs );
    ShinyMetaRootDir( const char * serializedInput, ShinyFilesystem * fs );
    
    virtual ShinyNodeType getNodeType( void );
    
    //Override this just as a performance boost to always return '/'
    virtual const char * getPath( void );
    
protected:
    //LOL, Override this guy so that we don't check if our parent (us) has us as a child
    virtual bool check_parentHasUsAsChild( void );
};


#endif //SHINYROOTDIR_H