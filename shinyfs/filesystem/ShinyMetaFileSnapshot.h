#pragma once
#ifndef ShinyMetaFileSnapshot_H
#define ShinyMetaFileSnapshot_H
#include <sys/types.h>

#include "ShinyMetaNodeSnapshot.h"
#include "ShinyDBWrapper.h"

class ShinyMetaDir;
class ShinyMetaFileSnapshot : public ShinyMetaNodeSnapshot {
friend class ShinyMetaFile;
/////// DEFINES ///////
public:
    // The size of a "chunk" stored in the DB
    // NABIL: This should probably be defined in a Shiny_config.h eventually
    static const uint64_t CHUNKSIZE = 64*1024;
    
//////// CREATION ///////
public:
    //Load from a serialized stream
    ShinyMetaFileSnapshot( const char ** serializedInput, ShinyMetaDirSnapshot * parent );
    
    //Cleanup before DESTRUCTION
    ~ShinyMetaFileSnapshot();
    
    // We add in the length of the file to our serialization format
    virtual uint64_t serializedLen( void );
    virtual char * serialize( char * output );
    virtual void unserialize( const char **input );
protected:
    // Only for ShinyMetaFile to use
    ShinyMetaFileSnapshot();
    
    
/////// ATTRIBUTES //////
public:
    // Returns the length of the file in bytes
    virtual uint64_t getLen();
    
    // Blocks until task completion. Not multithread safe. Returns number of bytes read.
    virtual uint64_t read( uint64_t offset, char * data, uint64_t len );
protected:
    // This is the guy that does the real work, the above read() subs out to this guy,
    // and just grab the db object from the ShinyFS, (which is why I have ShinyMetafileHandle for when
    // the ShinyFS object is unreachable, but we have the db object at hand)
    virtual uint64_t read( ShinyDBWrapper * db, const char * path, uint64_t offset, char * data, uint64_t len );
    
    // The length of this here file
    uint64_t fileLen;
    
/////// MISC ///////
public:
    //Performs various checks to make sure this node is all right
    //virtual bool sanityCheck();
    
    //Always returns TYPE_FILE, imagine that!
    virtual ShinyMetaNodeSnapshot::NodeType getNodeType( void );
protected:
};

#endif // ShinyMetaFileSnapshot_H