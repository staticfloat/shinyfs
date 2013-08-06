#include "Nodes.h"
#include <time.h>
#include <unistd.h>
#include <stdint.h>

using namespace ShinyFS;

NodeSnapshot::NodeSnapshot(const char * newName, DirSnapshot * const newParent) {
    // This is used to actually create a new Node, it's never used when this is _just_ a snapshot,
    // it's only used when we're creating a new node.
    uint64_t nameLen = strlen(newName) + 1;
    name = new char[strlen(newName)+1];
    memcpy( name, newName, nameLen );

    // Initialize parent
    if( newParent ) {
        parent = newParent;
        parent->addNode( this );
    }

    // It's HAMMAH TIME!!!
    btime = atime = ctime = mtime = getCurrTime();

    // Set default permissions
    permissions = getDefaultPermissions();

    // NABIL: Change this to use a UserMap or whatever
    uid = getuid();
    gid = getgid();
}

NodeSnapshot::NodeSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent ) : name(NULL) {
    // Because vtable's don't update until construction is done, we need to call update manually in each constructor
    update( serializedStream, true );
    
    parent = newParent;
    parent->addNode( this );
}

NodeSnapshot::NodeSnapshot( NodeSnapshot * const clone ) {
    // First the name
    uint64_t nameLen = strlen(clone->getName()) + 1;
    name = new char[strlen(clone->getName())+1];
    memcpy( name, clone->getName(), nameLen );

    // Next the parent
    parent = clone->getParent();

    // Times
    btime = clone->getBtime();
    atime = clone->getAtime();
    ctime = clone->getCtime();
    mtime = clone->getMtime();

    // permissions
    permissions = clone->getPermissions();

    // Even the UID/GID!
    uid = clone->getUID();
    gid = clone->getGID();  
}

NodeSnapshot::~NodeSnapshot() {
    // Clear out any cached node paths first
    RootDirSnapshot * root = this->getRoot();
    if( root )
        root->clearCachedNodePath( this );

    // Remove myself from my parent (Note that if we're a snapshot, delete should only be called from the parent)
    if( getParent() ) {
        getParent()->delNode( this );        
        parent = NULL;
    }

    if( name ) {
        delete[] name;
        name = NULL;
    }
}


uint64_t NodeSnapshot::serializedLen() {
    // Start out with the length of the flag/NodeType
    uint64_t len = sizeof(uint8_t);
    
    //Time markers
    len += 4*sizeof(TimeStruct);
    
    //Then, permissions and user/group ids
    len += sizeof(uid) + sizeof(gid) + sizeof(permissions);
    
    //Finally, filename
    len += strlen(name) + 1;
    return len;
}

/* Serialization order is as follows:
 
 [type]          - uint8_t
 [btime]         - uint64_t + uint32_t
 [atime]         - uint64_t + uint32_t
 [ctime]         - uint64_t + uint32_t
 [mtime]         - uint64_t + uint32_t
 [uid]           - uint64_t
 [gid]           - uint64_t
 [permissions]   - uint16_t
 [name]          - char* (\0 terminated)
 */

// A helper function that writes out a value, and increments the pointer by the width of the type
#define write_and_increment( output, value, type ) \
    *((type *)output) = value; \
    output += sizeof(type)

void NodeSnapshot::serialize( uint8_t ** output_ptr ) {
    // Get an address to write out to
    uint8_t * output = *output_ptr;

    write_and_increment( output, this->getNodeType(), uint8_t );
    write_and_increment( output, this->btime, TimeStruct );
    write_and_increment( output, this->atime, TimeStruct );
    write_and_increment( output, this->ctime, TimeStruct );
    write_and_increment( output, this->mtime, TimeStruct );
    write_and_increment( output, this->uid, uint64_t );
    write_and_increment( output, this->gid, uint64_t );
    write_and_increment( output, this->permissions, uint16_t );
    
    //Finally, write out a \0-terminated string of the filename
    uint64_t nameLen = strlen(this->name) + 1;
    memcpy( output, this->name, nameLen );

    // Store the address of the next available byte into output_ptr
    *output_ptr = output + nameLen;
}

#define read_and_increment( input, value, type ) \
    value = *((type *)input); \
    input += sizeof(type)

