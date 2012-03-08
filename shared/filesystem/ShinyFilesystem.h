#pragma once
#ifndef ShinyFilesystem_H
#define ShinyFilesystem_H

#include <tr1/unordered_map>
#include "ShinyMetaNode.h"
#include "ShinyUser.h"

/*
 This is it; the big kahunah.  This is the main object that applications should deal with;
 When created, it should connect to the network, update the metadata, spawn off threads to
 deal with flushing, multithreaded communication, etc....
 
 Users should just have to ask for files, and it will transparently go get them and give 'em back.
 Write locks, all of that, should be completely invisible to the user, except in failure.
 
 ShinyMetaFiles and Dirs will transparently ask for write locks and all that.
 /**/

#define SHINYFS_VERSION     3


class ShinyMetaDir;
class ShinyMetaFile;
class ShinyMetaRootDir;
class ShinyFilesystem {
    //This grants these classes the power to requisition new inodes, set the fs as dirty, etc....
    friend class ShinyMetaNode;
    friend class ShinyMetaFile;
    friend class ShinyMetaDir;
    friend class ShinyMetaRootDir;
    
/////// INITIALIZATION/SAVING LOADING ///////
public:
    //Loads from/saves out to filecache, and connects up with peerAddr
    ShinyFilesystem( const char * filecache );
    
    //Obligatory cleanup chump
    ~ShinyFilesystem();
    
    //Serializes the entire tree into a bytestream, returning the length of said stream
    size_t serialize( char ** output );
    
    //Calls flush() on every node and such to get rid of any temporary data,
    //write stuff out to disk, etc. etc.... then if serializeAndSave is true, serializes and stores
    //out to shinycache/fs.cache. This should be called regularly, to make sure everything is safe
    void flush( bool serializeAndSave = true );

    //Returns filecache location
    const char * getFilecache( void );
protected:
    //Sets parameters for the fs, either generated or garnered from the Communicator
    void initialize( inode_t nextInode = 0, inode_t inodeRange = INODE_MAX );
private:
    //Helper function to unserialize from some input garnered from the Communicator
    void unserialize( const char * input );


    
/////// NODE ROUTINES ///////
public:
    //Find node frmo inode, returns NULL on not found
    ShinyMetaNode * findNode( inode_t inode );

    //Same as above, but first resolves a path to an inode
    ShinyMetaNode * findNode( const char * path );
    
    //Finds the parent node of the file at path, where the file does not need exist
    ShinyMetaDir * findParentNode( const char * path );
protected:
    //Called by ShinyMetaNode when a new one is created.
    inode_t genNewInode();
private:
    //Helper function for searching nodes that belong to a parent
    ShinyMetaNode * findMatchingChild( ShinyMetaDir * parent, const char * childName, uint64_t childNameLen );
    
    
    
/////// DATA ///////
public:
    //Performs various checks on the tree, making sure everything is in order
    bool sanityCheck();
    
    //Returns the revision this tree is at, (increments with every write)
    uint64_t getRevision();
    
    //Debug call to print out filesystem recursively.  Uses printDir(), etc....
    void print( void );
    void printDir( ShinyMetaDir * dir, const char * prefix );
    
    //Returns the version of this ShinyFS
    const uint64_t getVersion();
protected:
private:
    //All files are stored in this enormous map.  The key is the inode (unique id) of each file.
    //directories just give lists of inodes rather than pointers to ShinyMetaNodes.
    std::tr1::unordered_map<inode_t, ShinyMetaNode *> nodes;
    
    //Here is the map of users
    std::map<user_t, ShinyUser> users;

    //The root dir.  Come on, what do you want from me?!
    ShinyMetaRootDir * root;

    //nextInode to pass out with genNewInode()
    inode_t nextInode;
    
    //The maximum amount of inodes I can pass out with nextInode before I have to ask for more....
    inode_t inodeRange;
    
    //Stores the location of all cached files, etc...
    char * filecache;
};

#define FSCACHE_POSTFIX     "fs.cache"
#define KEY_LOCATION         "~/.ssh/id_rsa"


#endif //SHINYFILESYSTEM_H