#pragma once
#ifndef SHINYMETADIR_H
#define SHINYMETADIR_H
#include <vector>
#include "ShinyMetaNode.h"

class ShinyMetaDir : public ShinyMetaNode {
/////// CREATION ///////
public:
    // This makes a new one
    ShinyMetaDir( const char * newName );
    
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaDir( const char * newName, ShinyMetaDir * parent );
    
    //Loads in from serialized input
    ShinyMetaDir( const char ** serializedInput );

    //Deletes all children
    ~ShinyMetaDir( void );

    virtual uint64_t serializedLen( void );
    virtual char * serialize( char * output );

    //Unserializes. Duh.
    virtual void unserialize( const char **input );


/////// NODE MANAGEMENT ///////
public:
    //Adds/removes a meta node to the current list of nodes.
    //Nodes should not be added multiple times.
    virtual void addNode( ShinyMetaNode * newNode );
    virtual void delNode( ShinyMetaNode * delNode );

    //Returns a directory listing
    virtual const std::vector<ShinyMetaNode *> * getNodes();
    
    //Returns the number of children that belong to this dir
    virtual uint64_t getNumNodes();
protected:
    //All of this dir's child nodes
    std::vector<ShinyMetaNode*> nodes;

    
    
    
/////// MISC ///////
public:
    //Performs random sanity checks
    virtual bool sanityCheck();
    
    // Returns the NodeType (TYPE_DIR)
    virtual ShinyMetaNode::NodeType getNodeType( void );
protected:
    //Checks to make sure that all children have this as a parent
    //Lots of code overlap with check_parentsHaveUsAsChild, but whatever
    virtual bool check_childrenHaveUsAsParent( void );
    
    // Override the default permissions for directories, as we need to have execute set so that people can read!
    virtual uint16_t getDefaultPermissions( void );
};


#endif //SHINYMETADIR_H