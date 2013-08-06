#pragma once
#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <stdio.h>
#include <base/Logger.h>
#include <inttypes.h>
#include <unordered_map>
#include <list>

#include "Config.h"
#include "BitStuff.h"
#include "DBWrapper.h"

namespace ShinyFS {
// Forward declarations
class DirSnapshot;
class RootDirSnapshot;
class Dir;

// Enumeration for all node types
enum NodeType {
    // The most generic kind of "node", the abstract base class (Should never be used)
    TYPE_NODE,

    // A standard file, with data, and whatnot
    TYPE_FILE,

    // A special construct used to provide thread-safe read/write access to files, used by FilesystemMediator
    TYPE_FILEHANDLE,

    // A directory with nodes "underneath" it, etc...
    TYPE_DIR,

    // A directory that doesn't have its children in memory/serialized with it
    TYPE_DIRSTUMP,

    // The number of types (Not terribly useful, but habits are good).  Sometimes
    NUM_NODE_TYPES,
};


// My replacement for timespec.  WHY? WHY did they make the nanosecond parameter 8 bytes wide?!  D:
struct TimeStruct {
    uint64_t s;     // Seconds since the epoch
    uint32_t ns;    // Nanoseconds into this second (Note to kernel devs:  You only need 4 bytes for this!)

    TimeStruct() {
        s = ns = 0;
    }

    TimeStruct( uint64_t s, uint32_t ns ) {
        this->s = s;
        this->ns = ns;
    }

    TimeStruct( const TimeStruct & other ) {
        s = other.s;
        ns = other.ns;
    }

    TimeStruct & operator = (const TimeStruct & other) {
        if( this != &other ) {
            s = other.s;
            ns = other.ns;
        }
        return *this;
    }

    TimeStruct & operator += (const TimeStruct & other)  {
        s += other.s;
        ns += other.ns;
        return *this;
    }

    TimeStruct & operator -= (const TimeStruct & other) {
        s -= other.s;
        ns -= other.ns;
        return *this;
    }

    TimeStruct operator + (const TimeStruct & other) const {
        return TimeStruct(*this) += other;
    }

    TimeStruct operator - (const TimeStruct & other) const {
        TimeStruct temp = *this;
        return temp -= other;
    }


    bool operator == (const TimeStruct &other) const {
        return s == other.s && ns == other.ns;
    }

    bool operator != (const TimeStruct &other) const {
        return s != other.s || ns != other.ns;
    }

    bool operator < (const TimeStruct &other) const {
        if( s < other.s )
            return true;
        if( s == other.s && ns < other.ns )
            return true;
        return false;
    }

    bool operator > (const TimeStruct &other) const {
        return other < *this;
    }

    bool operator <= (const TimeStruct & other) const {
        return !(*this > other);
    }

    bool operator >= (const TimeStruct & other) const {
        return !(*this < other);
    }
};


class NodeSnapshot {
friend class DirSnapshot;
friend class Dir;
friend class RootDirSnapshot;
friend class RootDir;
/////// CREATION AND SERIALIZATION ///////
protected:
    // Creation constructor, used only by Node when inside the Node( const char *, Dir * const ) constructor
    NodeSnapshot( const char * newName, DirSnapshot * const newParent );

    // Create from a serialized stream, with the given parent
    NodeSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent );

    // Copy constructor
    NodeSnapshot( NodeSnapshot * const other );

    // Cleanup (free name, etc...).  Note that this is not public!  That's because NodeSnapshots should
    // only be destroyed via DirSnapshots or by their Node counterparts!
    virtual ~NodeSnapshot();

public:
    // returns the length of a serialized version of this node
    virtual uint64_t serializedLen();

    // Serializes into the given buffer, shifts output forward by serializedLen
    virtual void serialize( uint8_t ** output );

    // Called by the serialized string constructor. Shifts input by serializedLen.  Can also be called
    // to "update" a node by incorporating external changes.  Note that you should never set "skipSelf" to true
    void update( const uint8_t ** input, bool skipSelf = false );

    // This is the public-facing factory method that will reconstitute any node from its serialized representation
    static NodeSnapshot * unserialize( const uint8_t ** input, DirSnapshot * const parent );


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
    DirSnapshot * getParent( void );

