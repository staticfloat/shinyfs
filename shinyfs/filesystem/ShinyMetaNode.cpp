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
#include <unistd.h> // for getuid/gid()
#include <time.h>

ShinyMetaNode::ShinyMetaNode( const char * newName, ShinyMetaDir * parent ) : snapshot() {
    // This is used to actually create a new Node, it's never used when this is _just_ a snapshot,
    // it's only used when we're creating a new node.
    uint64_t nameLen = strlen(newName) + 1;
    snapshot.name = new char[strlen(newName)+1];
    memcpy( snapshot.name, newName, nameLen );
    
    // Initialize parent
    if( parent ) {
        snapshot.parent = dynamic_cast<ShinyMetaDirSnapshot *>(parent->getSnapshot());
        parent->addNode( this );
    } else {
        // This should only really happen when unserializing
        snapshot.parent = NULL;
    }
    
    // It's HAMMAH TIME!!!
    snapshot.btime = snapshot.atime = snapshot.ctime = snapshot.mtime = time(NULL);
    
    // Set default permissions
    snapshot.permissions = snapshot.getDefaultPermissions();
    
    // NABIL: Change this to use the ShinyUserMap or whatever
    snapshot.uid = getuid();
    snapshot.gid = getgid();
}

ShinyMetaNode::ShinyMetaNode( const char ** serializedInput, ShinyMetaDir * newParent ) : snapshot( serializedInput, dynamic_cast<ShinyMetaDirSnapshot *>(newParent->getSnapshot()) ) {
    snapshot.parent = dynamic_cast<ShinyMetaDirSnapshot *>(newParent->getSnapshot());
}

ShinyMetaNode::~ShinyMetaNode() {
    // Do nothing again!  (refactoring is _really_ weird)
}

ShinyMetaDir * ShinyMetaNode::getParent() {
    return dynamic_cast<ShinyMetaDir *>(snapshot.parent);
}

void ShinyMetaNode::setParent( ShinyMetaDir * newParent ) {
    // Purge any cached node paths that we might have previously had
    if( this->getParent() )
        delete( snapshot.getFS()->nodePaths[&snapshot] );

    snapshot.parent = dynamic_cast<ShinyMetaDirSnapshot *>(newParent->getSnapshot());
    this->set_ctime();
}

void ShinyMetaNode::setName( const char * newName ) {
    if( snapshot.name )
        delete( snapshot.name );
    
    uint64_t len = strlen(newName) + 1;
    snapshot.name = new char[len];
    memcpy( snapshot.name, newName, len );
    this->set_ctime();
}

void ShinyMetaNode::setPermissions( uint16_t newPermissions ) {
    snapshot.permissions = newPermissions;
    this->set_ctime();
}

void ShinyMetaNode::setUID( const uint64_t newUID ) {
    snapshot.uid = newUID;
    this->set_ctime();
}

void ShinyMetaNode::setGID( const uint64_t newGID ) {
    snapshot.gid = newGID;
    this->set_ctime();
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
    snapshot.atime = new_atime;
}

void ShinyMetaNode::set_ctime( const uint64_t new_ctime ) {
    snapshot.ctime = new_ctime;
}

void ShinyMetaNode::set_mtime( const uint64_t new_mtime ) {
    snapshot.mtime = new_mtime;
    snapshot.ctime = new_mtime;
}

/*
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
    
    TODO( "Verify this works");
    
    //Because the vectors are sorted, we only need check ourselves against the people right after us
    std::vector<ShinyMetaNode *>::iterator itty = list->begin();
    std::vector<ShinyMetaNode *>::iterator last_iterator = itty++;
    while( itty != list->end() ) {
        if( *itty == *last_iterator ) {
            WARN( "Warning, %s for node %s has duplicate entries for %s in it!", listName, this->getPath(), (*itty)->getPath() );
            list->erase( last_iterator );
            retVal = false;
        }
        last_iterator = itty++;
    }
    return retVal;
}

bool ShinyMetaNode::sanityCheck() {
    bool retVal = true;
    // The only check we ever do is to see if our parent has us as a child.  :P
    retVal &= this->check_parentHasUsAsChild();
    return retVal;
}
*/

