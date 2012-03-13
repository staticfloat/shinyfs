#pragma once
#ifndef SHINYMETAFILE_H
#define SHINYMETAFILE_H
#include <sys/types.h>
#include "ShinyMetaNode.h"
#include "ShinyFileChunk.h"
#include <vector>


class ShinyMetaDir;
class ShinyMetaFile : public ShinyMetaNode {
//////// CREATION ///////
public:
    // Create a new one
    ShinyMetaFile( ShinyFilesystem * fs, const char * newName );
    
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaFile( ShinyFilesystem * fs, const char * newName, ShinyMetaDir * parent );
    
    //Load from a serialized stream
    ShinyMetaFile( const char * serializedInput, ShinyFilesystem * fs );
    
    //Cleanup before DESTRUCTION
    ~ShinyMetaFile();
    
    //Get the length of the serialized output first
    virtual size_t serializedLen( void );
    virtual void serialize( char * output );
protected:
    virtual void unserialize( const char * input );
    

/////// ATTRIBUTES //////
public:
    // Set a new length for this file (is implicitly called by write())
    // truncates if newLen < getLen(), appends zeros if newLen > getLen()
    virtual void setLen( uint64_t newLen );
    virtual uint64_t getLen();
    
    // Returns how many bytes we were able to read/write. Communicates with the ShinyCache,
    // and uses the context stored in the fs to figure out exactly which fuse thread is asking, etc...
    virtual uint64_t read( uint64_t offset, char * buffer, uint64_t len );
    virtual uint64_t write( uint64_t offset, const char * buffer, uint64_t len );
protected:
    uint64_t fileLen;

    
/////// MISC ///////
public:
    //Performs various checks to make sure this node is all right
    virtual bool sanityCheck();
    
    //Always returns TYPE_FILE, imagine that!
    virtual ShinyMetaNode::NodeType getNodeType( void );
protected:   
};

#endif //SHINYMETAFILE_H