    // Gets the absolute path to this node (If parental chain is broken, returns a question mark at the last
    // good known position, e.g. "?/dir_1/dir_2/file").  In "normal" operation, this should not happen much.
    // Paths are cached inside RootDirs, therefore if the parental chain is unbroken calling this method lots
    // should be pretty fast.  Cached results are invalidated on parental change or destruction
    const char * getPath();

    // If we've already got a hold of the root object, no need to trace the parental tree again
    const char * getPath( RootDirSnapshot * root );

    // Gets the root dir of this filesystem tree
    // Note that getRoot is virtual, as if I am actually a RootDir, I really don't want to keep recursing
    virtual RootDirSnapshot * const getRoot();

    // Birthed, Accessed (read), Changed (metadata), Modified (file data) times
    const TimeStruct getBtime( void );
    const TimeStruct getAtime( void );
    const TimeStruct getCtime( void );
    const TimeStruct getMtime( void );

    // Returns the node type, e.g. if it's a file, directory, etc.
    virtual const NodeType getNodeType( void );

    // The actual data members of this class
protected:
    // Parent of this node
    DirSnapshot * parent;

    // Filename (+ extension)
    char * name;

    // file permissions (rwxrwxrwx, only 9 bits used by permissions)
	uint16_t permissions;

    // User/Group IDs (not implemented yet)
    uint64_t uid, gid;

    // Time birthed, changed (metadata), accessed (read), modified (file data)
    TimeStruct btime, ctime, atime, mtime;


/////// MISC ///////
protected:
    // Returns the default permissions for a new file of this type
    virtual uint16_t getDefaultPermissions();

    // Returns my TimeStruct object for the current time.  NOTE: Very platform-dependent! :P
    static TimeStruct getCurrTime();


/////// UTIL ///////
public:
    // returns permissions of a file in rwxrwxrwx style, in a shared [char *] buffer (shared across threads)
    static const char * permissionStr( uint16_t permissions );

    // returns the filename of a full path (e.g. everything past the last "/")
    static const char * basename( const char * path );
};





class Node : virtual public NodeSnapshot {
/////// CREATION AND SERIALIZATION ///////
public:
    // Generate a new node with the given name, and default everything else
    Node( const char * newName, Dir * const parent );
    virtual ~Node();

    // This is the public-facing factory method that will reconstitute any node from its serialized representation
    static Node * unserialize( const uint8_t ** input, Dir * const parent );

/////// SERIALIZIATION //////
protected:
    // Create from a serialized stream, with the given parent
    Node( const uint8_t ** serializedStream, Dir * const newParent );


/////// ATTRIBUTES ///////
public:
    // The corresponding setters for everything we defined in NodeSnapshot
    void setName( const char * newName );
    void setPermissions( uint16_t newPermissions);
    void setUID( const uint64_t newUID );
    void setGID( const uint64_t newGID );
    void setParent( Dir * newParent );

    // Reimplementing this guy as we're changing the return type
    Dir * getParent( void );

    void setAtime( void );
    void setCtime( void );
    void setMtime( void ); // Note: implicitly calls setCtime()!

    void setAtime( const TimeStruct new_atime );
    void setCtime( const TimeStruct new_ctime );
    void setMtime( const TimeStruct new_mtime ); // Note: implicitly calls setCtime()!
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class DirSnapshot : virtual public NodeSnapshot {
friend class NodeSnapshot;
friend class Node;
/////// CREATION AND SERIALIZATION ///////
protected:
    // Again, should only be done implicitly when creating a Dir()
    DirSnapshot( const char * newName, DirSnapshot * const newParent );

    // Create from a serialized stream, with the given parent
    DirSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent );

    // This method only to be invoked by Dir( serializedStream, newParent )
    DirSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent, bool onlyToBeUsedByDir );    

    // And this should only be done from another DirSnapshot or a Dir!
    virtual ~DirSnapshot();

public:
    // returns the length of a serialized version of this node
    virtual uint64_t serializedLen();

    // Serializes into the given buffer, shifts output forward by serializedLen
    virtual void serialize( uint8_t ** output );

    // Called by the serialized string constructor. Shifts input by serializedLen.  Can also be called
    // to "update" a node by incorporating external changes.  Note that you should never set "skipSelf" to true
    void update( const uint8_t ** input, bool skipSelf = false );


