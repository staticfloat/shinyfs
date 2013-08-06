#pragma once
#ifndef ShinyMetaNodeSnapshot_H
#define ShinyMetaNodeSnapshot_H
#include <stdint.h>

/*
 Snapshots are frozen instances of nodes; they can be read, but not written to. They essentially wrap the methods you're allowed to call on the nodes proper, but don't implement the methods you're not allowed to call
 
 */

class ShinyFilesystem;
class ShinyMetaDirSnapshot;
class ShinyMetaNodeSnapshot {
/////// FRIENDS ///////
    friend class ShinyMetaNode;
    
/////// TYPEDEFS ///////
public:
    enum NodeType {
        // The most generic kind of "node", the abstract base class (Should never be used)
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
    // Make a copy off of another snapshot (or live node, because INHERITANCE!)
    ShinyMetaNodeSnapshot( ShinyMetaNodeSnapshot * node );
    
    // Make a copy from a serialized stream, with the given parent
    ShinyMetaNodeSnapshot( const char ** serializedStream, ShinyMetaDirSnapshot * newParent );
    
    // Cleanup (free name, etc...).  Note that almost always this should be invoked indirectly by deleting the parent!
    ~ShinyMetaNodeSnapshot();
    
    // Returns the length of a serialized verion of this node
    virtual uint64_t serializedLen( void );
    
    // Serializes into the buffer [output], returns output shifted forward by serializedLen
    virtual char * serialize( char * output );
    
    // Called by ShinyMetaNode() to load from a serialized string, shifts input by this->serializedLen()
    // Can also be called to "update" a node after a change has been made to it, by ShinyFilesystemMediator
    virtual void unserialize( const char **input );
protected:
    // Creation constructor, used only by ShinyMetaNode
    ShinyMetaNodeSnapshot();


    
/////// ATTRIBUTES ///////
// We define getters only, no setters, as we are just a snapshot
public:
    // Name (filename, directory name, etc....)
    const char * getName();
    
    // Get the permissions (e.g. rwxrwxrwx)
    const uint16_t getPermissions();
    
    // Get the user and group ID (e.g. 501:501)
    const uint64_t getUID( void );
    const uint64_t getGID( void );
    
    // Get the directory that is the parent of this node
    ShinyMetaDirSnapshot * getParent( void );
    
    // Gets the absolute path to this node (If parental chain is broken, returns a question mark at the last
    // good known position, e.g. "?/dir 1/dir 2/file") I'm.... not sure how that can happen outside of nasty
    // race conditions or data corruption, but it's good to be prepared
    const char * getPath();
    
    // Gets the FS associated with this node
    ShinyFilesystem * const getFS();
    
    // Birthed, Accessed (read), Changed (metadata), Modified (file data) times
    const uint64_t get_btime( void );
    const uint64_t get_atime( void );
    const uint64_t get_ctime( void );
    const uint64_t get_mtime( void );
    
    // Returns the node type, e.g. if it's a file, directory, etc.
    virtual ShinyMetaNodeSnapshot::NodeType getNodeType( void );
    
// The actual data members of this class
protected:
    // Parent of this node
    ShinyMetaDirSnapshot * parent;
    
    // Filename (+ extension)
    char * name;
    
    // file permissions (-rwxrwxrwx, 9 bits wasting 16 in my precious, precious memory!)
    uint16_t permissions;
    
    // User/Group IDs (not implemented yet)
    uint64_t uid, gid;
    
    // Time birthed, changed (metadata), accessed (read), modified (file data)
    uint64_t btime, ctime, atime, mtime;

/////// MISC ///////
protected:
    // Returns the default permissions for a new file of this type
    virtual uint16_t getDefaultPermissions();
    
    
    
/////// UTIL ///////
public:
    // returns permissions of a file in rwxrwxrwx style, in a shared [char *] buffer (shared across threads)
    static const char * permissionStr( uint16_t permissions );
    
    // returns the filename of a full path (e.g. everything past the last "/")
    static const char * basename( const char * path );

};



#endif //ShinyMetaNodeSnapshot_H