	//
//  ShinyMetaNode.cpp
//  ShinyMetafs-tracker
//
//  Created by Elliot Nabil Saba on 5/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "ShinyMetaNode.h"
#include "ShinyMetaDir.h"
#include "ShinyMetaFile.h"
#include "ShinyFilesystem.h"
#include <base/Logger.h>
#include <sys/stat.h>
#include <time.h>

ShinyMetaNode::ShinyMetaNode( const char * newName ) {
    // Set name. If NULL, then we have no name...... D:
    this->name = NULL;
    if( newName )
        this->setName( newName );
    
    //Set parent to NULL for now
    this->parent = NULL;
    
    //It's HAMMAH TIME!!!
    this->ctime = this->atime = this->mtime = time(NULL);
    
    // Set default permissions
    this->setPermissions( this->getDefaultPermissions() );
    
    // Does this work?
    this->uid = getuid();
    this->gid = getgid();
    
    TODO( "Still need to work in modification times and stuch" );
}

ShinyMetaNode::ShinyMetaNode( const char ** serializedInput ) {
    this->unserialize(serializedInput);
}

ShinyMetaNode::~ShinyMetaNode() {
    if( this->name ) {
        LOG( "Deleting %s", this->name );
        delete( this->name );
    }
    
    //Remove myself from my parent
    if( this->getParent() )
        this->getParent()->delNode( this );
}


uint64_t ShinyMetaNode::serializedLen() {
    // Size of us
    uint64_t len = 0;

    //Time markers
    len += sizeof(ctime) + sizeof(atime) + sizeof(mtime);
    
    //Then, permissions and user/group ids
    len += sizeof(uid) + sizeof(gid) + sizeof(permissions);
    
    //Finally, filename
    len += strlen(name) + 1;
    return len;
}

/* Serialization order is as follows:
 
 [ctime]         - uint64_t
 [atime]         - uint64_t
 [mtime]         - uint64_t
 [uid]           - uint32_t
 [gid]           - uint32_t
 [permissions]   - uint16_t
 [name]          - char* (\0 terminated)
 */

#define write_and_increment( value, type )    *((type *)output) = value; output += sizeof(type)

char * ShinyMetaNode::serialize(char * output) {
    write_and_increment( this->ctime, uint64_t );
    write_and_increment( this->atime, uint64_t );
    write_and_increment( this->mtime, uint64_t );
    write_and_increment( this->uid, uint64_t );
    write_and_increment( this->gid, uint64_t );
    write_and_increment( this->permissions, uint16_t );
    
    //Finally, write out a \0-terminated string of the filename
    uint64_t i=0;
    while( this->name[i] ) {
        output[i] = this->name[i];
        i += 1;
    }
    output[i] = 0;
    return output + i + 1;
}

#define read_and_increment( value, type )   value = *((type *)input); input += sizeof(type)

void ShinyMetaNode::unserialize( const char ** input_double ) {
    const char * input = *input_double;
    read_and_increment( this->ctime, uint64_t );
    read_and_increment( this->atime, uint64_t );
    read_and_increment( this->mtime, uint64_t );
    read_and_increment( this->uid, uint64_t );
    read_and_increment( this->gid, uint64_t );
    read_and_increment( this->permissions, uint16_t );
    
    // Rather than doing even MORE strlen()'s, we'll only do one, and save the result
    uint64_t nameLen = strlen(input) + 1;
    this->name = new char[nameLen];
    memcpy( this->name, input, nameLen );
    
    // Note that parent is untouched by this method!
    
    // Return with the extra shift by nameLen
    *input_double = input + nameLen;
}

ShinyMetaDir * ShinyMetaNode::getParent( void ) {
    return this->parent;
}

void ShinyMetaNode::setParent( ShinyMetaDir * newParent ) {
    // Purge any cached node paths that we might have previously had
    if( this->getParent() )
        delete( this->getFS()->nodePaths[this] );

    this->parent = newParent;    
}

void ShinyMetaNode::setName( const char * newName ) {
    if( this->name )
        delete( this->name );
    
    uint64_t len = strlen(newName) + 1;
    this->name = new char[len];
    memcpy( this->name, newName, len );
}

const char * ShinyMetaNode::getName( void ) {
    return (const char *)this->name;
}

void ShinyMetaNode::setPermissions( uint16_t newPermissions ) {
    this->permissions = newPermissions;
}