/////// NODE MANAGEMENT ///////
public:
    // Returns a directory listing
    void const getNodes( std::vector<NodeSnapshot *> * nodes );
    const std::vector<NodeSnapshot *> * const getNodesView( void );

    // Finds a node and returns it, NULL otherwise
    NodeSnapshot * findNode( const char * name );

    // Returns the number of children that belong to this dir
    uint64_t getNumNodes();
protected:
    // These are protected so only Filesystem and Nodes can use them
    virtual bool addNode( NodeSnapshot * newNode );
    virtual bool delNode( NodeSnapshot * delNode );

    // All of this dir's child nodes
    std::vector<NodeSnapshot *> nodes;


/////// MISC ///////
public:
    virtual const NodeType getNodeType( void );
protected:
    // Override the default permissions for directories, as we need to have execute set so that people can read!
    virtual uint16_t getDefaultPermissions( void );
};


class Dir : virtual public DirSnapshot, virtual public Node {
friend class NodeSnapshot;
friend class Node;
/////// CREATION AND SERIALIZATION //////
public:
    // Create an entirely new Dir
    Dir( const char * newName, Dir * parent );

    // We can destroy this from anywhere, as this is a "live" Dir
    virtual ~Dir();

    // Called by the serialized string constructor. Shifts input by serializedLen.  Can also be called
    // to "update" a node by incorporating external changes.  Note that you should never set "skipSelf" to true
    void update( const uint8_t ** input, bool skipSelf = false );

protected:
    // Create from a serialized stream, with the given parent
    Dir( const uint8_t ** serializedStream, Dir * newParent );



/////// NODE MANAGEMENT ///////
public:
    void const getNodes( std::vector<Node *> * nodes );
    Node * const findNode( const char * name );

    // NABIL: These used to take in NodeSnapshots, I THINK because I want to override the corresponding ones in
    // DirSnapshot..... perhaps I should just put both in?
    virtual bool addNode( Node * newNode );
    virtual bool delNode( Node * delNode );
};


class DirStumpSnapshot : virtual public NodeSnapshot {
friend class NodeSnapshot;
friend class Node;
/////// CREATION AND SERIALIZATION ///////
protected:
    DirStumpSnapshot( const char * newName, DirSnapshot * const newParent );

    // Create from a serialized stream, with the given parent
    DirStumpSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent );

    // As usual, destruction only happens from parents
    virtual ~DirStumpSnapshot();


public:
    // This creates a DirStump off of that clone!  Isn't that GREAT?!
    DirStumpSnapshot( DirSnapshot * const clone );

    // returns the length of a serialized version of this node
    virtual uint64_t serializedLen();

    // Serializes into the given buffer, shifts output forward by serializedLen
    virtual void serialize( uint8_t ** output );

    // Called by the serialized string constructor. Shifts input by serializedLen.  Can also be called
    // to "update" a node by incorporating external changes.  Note that you should never set "skipSelf" to true
    // That is only used internally by constructors to denote that NodeSnapshot::update has already been called
    void update( const uint8_t ** input, bool skipSelf = false );


/////// NODE MANAGEMENT ///////
public:
    // Returns the number of children that belong to this dir
    const uint64_t getNumNodes();
protected:
    // How many nodes this Dir has 
    uint64_t numNodes;


/////// MISC ///////
public:
    virtual const NodeType getNodeType( void );
};

// We have a Snapshot version, but there's no difference between that and a non-snapshot version.  :P
//typedef DirStumpSnapshot DirStump;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class FileSnapshot : virtual public NodeSnapshot {
friend class NodeSnapshot;
/////// CREATION AND SERIALIZATION ///////
protected:
    // Creation from a File object
    FileSnapshot( const char * newName, DirSnapshot * const newParent );

    // Create from a serialized stream, with the given parent
    FileSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent );

    // Called only by File, or a DirSnapshot
    virtual ~FileSnapshot();

