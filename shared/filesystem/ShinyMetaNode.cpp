	//
//  ShinyMetaNode.cpp
//  ShinyMetafs-tracker
//
//  Created by Elliot Nabil Saba on 5/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "ShinyMetaNode.h"
#include "ShinyMetaDir.h"
#include "ShinyFilesystem.h"
#include <base/Logger.h>
#include <sys/stat.h>

ShinyMetaNode::ShinyMetaNode(ShinyFilesystem * fs, const char * newName) {
    // Set name (no constructor lists here, no siree!)
    this->name = NULL;
    this->setName( newName );
    
    //Set parent to NULL for now
    this->parent = NULL;
    
    //It's HAMMAH TIME!!!
    this->ctime = this->atime = this->mtime = time(NULL);
    
    // Set permissions to rwxr--r--
    this->setPermissions( S_IRWXU | S_IRGRP | S_IROTH );
    
    //Set these to nothing right now
    TODO( "need to init uid/gid!\n" );
    this->uid = this->gid = NULL;
}

ShinyMetaNode::ShinyMetaNode( const char * serializedInput, ShinyFilesystem * fs ) {
    this->unserialize(serializedInput);
}

ShinyMetaNode::~ShinyMetaNode() {
    if( this->name )
        delete( this->name );
    
    //Remove myself from my parent
    if( this->getParent() )
        this->getParent()->delNode( this );
}


size_t ShinyMetaNode::serializedLen() {
    //First, the size of the inode pointers
    size_t len = 0;
    /*
    //Time markers
    len += sizeof(ctime) + sizeof(atime) + sizeof(mtime);
    
    //Then, permissions and user/group ids
    len += sizeof(uid) + sizeof(gid) + sizeof(permissions);
    
    //parent inode
    len += sizeof(parent);
    
    //Finally, filename
    len += strlen(name) + 1;
     */
    return len;
}

/* Serialization order is as follows:
 NOTE THIS IS HORRIBLY OUT OF DATE EVERY SINCE I DESTROYED THE INODE CRAPPAGE
 
 [inode]         - uint64_t
 [ctime]         - uint64_t
 [atime]         - uint64_t
 [mtime]         - uint64_t
 [uid]           - uint32_t
 [gid]           - uint32_t
 [permissions]   - uint16_t
 [parents]       - uint64_t + uint64_t * num_parents
 [name]          - char * (\0 terminated)
 */

#define write_and_increment( value, type )    *((type *)output) = value; output += sizeof(type)

void ShinyMetaNode::serialize(char * output) {
    /*
    write_and_increment( this->ctime, uint64_t );
    write_and_increment( this->atime, uint64_t );
    write_and_increment( this->mtime, uint64_t );
    write_and_increment( this->uid, uint64_t );
    write_and_increment( this->gid, uint64_t );
    write_and_increment( this->permissions, uint16_t );
    write_and_increment( this->parent, inode_t );
    
    //Finally, write out a \0-terminated string of the filename
    strcpy( output, this->name );
    */
}

#define read_and_increment( value, type )   value = *((type *)input); input += sizeof(type)

void ShinyMetaNode::unserialize( const char * input ) {
    /*
    read_and_increment( this->inode, inode_t );
    read_and_increment( this->ctime, uint64_t );
    read_and_increment( this->atime, uint64_t );
    read_and_increment( this->mtime, uint64_t );
    read_and_increment( this->uid, uint32_t );
    read_and_increment( this->gid, uint32_t );
    read_and_increment( this->permissions, uint16_t );
    read_and_increment( this->parent, inode_t );
    
    this->name = new char[strlen(input) + 1];
    strcpy( this->name, input );
    
    if( this->path ) {
        delete( this->path );
        this->path = NULL;
    }
    */
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
    
    this->name = new char[strlen(newName)+1];
    strcpy( this->name, newName );
    
    TODO( "Change this to work on the parent!" );
    //fs->setDirty();
}

const char * ShinyMetaNode::getName( void ) {
    return (const char *)this->name;
}

void ShinyMetaNode::setPermissions( uint16_t newPermissions ) {
    this->permissions = newPermissions;
    TODO( "Change this to work on the parent!" );
    //fs->setDirty();
}

uint16_t ShinyMetaNode::getPermissions( void ) {
    return this->permissions;
}


// Gets the path of a ShinyMetaNode, "e.g. '/dir1/dir2/file'", returns "?/file" if parental chain is broken (how would that happen?)
const char * ShinyMetaNode::getPath( void ) {
    // First, grab ahold of that fs!
    ShinyFilesystem * fs = this->getFS();

    // If we've cached the result from a previous call, then just return that!
    std::tr1::unordered_map<ShinyMetaNode *, const char *>::iterator itty = fs->nodePaths.find( this );
    if( itty != fs->nodePaths.end() ) {
        return (*itty).second;
    }
    
    // First, get the length:
    uint64_t len = 0;
    
    // We'll store all the paths in here so we don't have to traverse the tree multiple times
    std::list<const char *> paths;
    paths.push_front( this->getName() );
    len += strlen( this->getName() );
    
    // Now iterate up the tree, gathering the names of parents and shoving them into the list of paths
    ShinyMetaNode * currNode = this;
    while( currNode != (ShinyMetaNode *) fs->root ) {
        // Move up in the chain of parents
        ShinyMetaDir * nextNode = currNode->getParent();
        
        // If our parental chain is broken, just return ?/name
        if( !nextNode ) {
            ERROR( "Parental chain for %s is broken at %s!", paths.back(), currNode->getName() );
            paths.push_front( "?" );
            break;
        }
        
        else if( nextNode != (ShinyMetaNode *)fs->root ) {
            //Push this parent's path onto the list, and add its length to len
            paths.push_front( nextNode->getName() );
            len += strlen( nextNode->getName() );
        }
        currNode = nextNode;
    }
    
    //Add 1 for each slash in front of each path
    len += paths.size();
    
    //Add another 1 for the NULL character
    char * path = new char[len+1];
    path[len] = NULL;
    
    // Write out each element of [paths] preceeded by forward slashes
    len = 0;
    for( std::list<const char *>::iterator itty = paths.begin(); itty != paths.end(); ++itty ) {
        path[len++] = '/';
        strcpy( path + len, *itty );
        len += strlen( *itty );
    }
    
    // Save this result into our cached nodePaths map and return it;
    return fs->nodePaths[this] = path;
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