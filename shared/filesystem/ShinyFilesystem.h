#pragma once
#ifndef ShinyFilesystem_H
#define ShinyFilesystem_H

#include <tr1/unordered_map>
#include "ShinyMetaNode.h"

/*
 * Le old shiny filesystem.  A tree of metadata information, with all the logic therein
 
 ShinyFilesystem is the "master" metadata located in the nodes.
 Clients will just pull the latest revision filesystem from the tracker when they connect, and then get delta updates as they go.
 
 
 
 Things I need to do/investigate:
    - I need to do my own permissions validation, of course, but how does _setting permissions_ work via FUSE?
 */

class ShinyMetaDir;
class ShinyMetaFile;
class ShinyMetaRootDir;
class ShinyFilesystem {
    //This grants these classes the power to requisition new inodes, set the fs as dirty, etc....
    friend class ShinyMetaNode;
    friend class ShinyMetaFile;
    friend class ShinyMetaDir;
    friend class ShinyMetaRootDir; 
public:
    static const uint16_t VERSION = 1;
    
    //Loads the filesystem from disk, saving to that same filename
    ShinyFilesystem( const char * serializedData = NULL, const char * filecache = "shinycache/" );
    ~ShinyFilesystem();
    
    /*******************
     PUBLIC API
     *******************/
    //Finds node associated with a path.  path _must_ be absolute!
    //returns NULL on file not found
    ShinyMetaNode * findNode( const char * path );
    
    //Finds node associated with an inode
    //returns NULL on file not found
    ShinyMetaNode * findNode( inode_t inode );
    
    //Finds the parent node of the file at path
    //NOTE that path does not necessarily exist!
    ShinyMetaDir * findParentNode( const char * path );
    
    //Note that there is no adding/removing of nodes; this is because you should do that manually,
    //by finding the ShinyMetaNode and then removing it directly.  Those classes will automagically callback to this filesystem
    
    //Returns the revision this tree is at, (increments with every write)
    uint64_t getRevision();

    //Performs various checks on the tree, making sure everything is in order
    bool sanityCheck();

    //Called by ShinyNode
    bool isDirty( void );
    
    //Calls flush() on every node and such to get rid of any temporary data,
    //write stuff out to disk, etc. etc.... then serializes and stores out to shinycache/fs.cache
    //This should be called regularly, e.g. once every minute or so to make sure everything is happy
    void flush( bool serializeAndSave = true );

    //Serializes the entire tree into a bytestream, returning the length of said stream
    uint64_t serialize( char ** output );
    
    //Debug call to print out filesystem recursively.  Uses printDir(), etc....
    void print( void );
    void printDir( ShinyMetaDir * dir, const char * prefix );
protected:
    //Called by ShinyMetaNode when a new one is created.
    inode_t genNewInode();
    
    //Called by ShinyMetaNode whenever a change is made, notifying that the tree has changed somehow.
    void setDirty( void );
    
    //All files are stored in this enormous map.  The key is the inode (unique id) of each file.
    //directories just give lists of inodes rather than pointers to ShinyMetaNodes.
    std::tr1::unordered_map<inode_t, ShinyMetaNode *> nodes;
    
    //Returns filecache location
    const char * getFilecache( void );
    
    //Returns location of fscachefilename
    const char * getFsCacheFilename();
private:
    //Helper function to unserialize
    void unserialize( const char * input );

    //Helper function for searching nodes that belong to a parent
    ShinyMetaNode * findMatchingChild( ShinyMetaDir * parent, const char * childName, uint64_t childNameLen );
    
    //The root dir.  Come on, what do you want from me?!
    ShinyMetaRootDir * root;

    //nextInode to pass out with genNewInode()
    inode_t nextInode;
    
    //Set to true on every write
    bool dirty;
    
    //Stores the location of all cached files, etc...
    char * filecache;
    
    //filecache + "/fs.cache"
    char * fscache;
};


#endif //SHINYFILESYSTEM_H