uint16_t ShinyMetaNode::getPermissions( void ) {
    return this->permissions;
}

void ShinyMetaNode::setUID( const uint64_t newUID ) {
    this->uid = newUID;
}

const uint64_t ShinyMetaNode::getUID( void ) {
    return this->uid;
}

void ShinyMetaNode::setGID( const uint64_t newGID ) {
    this->gid = newGID;
}

const uint64_t ShinyMetaNode::getGID( void ) {
    return this->gid;
}

const char * ShinyMetaNode::getPath( void ) {
    return this->getFS()->getNodePath( this );
}

const uint64_t ShinyMetaNode::get_atime( void ) {
    return this->atime;
}

const uint64_t ShinyMetaNode::get_ctime( void ) {
    return this->ctime;
}

const uint64_t ShinyMetaNode::get_mtime( void ) {
    return this->mtime;
}

void ShinyMetaNode::set_atime( void ) {
    this->set_atime( time(NULL) );
}

void ShinyMetaNode::set_ctime( void ) {
    this->set_ctime( time(NULL) );
}

void ShinyMetaNode::set_mtime( void ) {
    this->set_mtime( time(NULL) );
}

void ShinyMetaNode::set_atime( const uint64_t new_atime ) {
    this->atime = new_atime;
}

void ShinyMetaNode::set_ctime( const uint64_t new_ctime ) {
    this->atime = new_ctime;
}

void ShinyMetaNode::set_mtime( const uint64_t new_mtime ) {
    this->atime = new_mtime;
}

bool ShinyMetaNode::check_parentHasUsAsChild( void ) {
    //Iterate through all children of our parent, looking for us
    const std::vector<ShinyMetaNode *> children = *this->getParent()->getNodes();
    for( uint64_t i = 0; i<children.size(); ++i ) {
        if( children[i] == this )
            return true;
    }
    //If we can't find ourselves, COMPLAIN!
    WARN( "Parent %s did not point to child %s, when it should have!\n", this->getParent()->getPath(), this->getPath() );
    return false;
}

bool ShinyMetaNode::check_noDuplicates( std::vector<ShinyMetaNode *> * list, const char * listName ) {
    bool retVal = true;
    
    //Because the vectors are sorted, we only need check ourselves against the people right after us
    std::vector<ShinyMetaNode *>::iterator itty = list->begin();
    std::vector<ShinyMetaNode *>::iterator last_iterator = itty++;
    while( itty != list->end() ) {
        if( *itty == *last_iterator ) {
            WARN( "Warning, %s for node %s has duplicate entries for %s in it!", listName, this->getPath(), (*itty)->getPath() );
            list->erase( last_iterator );
            last_iterator = itty++;
            retVal = false;
        } else
            ++itty;
    }
    return retVal;
}

bool ShinyMetaNode::sanityCheck() {
    bool retVal = true;
    if( permissions == 0 ) {
        WARN( "permissions == 0 for %s", this->getPath() );
        retVal = false;
    }
    
    retVal &= this->check_parentHasUsAsChild();
    return retVal;
}

ShinyFilesystem * ShinyMetaNode::getFS() {
    // So irresponsible, ah. Just ask your parent to do it for you!
    if( this->getParent() )
        return this->getParent()->getFS();
    return NULL;
}

ShinyMetaNode::NodeType ShinyMetaNode::getNodeType( void ) {
    return ShinyMetaNode::TYPE_NODE;
}

uint16_t ShinyMetaNode::getDefaultPermissions() {
    return S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH;
}




////////////////////////////////////////////////////////////////////////////////////////////
///////////////////                      UTIL STUFF                      ///////////////////
////////////////////////////////////////////////////////////////////////////////////////////

// This follows the `ls -l` output guidelines
const char * printPermissions( uint16_t permissions ) {
    // The static buffer we return every time
    static char str[10] = {0,0,0,0,0,0,0,0,0,0};

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


const char * ShinyMetaNode::basename( const char * path ) {
    uint64_t slashIdx = 0;
    uint64_t i = 0;
    bool slashFound = false;
    while( path[i] != 0 ) {
        if( path[i] == '/' ) {
            slashIdx = i;
            slashFound = true;
        }
        i++;
    }
    
    if( !slashFound )
        return path;
    return path + slashIdx + 1;
}

