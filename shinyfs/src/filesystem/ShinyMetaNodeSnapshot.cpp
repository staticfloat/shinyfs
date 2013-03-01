#include "ShinyMetaNodeSnapshot.h"
#include "ShinyMetaDirSnapshot.h"
#include "ShinyFilesystem.h"

#include "base/Logger.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

ShinyMetaNodeSnapshot::ShinyMetaNodeSnapshot() {
    // This only used when creating a new node that isn't just a Snapshot
}

ShinyMetaNodeSnapshot::ShinyMetaNodeSnapshot( ShinyMetaNodeSnapshot * node ) {
    // Copy everything in, memcpy-style
    memcpy( (void *)this, (void *)node, sizeof(ShinyMetaNodeSnapshot) );
    
    // Fix the one pointer in the class that needs to be unique, the name
    this->name = new char[strlen(node->getName())+1];
    strcpy( this->name, node->getName() );
}

ShinyMetaNodeSnapshot::ShinyMetaNodeSnapshot( const char ** serializedInput, ShinyMetaDirSnapshot * newParent ) : name(NULL) {
    this->unserialize( serializedInput );
    this->parent = newParent;
    newParent->addNode( this );
}

ShinyMetaNodeSnapshot::~ShinyMetaNodeSnapshot() {
    if( this->name ) {
        LOG( "Deleting %s", this->name );
        delete( this->name );
    }
    
    // Remove myself from my parent (Note that if we're a snapshot, delete should only be called from the parent)
    if( this->getParent() )
        this->getParent()->delNode( this );
}

uint64_t ShinyMetaNodeSnapshot::serializedLen() {
    // Size of us
    uint64_t len = 0;
    
    //Time markers
    len += sizeof(btime) + sizeof(atime) + sizeof(ctime) + sizeof(mtime);
    
    //Then, permissions and user/group ids
    len += sizeof(uid) + sizeof(gid) + sizeof(permissions);
    
    //Finally, filename
    len += strlen(name) + 1;
    return len;
}

/* Serialization order is as follows:
 
 [btime]         - uint64_t
 [atime]         - uint64_t
 [ctime]         - uint64_t
 [mtime]         - uint64_t
 [uid]           - uint64_t
 [gid]           - uint64_t
 [permissions]   - uint16_t
 [name]          - char* (\0 terminated)
 */

#define write_and_increment( value, type ) \
    *((type *)output) = value; \
    output += sizeof(type)

char * ShinyMetaNodeSnapshot::serialize(char * output) {
    write_and_increment( this->btime, uint64_t );
    write_and_increment( this->atime, uint64_t );
    write_and_increment( this->ctime, uint64_t );
    write_and_increment( this->mtime, uint64_t );
    write_and_increment( this->uid, uint64_t );
    write_and_increment( this->gid, uint64_t );
    write_and_increment( this->permissions, uint16_t );
    
    //Finally, write out a \0-terminated string of the filename
    uint64_t i=0;
    while( this->name[i] ) {
        output[i] = this->name[i];
        ++i;
    }
    output[i] = 0;
    return output + i + 1;
}

#define read_and_increment( value, type ) \
    value = *((type *)input); \
    input += sizeof(type)

void ShinyMetaNodeSnapshot::unserialize( const char ** input_double ) {
    const char * input = *input_double;
    read_and_increment( this->btime, uint64_t );
    read_and_increment( this->atime, uint64_t );
    read_and_increment( this->ctime, uint64_t );
    read_and_increment( this->mtime, uint64_t );
    read_and_increment( this->uid, uint64_t );
    read_and_increment( this->gid, uint64_t );
    read_and_increment( this->permissions, uint16_t );
    
    // Rather than doing even MORE strlen()'s, we'll only do one, and save the result
    uint64_t nameLen = strlen(input) + 1;
    
    // This is because we update nodes by unserializing into an already created node
    if( this->name )
        delete( this->name );
    this->name = new char[nameLen];
    memcpy( this->name, input, nameLen );
    
    // Note that [this->parent] is untouched by this method!
    
    // Return with the extra shift by nameLen
    *input_double = input + nameLen;
}


////////////////////////////////////////////////////////////////////////////////////////////
///////////////////                   ATTRIBUTE STUFF                    ///////////////////
////////////////////////////////////////////////////////////////////////////////////////////

ShinyMetaDirSnapshot * ShinyMetaNodeSnapshot::getParent( void ) {
    return this->parent;
}

const char * ShinyMetaNodeSnapshot::getName() {
    return this->name;
}

const uint16_t ShinyMetaNodeSnapshot::getPermissions( void ) {
    return this->permissions;
}

const uint64_t ShinyMetaNodeSnapshot::getUID( void ) {
    return this->uid;
}

const uint64_t ShinyMetaNodeSnapshot::getGID( void ) {
    return this->gid;
}

const char * ShinyMetaNodeSnapshot::getPath( void ) {
    return this->getFS()->getNodePath( this );
}

const uint64_t ShinyMetaNodeSnapshot::get_btime( void ) {
    return this->btime;
}

const uint64_t ShinyMetaNodeSnapshot::get_atime( void ) {
    return this->atime;
}

const uint64_t ShinyMetaNodeSnapshot::get_ctime( void ) {
    return this->ctime;
}

const uint64_t ShinyMetaNodeSnapshot::get_mtime( void ) {
    return this->mtime;
}

ShinyMetaNodeSnapshot::NodeType ShinyMetaNodeSnapshot::getNodeType( void ) {
    // This shouldn't ever really happen, we're never dealing with this base class directly
    return ShinyMetaNodeSnapshot::TYPE_NODE;
}

ShinyFilesystem * const ShinyMetaNodeSnapshot::getFS() {
    // So irresponsible, always asking your parent to do it for you!
    if( this->getParent() )
        return this->getParent()->getFS();
    return NULL;
}

uint16_t ShinyMetaNodeSnapshot::getDefaultPermissions( void ) {
    return S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;
}

////////////////////////////////////////////////////////////////////////////////////////////
///////////////////                      UTIL STUFF                      ///////////////////
////////////////////////////////////////////////////////////////////////////////////////////

// This follows the `ls -l` output guidelines
const char * ShinyMetaNodeSnapshot::permissionStr( uint16_t permissions ) {
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


const char * ShinyMetaNodeSnapshot::basename( const char * path ) {
    uint64_t i = strlen(path);
    // Search backwards through path
    for( uint64_t i = strlen(path); i > 0; ++i ) {
        if( path[i] == '/' )
            return path + i + 1;
    }
    // There was no slash, we'll just return the path then
    return path;
}