void NodeSnapshot::update( const uint8_t ** input_ptr, bool skipSelf ) {
    // Get the address of the input we're going to read in from
    const char * input = (const char *) *input_ptr;

    read_and_increment( input, this->btime, TimeStruct );
    read_and_increment( input, this->atime, TimeStruct );
    read_and_increment( input, this->ctime, TimeStruct );
    read_and_increment( input, this->mtime, TimeStruct );
    read_and_increment( input, this->uid, uint64_t );
    read_and_increment( input, this->gid, uint64_t );
    read_and_increment( input, this->permissions, uint16_t );
    
    // Rather than doing even MORE strlen()'s, we'll only do one, and save the result
    uint64_t nameLen = strlen(input) + 1;
    
    // This is because we update nodes by unserializing into an already created node
    if( this->name )
        delete[] this->name;
    this->name = new char[nameLen];
    memcpy( this->name, input, nameLen );
    
    // Note that [this->parent] is untouched by this method!
    // There are two cases in which this method is called:
    //   * Creation from a serialized stream: In which case the constructor will take care of parent
    //   * Updating from a serialized stream: In which case our parent doesn't change!
    
    // Return with the extra shift by nameLen
    *input_ptr = (const uint8_t *) (input + nameLen);
}


NodeSnapshot * NodeSnapshot::unserialize( const uint8_t ** input, DirSnapshot * const parent ) {
    uint8_t type = (*input)[0];
    *input += sizeof(uint8_t);

    switch( type ) {
        case TYPE_DIR: {
            DirSnapshot * ret = new DirSnapshot( input, parent );
            //ret->update( input );
            return ret;
        }
        case TYPE_DIRSTUMP:
            return new DirStumpSnapshot( input, parent );
        case TYPE_FILE:
            return new FileSnapshot( input, parent );
        default:
            WARN( "Unknown node type %d\n", type );
    }
    return NULL;
}


// Name (filename, directory name, etc....)
const char * NodeSnapshot::getName() {
    return this->name;
}

// Get the permissions (e.g. rwxrwxrwx)
const uint16_t NodeSnapshot::getPermissions() {
    // Only return the first 9 bits
    return this->permissions & 0x01ff;
}

// Get the user and group ID (e.g. 501:501)
const uint64_t NodeSnapshot::getUID( void ) {
    return this->uid;
}
const uint64_t NodeSnapshot::getGID( void ) {
    return this->gid;
}

// Get the directory that is the parent of this node
DirSnapshot * NodeSnapshot::getParent( void ) {
    return this->parent;
}

// Gets the absolute path to this node (If parental chain is broken, returns a question mark at the last
// good known position, e.g. "?/dir 1/dir 2/file") I'm.... not sure how that can happen outside of nasty
// race conditions or data corruption, but it's good to be prepared.
const char * NodeSnapshot::getPath() {
    RootDirSnapshot * root = this->getRoot();
    return getPath( root );
}

const char * NodeSnapshot::getPath( RootDirSnapshot * root ) {
    // If we can find a root, check for cached results:
    if( root ) {
        const char * path = root->findCachedNodePath( this );

        // If the cached result exists, return it!
        if( path )
            return path;
    }

    // If we can't find our root, or we haven't generated this path before, let's start generating!
    // First, we must calculate the total length of this path:
    uint64_t len = 0;
    
    // We'll store all the paths in here so we don't have to traverse the tree multiple times
    std::list<const char *> paths;
    paths.push_front( this->getName() );
    len += strlen( this->getName() );
    
    // Now iterate up the tree, gathering the names of parents and shoving them into the list of paths
    // Make sure to stop once we reach a NULL node (broken parental chain) or we hit the root node
    NodeSnapshot * currNode = this;
    while( dynamic_cast<NodeSnapshot *>(currNode->getParent()) != currNode ) {
        // Move up in the chain of parents
        DirSnapshot * nextNode = currNode->getParent();

        // If our parental chain is broken, just return ?/name
        if( !nextNode ) {
            WARN( "Parental chain for %s is broken at %s!", paths.back(), currNode->getName() );
            paths.push_front( "?" );
            break;
        } else {
            // Push node parent's path onto the list, and add its length to len
            paths.push_front( nextNode->getName() );
            len += strlen( nextNode->getName() );
        }
        currNode = nextNode;
    }

    // If our last element was the root, go ahead and pop it since we don't need it anywho
    if( root )
        paths.pop_front();
    
    // Add 1 for each slash in front of each path
    len += paths.size();

    // If it's a directory, add an extra slash
    if( this->getNodeType() == TYPE_DIR || this->getNodeType() == TYPE_DIRSTUMP )
        len += 1;

    // Add another 1 for the NULL character
    char * path = new char[len+1];
    path[len] = 0;
    
    // Write out each element of [paths] preceeded by forward slashes
    len = 0;
    for( std::list<const char *>::iterator itty = paths.begin(); itty != paths.end(); ++itty ) {
        path[len++] = '/';
        strcpy( path + len, *itty );
        len += strlen( *itty );
    }
    if( this->getNodeType() == TYPE_DIR || this->getNodeType() == TYPE_DIRSTUMP )
        path[len++] = '/';

    // Finally, push this path into root's node cache (if it exists)
    if( root )
        root->storeCachedNodePath( path, this );
    else
        ERROR( "root missing, so this path (%s) is going to leak!" );

    return path;
}

