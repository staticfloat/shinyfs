#pragma once
#ifndef SHINYMETAFILE_H
#define SHINYMETAFILE_H
#include <sys/types.h>

#include "ShinyMetaNode.h"
#include "ShinyMetaFileSnapshot.h"
#include "ShinyDBWrapper.h"

class ShinyMetaDir;
class ShinyMetaFile : virtual public ShinyMetaNode {
/////// DEFINES ///////
public:
    // The size of a "chunk" stored in the DB
    static const uint64_t CHUNKSIZE = 64*1024;
    
//////// CREATION ///////
public:
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaFile( const char * newName, ShinyMetaDir * parent );

    //Load from a serialized stream
    ShinyMetaFile( const char ** serializedInput, ShinyMetaDir * parent );
    
    //Cleanup before DESTRUCTION
    ~ShinyMetaFile();
    
/////// ATTRIBUTES //////
public:
    // Set a new length for this file (is implicitly called by write())
    // truncates if newLen < getLen(), appends zeros if newLen > getLen()
    virtual void setLen( uint64_t newLen );
    
    // Blocks until task completion. Should only be called from same thread as one that owns the
    // ShinyFilesystem. Returns how many bytes we were able to read/write.
    virtual uint64_t write( uint64_t offset, const char * data, uint64_t len );
protected:
    // These are the peeps that do the real work, the above setLen() and write() sub out to thess guys,
    // and just grab the db object from the ShinyFS, (which is why I have ShinyMetafileHandle for when
    // the ShinyFS object is unreachable, but we have the db object at hand)
    virtual uint64_t write( ShinyDBWrapper * db, const char * path, uint64_t offset, const char * data, uint64_t len );
    virtual void setLen( ShinyDBWrapper * db, const char * path, uint64_t newLen );
    
/////// MISC ///////
public:
    //Performs various checks to make sure this node is all right
    //virtual bool sanityCheck();
    
    //Always returns TYPE_FILE, imagine that!
    //virtual ShinyMetaNode::NodeType getNodeType( void );
protected:   
};

#endif //SHINYMETAFILE_H