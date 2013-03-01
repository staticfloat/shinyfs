#pragma once
#ifndef ShinyMetaDir_H
#define ShinyMetaDir_H
#include <vector>
#include "ShinyMetaNode.h"
#include "ShinyMetaDirSnapshot.h"

class ShinyMetaDir : virtual public ShinyMetaNode {
friend class ShinyMetaNode;
/////// CREATION ///////
public:
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaDir( const char * newName, ShinyMetaDir * parent );
    
    //Loads in from serialized input
    ShinyMetaDir( const char ** serializedInput, ShinyMetaDir * parent );

    //Deletes all children
    ~ShinyMetaDir( void );

    //Unserializes. Duh.
    virtual void unserialize( const char **input );
    

/////// NODE MANAGEMENT ///////
public:
    // Returns a directory listing
    const std::vector<ShinyMetaNode *> * getNodes();
    
    // finds a node with the given basename and returns it, NULL otherwise
    virtual ShinyMetaNode * findNode( const char * name );

protected:
    // Adds/removes a meta node to the current list of nodes, only callable by ShinyMetaNodeSnapshot and friends
    void addNode( ShinyMetaNode * newNode );
    void delNode( ShinyMetaNode * delNode );

    std::vector<ShinyMetaNode *> nodes;
    
/////// MISC ///////
public:
    //Performs random sanity checks
    //virtual bool sanityCheck();
    
    // Returns the NodeType (TYPE_DIR)
    
protected:
    //Checks to make sure that all children have this as a parent
    //Lots of code overlap with check_parentsHaveUsAsChild, but whatever
    //virtual bool check_childrenHaveUsAsParent( void );
    
    // Override the default permissions for directories, as we need to have execute set so that people can read!
    //virtual uint16_t getDefaultPermissions( void );
};


#endif //ShinyMetaDir_H