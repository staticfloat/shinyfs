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
    // The types of nodes, used when determining whether a node is a dir or a file, and
    // used extensively during (un)serialization
    enum NodeType {
        // The most generic kind of "node", the abstract base class (unused)
        TYPE_NODE,
        
        // A standard file, with data, and whatnot
        TYPE_FILE,

        // A special construct used to provide thread-safe read/write access to files, used by ShinyFilesystemMediator
        TYPE_FILEHANDLE,
        
        // A directory with nodes "underneath" it, etc...
        TYPE_DIR,
        
        // A special case of the above, where the main difference is that the root nodes' parent is itself
        TYPE_ROOTDIR,

        // The number of types (I just always do this out of habit)
        NUM_NODE_TYPES,
    };


/////// CREATION ///////
public:
    // Generate a new node with the given name, and default everything else
    ShinyMetaNode( const char * newName );

    // Initialize this node, from serialized data.  Get amount of data read in via serializeLen() afterwards
    ShinyMetaNode( const char ** serializedInput );

    // Clean up (free name, etc...)
    ~ShinyMetaNode();
    
    // Returns the length of a serialization on this node
    virtual uint64_t serializedLen( void );
    
    // Serializes into the buffer [output], returns output shifted forward by serializedLen
    virtual char * serialize( char * output );

    // Called by ShinyMetaNode() to load from a serialized string, shifts input by this->serializedLen()
    // Can also be called to "update" a node after a change has been made to it, by ShinyFilesystemMediator
    virtual void unserialize( const char **input );    

//////// ATTRIBUTES ///////
public:    
    // Name (filename, directory name, etc....)
    virtual void setName( const char * newName );
    virtual const char * getName();
    
    // Set new permissions for this node
    virtual void setPermissions( uint16_t newPermissions);
    virtual uint16_t getPermissions();
    
    // chown() anyone?
    virtual void setUID( const uint64_t newUID );
    virtual const uint64_t getUID( void );
    virtual void setGID( const uint64_t newGID );
    virtual const uint64_t getGID( void );
    
    // Get/set parent
    virtual ShinyMetaDir * getParent( void );
    virtual void setParent( ShinyMetaDir * newParent );
    
    // Gets the absolute path to this node (If parental chain is broken, returns a question mark at the last
    // good known position, e.g. "?/dir 1/dir 2/file") I'm.... not sure how that can happen, but it probably can
    virtual const char * getPath();
    
    // These return the respective times
    virtual const uint64_t get_atime( void );
    virtual const uint64_t get_ctime( void );
    virtual const uint64_t get_mtime( void );
    
    // These set the respective times to the current time
    virtual void set_atime( void );
    virtual void set_ctime( void );
    virtual void set_mtime( void );
    
    // These set the respective times to the given new time
    virtual void set_atime( const uint64_t new_atime );
    virtual void set_ctime( const uint64_t new_ctime );
    virtual void set_mtime( const uint64_t new_mtime );
    
    // Returns the node type, e.g. if it's a file, directory, etc.
    virtual ShinyMetaNode::NodeType getNodeType( void );
protected:
    // Parent of this node
    ShinyMetaDir * parent;
    
    // Filename (+ extension)
    char * name;
    
    // file permissions (rwx rwx rwx, 9 bits wasting 16 in my precious, precious memory!)
    uint16_t permissions;
    
    // User/Group IDs (not implemented yet)
    uint64_t uid;
    uint64_t gid;
    
    // Time created
    uint64_t ctime;
    // Time accessed
    uint64_t atime;
    // Time modified
    uint64_t mtime;
    
/////// MISC ///////
public:
    //Performs any necessary checks (e.g. directories check for multiple entries of the same node, etc...)
    virtual bool sanityCheck( void );
        
    //Since we don't want to have a pointer to the FS in _every_ _single_ node, we'll only have it in the root node
    virtual ShinyFilesystem * getFS();

protected:
    // Checks to make sure our parent has us as a child
    virtual bool check_parentHasUsAsChild( void );
    
    // Checks to make sure we don't have any duplicates in a list of inodes
    virtual bool check_noDuplicates( std::vector<ShinyMetaNode *> * list, const char * listName );
    
    // returns the "default" permissions for a new file
    virtual uint16_t getDefaultPermissions();
};

#endif