// Gets the FS associated with this node
// Note that getFS is virtual, as if I am actuall a RootDir, I really don't want to keep recursing
RootDirSnapshot * const NodeSnapshot::getRoot() {
    DirSnapshot * myParent = this->getParent();
    if( myParent )
        return myParent->getRoot();
    return NULL;
}

// Birthed, Accessed (read), Changed (metadata), Modified (file data) times
const TimeStruct NodeSnapshot::getBtime( void ) {
    return this->btime;
}
const TimeStruct NodeSnapshot::getAtime( void ) {
    return this->atime;
}
const TimeStruct NodeSnapshot::getCtime( void ) {
    return this->ctime;
}
const TimeStruct NodeSnapshot::getMtime( void ) {
    return this->mtime;
}

// Returns the node type, e.g. if it's a file, directory, etc.
const NodeType NodeSnapshot::getNodeType( void ) {
    // This should never happen, as we should be overridden at all times!
    return TYPE_NODE;
}

// Returns the default permissions for a new file of this type
uint16_t NodeSnapshot::getDefaultPermissions() {
    // rw-r--r--
    return S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;
}

const char * NodeSnapshot::permissionStr( uint16_t permissions ) {
    // The static buffer we return every time
    static char str[10] = {'-','-','-','-','-','-','-','-','-','-'};

    // First, the "user" (or "owner") triplet
    str[1] = permissions & S_IRUSR ? 'r' : '-';
    str[2] = permissions & S_IWUSR ? 'w' : '-';
    if( permissions & S_IXUSR ) {
        if( permissions & S_ISUID )
            str[3] = 's';
        else
            str[3] = 'x';
    } else {
        if( permissions & S_ISUID )
            str[3] = 'S';
        else
            str[3] = '-';
    }

    // Next, the "group" triplet:
    str[3] = permissions & S_IRGRP ? 'r' : '-';
    str[4] = permissions & S_IWGRP ? 'w' : '-';
    if( permissions & S_IXGRP ) {
        if( permissions & S_ISGID )
            str[3] = 's';
        else
            str[3] = 'x';
    } else {
        if( permissions & S_ISGID )
            str[3] = 'S';
        else
            str[3] = '-';
    }

    // Finally, the "other" triplet:
    str[6] = permissions & S_IROTH ? 'r' : '-';
    str[7] = permissions & S_IWOTH ? 'w' : '-';
    if( permissions & S_IXOTH ) {
        if( permissions & S_ISVTX )
            str[8] = 't';
        else
            str[8] = 'x';
    } else {
        if( permissions & S_ISVTX )
            str[8] = 'T';
        else
            str[8] = '-';
    }

    return str;
}

// returns the filename of a full path (e.g. everything past the last "/")
const char * NodeSnapshot::basename( const char * path ) {
    uint64_t i = strlen(path);
    // Search backwards through path
    for( uint64_t i = strlen(path); i > 0; ++i ) {
        if( path[i] == '/' )
            return path + i + 1;
    }
    // There was no slash, we'll just return the path then
    return path;
}

TimeStruct NodeSnapshot::getCurrTime() {
    // If we're on Linux:
    static struct timespec t;

    // We can speed this up by using CLOCK_REALTIME_COARSE
#ifdef _DEBUG
    clock_gettime( CLOCK_REALTIME, &t );
#else
    clock_gettime( CLOCK_REALTIME_COARSE, &t );
#endif
    
    return TimeStruct( t.tv_sec, t.tv_nsec );
}