public:
    // returns the length of a serialized version of this node
    virtual uint64_t serializedLen();

    // Serializes into the given buffer, shifts output forward by serializedLen
    virtual void serialize( uint8_t ** output );

    // Called by the serialized string constructor. Shifts input by serializedLen.  Can also be called
    // to "update" a node by incorporating external changes.  Note that you should never set "skipSelf" to true
    void update( const uint8_t ** input, bool skipSelf = false );


/////// ATTRIBUTES //////
public:
    // Returns the length of the file in bytes
    uint64_t getLen();
    
    // Blocks until task completion. Not multithread safe. Returns number of bytes read.
    virtual uint64_t read( uint64_t offset, char * data, uint64_t len );
protected:
    // This is the guy that does the real work, the above read() subs out to this guy,
    // and just grab the db object from the Filesystem, (which is why I have FileHandle for
    // when the Filesystem object is unreachable, but we have the db object at hand)
    uint64_t read( DBWrapper * db, const char * path, uint64_t offset, char * data, uint64_t len );
    
    // The length of this here file
    uint64_t fileLen;


/////// MISC ///////
public:
    virtual const NodeType getNodeType();
};



class File : virtual public FileSnapshot, virtual public Node {
friend class NodeSnapshot;
friend class Node;
//////// CREATION AND SERIALIZATION ///////
public:
    // Same as above, but adds this guy as a child to given parent (this is just for convenience, this just calls "addNode()" for you)
    File( const char * newName, Dir * const parent );
    virtual ~File();

protected:
    // Create from a serialized stream, with the given parent
    File( const uint8_t ** serializedStream, Dir * const newParent );
    

/////// ATTRIBUTES //////
public:
    // Set a new length for this file (is implicitly called by write())
    // truncates if newLen < getLen(), appends zeros if newLen > getLen()
    virtual void setLen( uint64_t newLen );

    // Blocks until task completion. Not multithread safe. Returns number of bytes read.
    virtual uint64_t read( uint64_t offset, char * data, uint64_t len );
    
    // Blocks until task completion. Should only be called from same thread as one that owns the
    // filesystem tree. Returns how many bytes we were able to read/write.
    virtual uint64_t write( uint64_t offset, const char * data, uint64_t len );

    // Blocks until task completion.  Convenience shortcut for appending strings (e.g. write with the help of strlen() and getLen())
    virtual uint64_t append_str( const char * data );
protected:
    // These are the peeps that do the real work, the above setLen() and write() sub out to thess guys,
    // and just grab the db object from the filesystem, (which is why I have FileHandle for when
    // the Filesystem object is unreachable, but we have the db object at hand)
    uint64_t write( DBWrapper * db, const char * path, uint64_t offset, const char * data, uint64_t len );
    void setLen( DBWrapper * db, const char * path, uint64_t newLen );
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class RootDirSnapshot : virtual public DirSnapshot {
friend class NodeSnapshot;
friend class Node;
/////// CREATION AND SERIALIZATION ///////
protected:
    // NABIL: Not entirely sure what to do here.  Do I need some kind of specification as to
    // what point in time this snapshot represents?
    RootDirSnapshot( const char * filecache_path );

    // Only called from RootDir
    virtual ~RootDirSnapshot();

/////// MISC //////
public:
    // Just return ourselves
    virtual RootDirSnapshot * const getRoot();

    // Return the wrapper of the database backing this whole operation
    DBWrapper * const getDB();

    
protected:
    // Returns a cached node path, or NULL if it does not exist
    const char * findCachedNodePath( NodeSnapshot * node );

    // Stores (or overwrites) a cached node path
    void storeCachedNodePath( const char * path, NodeSnapshot * node );

    // Clears a cached node path and frees the memory corresponding to its string
    void clearCachedNodePath( NodeSnapshot * node );

private:
    DBWrapper * db;

    // This should probably become a Least-Recently-Used cache rather than a simple map to avoid memory creep
    std::unordered_map<NodeSnapshot *, const char *> cachedNodePaths;

    // NABIL: Some kind of time marker to pass to db to specify what point in time we should be looking at
};



class RootDir : virtual public Dir, virtual public RootDirSnapshot {
/////// CREATION AND SERIALIZATION ///////
public:
    RootDir( const char * filecache_path );
    virtual ~RootDir();

};



}; // For Namespace
