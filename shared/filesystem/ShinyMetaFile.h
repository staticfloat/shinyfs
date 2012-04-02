#pragma once
#ifndef SHINYMETAFILE_H
#define SHINYMETAFILE_H
#include <sys/types.h>
// (kyoto cabinet this clobbers my ERROR/WARN macros, so I have to include it before Logger.h)
#include <kcpolydb.h>
#include "ShinyMetaNode.h"

class ShinyMetaDir;
class ShinyMetaFile : public ShinyMetaNode {
/////// DEFINES ///////
public:
    // The size of a "chunk" stored in the DB
    static const uint64_t CHUNKSIZE = 1*1024;
    
//////// CREATION ///////
public:
    // Create a new one
    ShinyMetaFile( const char * newName );
    
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaFile( const char * newName, ShinyMetaDir * parent );

    //Load from a serialized stream
    ShinyMetaFile( const char ** serializedInput );
    
    //Cleanup before DESTRUCTION
    ~ShinyMetaFile();
    
    //Get the length of the serialized output first
    virtual uint64_t serializedLen( void );
    virtual char * serialize( char * output );

    virtual void unserialize( const char **input );
    

/////// ATTRIBUTES //////
public:
    // Set a new length for this file (is implicitly called by write())
    // truncates if newLen < getLen(), appends zeros if newLen > getLen()
    virtual void setLen( uint64_t newLen );
    virtual uint64_t getLen();
    
    // Blocks until task completion. Should only be called from same thread as one that owns the
    // ShinyFilesystem. Returns how many bytes we were able to read/write.
    virtual uint64_t read( uint64_t offset, char * data, uint64_t len );
    virtual uint64_t write( uint64_t offset, const char * data, uint64_t len );
protected:
    // These are the peeps that do the real work, the above read() and write() sub out to this guy,
    // and just grab the db object from the ShinyFS, (which is why I have ShinyMetafileHandle for when
    // the ShinyFS object is unreachable, but we have the db object at hand)
    virtual uint64_t read( kyotocabinet::PolyDB * db, const char * path, uint64_t offset, char * data, uint64_t len );
    virtual uint64_t write( kyotocabinet::PolyDB * db, const char * path, uint64_t offset, const char * data, uint64_t len );
    virtual void setLen( kyotocabinet::PolyDB * db, const char * path, uint64_t newLen );
    
    // The length of this here file
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