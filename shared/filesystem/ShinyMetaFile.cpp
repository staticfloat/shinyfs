//
//  ShinyMetaFile.cpp
//  ShinyMetafs-tracker
//
//  Created by Elliot Nabil Saba on 5/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "ShinyMetaFile.h"
#include "ShinyMetaFilesystem.h"
#include "ShinyMetaDir.h"
#include <base/Logger.h>

ShinyMetaFile::ShinyMetaFile( ShinyMetaFilesystem * fs, const char * newName ) : ShinyMetaNode( fs, newName ) {
    this->setFileLen( 0 );
    memset( this->hash, NULL, sizeof(Filehash) );
}

ShinyMetaFile::ShinyMetaFile( ShinyMetaFilesystem * fs, const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( fs, newName ) {
    this->setFileLen( 0 );
    memset( this->hash, NULL, sizeof(Filehash) );
    parent->addNode( this );
}

ShinyMetaFile::ShinyMetaFile( const char * serializedInput, ShinyMetaFilesystem * fs ) : ShinyMetaNode( serializedInput, fs ) {
    this->unserialize( serializedInput + ShinyMetaNode::serializedLen() );
    
}

void ShinyMetaFile::setFileLen( inode_t newLen ) {
    this->filelen = newLen;
    fs->setDirty();
}

uint64_t ShinyMetaFile::getFileLen() {
    return this->filelen;
}


bool ShinyMetaFile::sanityCheck( void ) {
    //First, the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();
    
    LOG( "Do file integrity checking here for %s [%d]", this->getPath(), this->inode );
    
    return retVal;
}


uint64_t ShinyMetaFile::serializedLen( void ) {
    //Start off with the basic length
    uint64_t len = ShinyMetaNode::serializedLen();
    
    //Tack on the added stuff from len
    len += sizeof(uint64_t);
    
    return len;
}

void ShinyMetaFile::serialize( char *output ) {
    //First serialize out the basic stuff into output
    ShinyMetaNode::serialize(output);
    
    //Then, pop filelen onto the end
    uint64_t len = ShinyMetaNode::serializedLen();
    *((uint64_t *)(output + len)) = this->filelen;
}

void ShinyMetaFile::unserialize( const char * input ) {
    //Just pop filelen off of the end
    uint64_t len = ShinyMetaNode::serializedLen();
    this->filelen = *((uint64_t *)(input + len));
}

ShinyNodeType ShinyMetaFile::getNodeType( void ) {
    return SHINY_NODE_TYPE_FILE;
}