#pragma once
#ifndef SHINYMETANODE_H
#define SHINYMETANODE_H

#include <stdint.h>
#include <list>

/*
 A Node is both a directory and a file.  It is......... what ever you wish it to be
 
 This is where most of the magic of files is hidden.
 (auto-generating new inode numbers, handling permissions, etc)
 
 */

//Structure to aid in permissions handling
/*
typedef union  {
    typedef struct {
        bool r,w,x;
    } perm_struct;
    uint16_t block;
    struct {
        perm_struct user;   //User  read/write/execute permissions
        perm_struct group;  //Group read/write/execute permissions
        perm_struct other;  //Other read/write/execute permissions
    };
} ShinyMetaPermissions;
*/

typedef uint64_t inode_t;


//I should make these belong to ShinyMetaNode
enum ShinyNodeType {
    SHINY_NODE_TYPE_NODE,
    SHINY_NODE_TYPE_FILE,
    SHINY_NODE_TYPE_DIR,
    SHINY_NODE_TYPE_ROOTDIR,
    NUM_SHINY_NODE_TYPES,
};

class ShinyMetaFilesystem;
class ShinyMetaNode { 
public:
    //Generate a new node and create new inode (by asking ShinyMetaTree), etc.....
    ShinyMetaNode( ShinyMetaFilesystem * fs, const char * newName );
    
    //Initialize this node, from serialized data.  Get amount of data read in via serializeLen() afterwards
    ShinyMetaNode( const char * serializedInput, ShinyMetaFilesystem * fs );
    
    //Clean up (free name)
    ~ShinyMetaNode();
    
    //Name (filename, directory name, etc....)
    virtual void setName( const char * newName );
    virtual const char * getName();
    
    //Gets the absolute path to this node
    virtual const char * getPath();
    
    //Set new permissions for this node
    virtual void setPermissions( uint16_t newPermissions);
    virtual uint16_t getPermissions();
    
    //Get the unique id for this node
    virtual inode_t getInode();
    
    //Get the inode of the  parents of this node
    virtual const inode_t getParent( void );
    
    //Sets the parent of this node to newParent
    virtual void setParent( inode_t newParent );
        
    //Performs any necessary checks (e.g. directories check for multiple entries of the same node, etc...)
    virtual bool sanityCheck( void );
    
    //Returns the length of a serialization on this node
    virtual uint64_t serializedLen( void );
    
    //Serializes into the buffer [output]
    virtual void serialize( char * output );
    
    //Returns the node type, e.g. if it's a file, directory, etc.
    virtual ShinyNodeType getNodeType( void );
protected:
    //Called by ShinyMetaNode() to load from a serialized string
    virtual void unserialize( const char * input );
    
    //Checks to make sure all of list are actually in fs.  If they aren't, removes them as a parent
    virtual bool check_existsInFs( std::list<inode_t> * list, const char * listName );
    
    //Checks to make sure all parents have us as children
    virtual bool check_parentHasUsAsChild( void );
    
    //Checks to make sure we don't have any duplicates in a list of inodes
    virtual bool check_noDuplicates( std::list<inode_t> * list, const char * listName );
    
    //Pointer to fs so that we can traverse the tree if we must (and believe me, we must!)
    ShinyMetaFilesystem * fs;
    
    //Parent of this inode
    inode_t parent;
    
    //Filename (+ extension, if you so wish)
    char * name;
    
    //This is the path of the node, destroyed everytime the parent is set, and cached
    //whenever getPath() is called.
    char * path;
    
    //Unique identifier for this node, whether it be directory or file
    inode_t inode;
    
    //file permissions
    uint16_t permissions;
    
    //User/Group IDs (These will be managed by something higher-level)
    uint32_t uid;
    uint32_t gid;
    
    //These should all be maintained tracker-side
    uint64_t ctime;                 //Time created          (Set by tracker on initial client write())
    uint64_t atime;                 //Time last accessed    (Tracker notified by client on read())
    uint64_t mtime;                 //Time last modified    (Set by tracker on client write())
};

#endif