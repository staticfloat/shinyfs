#pragma once
#ifndef ShinyMetaNode_H
#define ShinyMetaNode_H

#include <stdint.h>
#include <list>
#include <vector>
#include "ShinyMetaNodeSnapshot.h"

class ShinyFilesystem;
class ShinyMetaDir;
class ShinyMetaNode {
/////// SNAPSHOT ///////
protected:
    ShinyMetaNodeSnapshot snapshot;
    
/////// CREATION ///////
public:
    // Generate a new node with the given name, and default everything else
    ShinyMetaNode( const char * newName, ShinyMetaDir * parent );

    // Initialize this node, from serialized data.  Get amount of data read in via serializeLen() afterwards
    ShinyMetaNode( const char ** serializedInput, ShinyMetaDir * parent );

    // Clean up (free name, etc...)
    ~ShinyMetaNode();
        
    // Returns the length of a serialized verion of this node
    virtual uint64_t serializedLen( void );
    
    // Serializes into the buffer [output], returns output shifted forward by serializedLen
    virtual char * serialize( char * output );
    
    // Called by ShinyMetaNode() to load from a serialized string, shifts input by this->serializedLen()
    // Can also be called to "update" a node after a change has been made to it, by ShinyFilesystemMediator
    virtual void unserialize( const char **input );


/////// ATTRIBUTES ///////
public:
    // Name (filename, directory name, etc....)
    const char * getName();
    void setName( const char * newName );

    // Set new permissions for this node
    const uint16_t getPermissions();
    void setPermissions( uint16_t newPermissions);
    
    // chown() anyone?
    const uint64_t getUID( void );
    void setUID( const uint64_t newUID );
    const uint64_t getGID( void );
    void setGID( const uint64_t newGID );
    
    // Get/set parent
    ShinyMetaDir * getParent( void );
    void setParent( ShinyMetaDir * newParent );
    
    // Gets the absolute path to this node (If parental chain is broken, returns a question mark at the last
    // good known position, e.g. "?/dir 1/dir 2/file") I'm.... not sure how that can happen outside of nasty
    // race conditions or data corruption, but it's good to be prepared
    const char * getPath();
    
    // Gets the FS associated with this node
    ShinyFilesystem * const getFS();
    
    // Accessed (read), Changed (metadata), Modified (file data) times
    // Birthed time can't be changed (obviously) and modifying filedata modifies metadata
    const uint64_t get_btime( void );
    const uint64_t get_atime( void );
    const uint64_t get_ctime( void );
    const uint64_t get_mtime( void );
    
    void set_atime( void );
    void set_ctime( void );
    void set_mtime( void ); // Note; implicitly calls set_ctime!
    
    // These set the respective times to the given new time
    void set_atime( const uint64_t new_atime );
    void set_ctime( const uint64_t new_ctime );
    void set_mtime( const uint64_t new_mtime ); // Note; implicitly calls set_ctime!
    
    // Returns a snapshot of this node
    ShinyMetaNodeSnapshot * getSnapshot();
    
    // Returns the node type, e.g. if it's a file, directory, etc.
    virtual ShinyMetaNodeSnapshot::NodeType getNodeType( void );

/////// MISC ///////
public:
    typedef ShinyMetaDir parentType;
/*
    //Performs any necessary checks (e.g. directories check for multiple entries of the same node, etc...)
    virtual bool sanityCheck( void );

protected:
    // Checks to make sure our parent has us as a child
    bool check_parentHasUsAsChild( void );
    
    // Checks to make sure we don't have any duplicates in a list of inodes
    bool check_noDuplicates( std::vector<ShinyMetaNode *> * list, const char * listName );
    
/////// UTIL ///////
public:
    // returns permissions of a file in rwxrwxrwx style, in a shared [char *] buffer (shared across threads)
    static const char * permissionStr( uint16_t permissions );
    
    // returns the filename of a full path (e.g. everything past the last "/")
    static const char * basename( const char * path );
*/
};

#endif