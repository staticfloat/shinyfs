#pragma once
#ifndef SHINYMETAFILEHANDLE_H
#define SHINYMETAFILEHANDLE_H
#include "ShinyMetaFile.h"
#include <zmq.hpp>

// Specialized subclass of ShinyMetaFile used to allow multithreaded access to the 
// DB object, and thus multithreaded read/write access to file data.
class ShinyMetaFileHandle : public ShinyMetaFile {
//////// CREATION ///////
public:
    // This guy can only be created from a serialized input, as he is only
    // used to perform read/writes from ShinyFuse.
    ShinyMetaFileHandle( const char ** serializedInput, ShinyFilesystem * fs, const char * path );
    
    // Cleanup before DESTRUCTION
    ~ShinyMetaFileHandle();
protected:
    // Gotta hang on to this sucker, so that we can use fs->getZMQContext()
    ShinyFilesystem * fs;
    
    // Gotta hang on to the path, as we need it when talking to the db object
    const char * path;

/////// ATTRIBUTES //////
public:
    // Blocks until task completion. Uses context from ShinyFilesystem to create
    // a new socket to talk to the filecache directly.  This is significantly
    // different from the read/write pair in ShinyMetaFile, and indeed is the
    // whole reason for this object's existance
    virtual uint64_t read( uint64_t offset, char * data, uint64_t len );
    virtual uint64_t write( uint64_t offset, const char * data, uint64_t len );
    virtual void setLen( uint64_t newLen );
    
    // Override this for simplicity
    virtual ShinyMetaNode::NodeType getNodeType();
};

#endif //SHINYMETAFILE_H