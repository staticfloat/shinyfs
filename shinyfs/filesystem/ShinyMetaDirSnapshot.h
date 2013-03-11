#pragma once
#ifndef ShinyMetaDirSnapshot_H
#define ShinyMetaDirSnapshot_H
#include <stdint.h>
#include <vector>
#include "ShinyMetaNodeSnapshot.h"

class ShinyMetaDirSnapshot : virtual public ShinyMetaNodeSnapshot {
friend class ShinyMetaNodeSnapshot;
/////// CREATION ///////
public:
    // Don't think I need this right now
    //ShinyMetaDirSnapshot( ShinyMetaDirSnapshot * copy );
    
    // Loads in from serialized input
    explicit ShinyMetaDirSnapshot( const char ** serializedInput, ShinyMetaDirSnapshot * parent );
    
    // Deletes this guy, and all his children (man, that sounds violent)
    ~ShinyMetaDirSnapshot();
protected:
    ShinyMetaDirSnapshot();

/////// NODE MANAGEMENT ///////
public:
    // Returns a directory listing
    const std::vector<ShinyMetaNodeSnapshot *> * getNodes();
    
    // Finds a node and returns it, NULL otherwise
    ShinyMetaNodeSnapshot * findNode( const char * name );
    
    // Returns the number of children that belong to this dir
    uint64_t getNumNodes();
protected:
    // These are protected so only ShinyFilesystem and ShinyMetaNodes can use them
    void addNode( ShinyMetaNodeSnapshot * newNode );
    void delNode( ShinyMetaNodeSnapshot * delNode );
    
    // All of this dir's child nodes
    std::vector<ShinyMetaNodeSnapshot *> nodes;

    /////// MISC ///////
public:
    virtual ShinyMetaNodeSnapshot::NodeType getNodeType( void );
protected:
    // Override the default permissions for directories, as we need to have execute set so that people can read!
    virtual uint16_t getDefaultPermissions( void );
};

#endif //ShinyMetaDirSnapshot_H
