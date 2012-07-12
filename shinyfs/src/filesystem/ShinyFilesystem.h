#pragma once
#ifndef ShinyFilesystem_H
#define ShinyFilesystem_H

#include <tr1/unordered_map>

#include "ShinyMetaNode.h"
#include "ShinyDBWrapper.h"

/*
 This guy is responsible ONLY for management of the filesystem tree. Metadata, etc. are all directly
 under his purview. He subs out to kyoto cabinet (or leveldb) to get the "actual" filesystem data.
 */

#define SHINYFS_VERSION     4

class ShinyMetaDir;
class ShinyMetaFile;
class ShinyMetaRootDir;
class ShinyFilesystem {
    friend class ShinyMetaNode;
    friend class ShinyMetaFile;
    friend class ShinyMetaFileHandle;
    friend class ShinyMetaDir;
    friend class ShinyMetaRootDir;
    
/////// INITIALIZATION/SAVING LOADING ///////
public:
    //Creates the ShinyCache to do serving of cached content, and sets up a few zmq helper stuffs
    ShinyFilesystem( const char * filecache );
    
    //Obligatory cleanup chump
    ~ShinyFilesystem();
    
    // Serializes a subtree starting at start into a bytestream, returning the length of said stream
    // start defaults (when NULL) to the root node of the entire tree
    // recursive defaults to true, and denotes whether dirs should include subdirs
    uint64_t serialize( char ** output, ShinyMetaNode * start = NULL, bool recursive = true );
    
    // Basically calls serialize on the root, then sends it over to ShinyCache for saving
    void save();

    //Helper function to unserialize a tree (or subtree)
    ShinyMetaRootDir * unserialize( const char * input );
protected:
    // recursive helper function for unserialize
    ShinyMetaNode * unserializeTree( const char ** input );


/////// NODE ROUTINES ///////
public:
    // Find a node from its path
    ShinyMetaNode * findNode( const char * path );
    
    // Finds the parent node of the file at path, where the file does not need exist
    ShinyMetaDir * findParentNode( const char * path );
    
    // reconstructs the path of a node
    const char * getNodePath( ShinyMetaNode * node );
protected:
    // cached paths for nodes
    std::tr1::unordered_map<ShinyMetaNode *, const char *> nodePaths;
    
    // The root dir.  Come on, what do you want from me?!
    ShinyMetaRootDir * root;
private:
    // Helper function for searching nodes that belong to a parent
    ShinyMetaNode * findMatchingChild( ShinyMetaDir * parent, const char * childName, uint64_t childNameLen );
    
    
/////// FILECACHE ///////
protected:
    // Returns the DB object, (used for FileHandle and File to write and read, etc....)
    ShinyDBWrapper * getDB();
private:
    // The key used to store the ShinyFS tree when we serialize it
    const char * getShinyFilesystemDBKey();
    const char * getShinyFilesystemSizeDBKey();
    ShinyDBWrapper db;
    

/////// MISC ///////
public:
    //Performs various checks on the tree, making sure everything is in order
    bool sanityCheck();
    
    //Returns the revision this tree is at, (increments with every write)
    uint64_t getRevision();
    
    //Debug call to print out filesystem recursively.  Uses printDir(), etc....
    void print( void );
    void printDir( ShinyMetaDir * dir, const char * prefix = "" );
    
    //Returns the version of this ShinyFS
    const uint64_t getVersion();
};

#endif //SHINYFILESYSTEM_H