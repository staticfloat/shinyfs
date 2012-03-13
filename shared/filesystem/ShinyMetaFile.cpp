//
//  ShinyMetaFile.cpp
//  ShinyMetafs-tracker
//
//  Created by Elliot Nabil Saba on 5/25/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "ShinyMetaFile.h"
#include "ShinyFilesystem.h"
#include "ShinyMetaDir.h"
#include <base/Logger.h>

#define min( x, y ) ((x) > (y) ? (y) : (x))

ShinyMetaFile::ShinyMetaFile( ShinyFilesystem * fs, const char * newName ) : ShinyMetaNode( fs, newName ) {
    this->setLen( 0 );
}

ShinyMetaFile::ShinyMetaFile( ShinyFilesystem * fs, const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( fs, newName ) {
    this->setLen( 0 );
    parent->addNode( (ShinyMetaNode *)this );
}

ShinyMetaFile::ShinyMetaFile( const char * serializedInput, ShinyFilesystem * fs ) : ShinyMetaNode( serializedInput, fs ) {
    this->unserialize( serializedInput + ShinyMetaNode::serializedLen() );
}

ShinyMetaFile::~ShinyMetaFile() {
}

void ShinyMetaFile::setLen( uint64_t newLen ) {
    this->fileLen = newLen;
    TODO( "Communicate with the cache that the size has changed" );
}

uint64_t ShinyMetaFile::getLen() {
    return this->fileLen;
}

uint64_t ShinyMetaFile::read( uint64_t offset, char * buffer, uint64_t len ) {
    TODO( "talk to ShinyCache here!" );
}

uint64_t ShinyMetaFile::write( uint64_t offset, const char * buffer, uint64_t len ) {
    TODO( "talk to ShinyCache here!" );
}


bool ShinyMetaFile::sanityCheck( void ) {
    //First, the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();

    return retVal;
}

size_t ShinyMetaFile::serializedLen( void ) {
    /*
    //Start off with the basic length
    size_t len = ShinyMetaNode::serializedLen();
    
    //add on sizeof number of chunks
    len += sizeof(size_t);
    
    //add on hashes for each chunk
    len += sizeof(Chunkhash)*this->chunks.size();
    
    //Add on length of last chunk
    len += sizeof(chunklen_t);
    return len;
     */
    return 0;
}

void ShinyMetaFile::serialize( char *output ) {
    /*
    //First serialize out the basic stuff into output
    ShinyMetaNode::serialize(output);
    
    //Now, add the ShinyFileChunks on;
    size_t len = ShinyMetaNode::serializedLen();
    
    //First, store the number of chunks;
    *((size_t *)(output + len)) = this->chunks.size();
    len += sizeof(size_t);
    
    if( !this->chunks.empty() ) {
        //Next, store out the file hashes
        for( size_t i=0; i<this->chunks.size(); ++i ) {
            memcpy( output + len, this->chunks[i]->getHash(), sizeof(Chunkhash) );
            len += sizeof(Chunkhash);
        }
        
        //Finally, store the last chunk's size
        *((chunklen_t *)(output + len)) = this->chunks[this->chunks.size()-1]->getChunkLen();
    }
     */
}

void ShinyMetaFile::unserialize( const char * input ) {
    /*
    size_t len = 0;

    //Let's get the number of chunks:    
    size_t numChunks = *((size_t *)(input + len));
    len += sizeof(size_t);

    if( numChunks ) {
        //reserve that in memory so that we don't over-allocate
        this->chunks.resize( numChunks );
        
        for( size_t i=0; i<numChunks-1; ++i ) {
            this->chunks[i] = new ShinyFileChunk( this, i, ShinyFileChunk::MAX_CHUNK_LEN,  *((Chunkhash *)(input + len)) );
            len += sizeof(Chunkhash);
        }
    
        //Special case for the last one, as it has a different file length
        Chunkhash * lastHash = ((Chunkhash *)(input + len));
        len += sizeof(Chunkhash);
        this->chunks[numChunks-1] = new ShinyFileChunk( this, numChunks - 1, *((chunklen_t *)(input + len)), *lastHash );
    }
    */
}

ShinyMetaNode::NodeType ShinyMetaFile::getNodeType( void ) {
    return ShinyMetaNode::TYPE_FILE;
}