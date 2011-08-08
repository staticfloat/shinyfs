#pragma once
#ifndef SHINYMETAROOTDIR_H
#define SHINYMETAROOTDIR_H
#include "ShinyMetaDir.h"

class ShinyMetaRootDir : public ShinyMetaDir {
public:
    ShinyMetaRootDir( ShinyMetaFilesystem * fs );
    ShinyMetaRootDir( const char * serializedInput, ShinyMetaFilesystem * fs );
    
    virtual ShinyNodeType getNodeType( void );
        
    virtual const char * getPath();
protected:
    //Override this guy so that we don't check if our parents have us as children
    virtual bool check_parentsHaveUsAsChild( void );
};


#endif //SHINYROOTDIR_H