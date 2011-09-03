#pragma once
#ifndef SHINYMETAFILE_H
#define SHINYMETAFILE_H
#include <sys/types.h>
#include "ShinyMetaNode.h"
#include "ShinyFileChunk.h"
#include <vector>


class ShinyMetaDir;
class ShinyMetaFile : public ShinyMetaNode {
public:
    //Create a new one
    ShinyMetaFile( ShinyFilesystem * fs, const char * newName );
    
    //Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaFile( ShinyFilesystem * fs, const char * newName, ShinyMetaDir * parent );
    
    //Load from a serialized stream
    ShinyMetaFile( const char * serializedInput, ShinyFilesystem * fs );
    
    //Cleanup before DESTRUCTION
    ~ShinyMetaFile();
    
    //Have to override this so that we can invalidate our cached prefix
    virtual void setName( const char * newName );
    
    //Set a new length for this file (is implicitly called by write())
    virtual size_t truncate( size_t newLen );
    virtual size_t getFileLen();
    
    //Performs various checks to make sure this node is all right
    virtual bool sanityCheck();

    //Helper function to generate the path used to access the file on disk
    //Note that this is a _prefix_, and that actual file cache chunks will be
    //stored on disk with postfixes of what byte they start at
    virtual const char * getCachePrefix( void );
    
    //Get the length of the serialized output first
    virtual size_t serializedLen( void );
    virtual void serialize( char * output );
    
    //Pass flush() messages on to chunks
    virtual void flush( void );

    //Returns how many bytes we were able to read/write.  Note that both operations
    //require us to have the files loaded in the segments where we read from/write to.
    //There is no writing to files that we don't have yet yet.
    virtual size_t read( uint64_t offset, char * buffer, size_t len );
    virtual size_t write( uint64_t offset, const char * buffer, size_t len );
    
    //Always returns SHINY_NODE_TYPE_FILE, imagine that!
    virtual ShinyNodeType getNodeType( void );
protected:
    virtual void unserialize( const char * input );
private:
    /********** TRANSIENT/GENERATED STUFF - NOT SERIALIZED ****************/
    char * cachePrefix;
    
    //A new file 
    std::vector<ShinyFileChunk *> chunks;
};

#endif //SHINYMETAFILE_H