#pragma once
#ifndef SHINYMETADIR_H
#define SHINYMETADIR_H
#include <list>
#include "ShinyMetaNode.h"

class ShinyMetaDir : public ShinyMetaNode {
public:
    //This makes a new one
    ShinyMetaDir( ShinyMetaFilesystem * fs, const char * newName );
    
    //Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    ShinyMetaDir( ShinyMetaFilesystem * fs, const char * newName, ShinyMetaDir * parent );
    
    ShinyMetaDir( const char * serializedInput, ShinyMetaFilesystem * fs );
    
    //Adds/removes a meta node to the current list of nodes.
    //Nodes should not be added multiple times.
    virtual void addNode( ShinyMetaNode * newNode );
    virtual void delNode( ShinyMetaNode * delNode );

    //Returns a directory listing
    virtual const std::list<inode_t> * getListing();
    
    //Returns the number of children that belong to this dir
    virtual uint64_t getNumChildren();

    //Performs random sanity checks
    virtual bool sanityCheck();
    
    virtual uint64_t serializedLen( void );
    virtual void serialize( char * output );
    virtual ShinyNodeType getNodeType( void );

    //Override this so we can pass it down onto files
    virtual void flush( void );
protected:
    //Checks to make sure that all children have this as a parent
    //Lots of code overlap with check_parentsHaveUsAsChild, but whatever
    virtual bool check_childrenHaveUsAsParent( void );

    //Unserializes.  Duh.
    virtual void unserialize( const char * input );

    //All of this dir's child nodes
    std::list<inode_t> nodes;
};


#endif //SHINYMETADIR_H