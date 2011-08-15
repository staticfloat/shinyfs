//
//  ShinyMetaNode.cpp
//  ShinyMetafs-tracker
//
//  Created by Elliot Nabil Saba on 5/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "ShinyMetaNode.h"
#include "ShinyMetaDir.h"
#include "ShinyMetaFilesystem.h"
#include <base/Logger.h>

ShinyMetaNode::ShinyMetaNode(ShinyMetaFilesystem * fs, const char * newName) {
    //Save pointer to fs
    this->fs = fs;
    
    //Generate a new inode for this node
    this->inode = fs->genNewInode();
    
    //Add this Node into the inode map
    this->fs->nodes[this->inode] = this;
    
    //Set name
    this->name = NULL;
    this->setName( newName );
    
    //It's HAMMAH TIME!!!
    this->ctime = this->atime = this->mtime = time(NULL);
    
    LOG( "Does FUSE initialize default permissions?\n" );
    this->setPermissions( 0x1f );   //0b111111111
    
    //Set these to nothing right now
    LOG( "need to init uid/gid!\n" );
    this->uid = this->gid = NULL;
}

ShinyMetaNode::ShinyMetaNode( const char * serializedInput, ShinyMetaFilesystem * fs ) : fs( fs ) {
    this->unserialize(serializedInput);
    
    //Add this Node into the inode map
    this->fs->nodes[this->inode] = this;
}

ShinyMetaNode::~ShinyMetaNode() {
    if( this->name )
        delete( this->name );
}

inode_t ShinyMetaNode::getInode() {
    return this->inode;
}

const inode_t ShinyMetaNode::getParent( void ) {
    return this->parent;
}

void ShinyMetaNode::setParent( inode_t newParent ) {
    this->parent = newParent;
}


void ShinyMetaNode::setName( const char * newName ) {
    if( this->name )
        delete( this->name );
    
    this->name = new char[strlen(newName)+1];
    strcpy( this->name, newName );
    
    fs->setDirty();
}

const char * ShinyMetaNode::getName( void ) {
    return (const char *)this->name;
}

const char * ShinyMetaNode::getNameCopy( void ) {
    char * nameCopy = new char[strlen(this->name)+1];
    strcpy( nameCopy, this->name );
    return nameCopy;
}

const char * ShinyMetaNode::getPath() {
    //First, get the length:
    uint64_t len = 0;

    //We'll store all the paths in here so we don't have to traverse the tree multiple times
    std::list<const char *> paths;
    paths.push_front( this->getName() );
    len += strlen( this->getName() );
    
    ShinyMetaNode * currNode = this;
    while( currNode != (ShinyMetaNode *) fs->root ) {
        //Move up in the chain of parents
        ShinyMetaNode * nextNode = fs->findNode( currNode->getParent() );
        if( !nextNode )
            return this->getNameCopy();
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

    len = 0;
    for( std::list<const char *>::iterator itty = paths.begin(); itty != paths.end(); ++itty ) {
        path[len++] = '/';
        strcpy( path + len, *itty );
        len += strlen( *itty );
    }
    return path;
}

void ShinyMetaNode::setPermissions( uint16_t newPermissions ) {
    this->permissions = newPermissions;
    fs->setDirty();
}

uint16_t ShinyMetaNode::getPermissions() {
    return this->permissions;
}

bool ShinyMetaNode::check_existsInFs( std::list<inode_t> * list, const char * listName ) {
    bool retVal = true;
    std::list<inode_t>::iterator itty = list->begin();
    while( itty != list->end() ) {
        if( !this->fs->findNode( *itty ) ) {
            const char * path = this->getPath();
            WARN( "Orphaned member of %s pointing to [%llu], belonging to %s [%llu]", listName, *itty, path, this->inode );
            std::list<inode_t>::iterator delItty = itty++;
            list->erase( delItty );
            retVal = false;
        } else
            ++itty;
    }
    return retVal;
}

bool ShinyMetaNode::check_parentsHaveUsAsChild( void ) {
    //First, check to make sure the parent node exists.....
    ShinyMetaDir * parentNode = (ShinyMetaDir *)fs->findNode( this->parent );
    if( parent ) {
        //Iterate through all children, looking for us
        const std::list<inode_t> * children = parentNode->getListing();
        for( std::list<inode_t>::const_iterator cItty = children->begin(); cItty != children->end(); ++cItty ) {
            if( *cItty == this->inode )
                return true;
        }
        //If we can't find ourselves, FIX IT!
        const char * path = this->getPath();
        const char * parentPath = parentNode->getPath();
        WARN( "Parent %s [%llu] did not point to child %s [%llu], when it should have!  Add us as a child!\n", parentPath, parentNode->getInode(), path, this->inode );
        parentNode->addNode( this );
        delete( path );
        delete( parentPath );
    } else {
        ERROR( "parent [%d] of node %s [%d] could not be found in fs!", this->parent, this->name, this->inode );
        return false;
    }
    return true;
}

bool ShinyMetaNode::check_noDuplicates( std::list<inode_t> * list, const char * listName ) {
    bool retVal = true;
    std::list<inode_t>::iterator itty = list->begin();
    std::list<inode_t>::iterator last_iterator = itty++;
    while( itty != list->end() ) {
        if( *itty == *last_iterator ) {
            const char * path = this->getPath();
            const char * listPath = NULL;
            ShinyMetaNode * listNode = this->fs->findNode( *itty );
            if( listNode )
                listPath = listNode->getPath();

            WARN( "Warning, %s for node %s [%d] has duplicate entries for %s [%d] in it!", listName, path, this->inode, listPath, *itty );
            delete( listPath );
            delete( path );
            
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
        const char * path = this->getPath();
        WARN( "permissions == 0 for %s", path );
        delete( path );
        retVal = false;
    }
    
    retVal &= this->check_parentsHaveUsAsChild();
    return retVal;
}

uint64_t ShinyMetaNode::serializedLen() {
    //First, the size of the inode pointers
    uint64_t len = sizeof(inode);

    //Time markers
    len += sizeof(ctime) + sizeof(atime) + sizeof(mtime);
    
    //Then, permissions and user/group ids
    len += sizeof(uid) + sizeof(gid) + sizeof(permissions);
    
    //Number of parents + parent inodes
    len += sizeof(uint64_t) + sizeof(inode_t);
    
    //Finally, filename
    len += strlen(name) + 1;
    return len;
}

/* Serialization order is as follows:
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
    write_and_increment( this->inode, inode_t );
    write_and_increment( this->ctime, uint64_t );
    write_and_increment( this->atime, uint64_t );
    write_and_increment( this->mtime, uint64_t );
    write_and_increment( this->uid, uint32_t );
    write_and_increment( this->gid, uint32_t );
    write_and_increment( this->permissions, uint16_t );
    write_and_increment( this->parent, inode_t );
    
    //Finally, write out a \0-terminated string of the filename
    strcpy( output, this->name );
}

#define read_and_increment( value, type )   value = *((type *)input); input += sizeof(type)

void ShinyMetaNode::unserialize( const char * input ) {
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
}

ShinyNodeType ShinyMetaNode::getNodeType( void ) {
    return SHINY_NODE_TYPE_NODE;
}