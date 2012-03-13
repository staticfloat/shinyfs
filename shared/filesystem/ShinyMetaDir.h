#pragma once
#ifndef SHINYMETADIR_H
#define SHINYMETADIR_H
#include <vector>
#include "ShinyMetaNode.h"

class ShinyMetaDir : public ShinyMetaNode {
/////// CREATION ///////
public:
    // This makes a new one
    ShinyMetaDir( ShinyFilesystem * fs, const char * newName );
    
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaDir( ShinyFilesystem * fs, const char * newName, ShinyMetaDir * parent );
    
    //Loads in from serialized input
    ShinyMetaDir( const char * serializedInput, ShinyFilesystem * fs );
    
    //Deletes all children
    ~ShinyMetaDir( void );

    virtual size_t serializedLen( void );
    virtual void serialize( char * output );
protected:
    //Unserializes. Duh.
    virtual void unserialize( const char * input );


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

};


#endif //SHINYMETADIR_H