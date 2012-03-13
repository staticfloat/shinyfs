#pragma once
#ifndef ShinyMetaNode_H
#define ShinyMetaNode_H

#include <stdint.h>
#include <list>
#include <vector>

class ShinyFilesystem;
class ShinyMetaDir;
class ShinyMetaNode {
/////// TYPEDEFS ///////
public:
    // The type 
    enum NodeType {
        TYPE_NODE,
        TYPE_FILE,
        TYPE_DIR,
        TYPE_ROOTDIR,
        NUM_NODE_TYPES,
    };


/////// CREATION ///////
public:
    //Generate a new node and create new inode (by asking ShinyMetaTree), etc.....
    ShinyMetaNode( ShinyFilesystem * fs, const char * newName );
    
    //Initialize this node, from serialized data.  Get amount of data read in via serializeLen() afterwards
    ShinyMetaNode( const char * serializedInput, ShinyFilesystem * fs );
    
    //Clean up (free name)
    ~ShinyMetaNode();
    
    //Returns the length of a serialization on this node
    virtual size_t serializedLen( void );
    
    //Serializes into the buffer [output]
    virtual void serialize( char * output );
protected:
    //Called by ShinyMetaNode() to load from a serialized string
    virtual void unserialize( const char * input );
    
    

//////// ATTRIBUTES ///////
public:    
    //Name (filename, directory name, etc....)
    virtual void setName( const char * newName );
    virtual const char * getName();
    
    //Set new permissions for this node
    virtual void setPermissions( uint16_t newPermissions);
    virtual uint16_t getPermissions();
    
    //Get/set parent
    virtual ShinyMetaDir * getParent( void );
    virtual void setParent( ShinyMetaDir * newParent );
    
    //Gets the absolute path to this node
    virtual const char * getPath();
    
    //Returns the node type, e.g. if it's a file, directory, etc.
    virtual ShinyMetaNode::NodeType getNodeType( void );
protected:
    //Parent of this node
    ShinyMetaDir * parent;
    
    //Filename (+ extension)
    char * name;
    
    //file permissions
    uint16_t permissions;
    
    //User/Group IDs (not implemented yet)
    uint64_t uid;
    uint64_t gid;
    
    uint64_t ctime;                 //Time created
    uint64_t atime;                 //Time last accessed
    uint64_t mtime;                 //Time last modified

    
    
/////// MISC ///////
public:
    //Performs any necessary checks (e.g. directories check for multiple entries of the same node, etc...)
    virtual bool sanityCheck( void );
        
    //Since we don't want to have a pointer to the FS in _every_ _single_ node, we'll only have it in the root node
    virtual ShinyFilesystem * getFS();

protected:
    //Checks to make sure our parent has us as a child
    virtual bool check_parentHasUsAsChild( void );
    
    //Checks to make sure we don't have any duplicates in a list of inodes
    virtual bool check_noDuplicates( std::vector<ShinyMetaNode *> * list, const char * listName );
};

#endif