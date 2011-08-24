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

#define min( x, y ) ((x) > (y) ? (y) : (x))

ShinyMetaFile::ShinyMetaFile( ShinyMetaFilesystem * fs, const char * newName ) : ShinyMetaNode( fs, newName ) {
    this->truncate( 0 );
    this->cachePrefix = NULL;
}

ShinyMetaFile::ShinyMetaFile( ShinyMetaFilesystem * fs, const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( fs, newName ) {
    this->truncate( 0 );
    this->cachePrefix = NULL;
    parent->addNode( (ShinyMetaNode *)this );
}

ShinyMetaFile::ShinyMetaFile( const char * serializedInput, ShinyMetaFilesystem * fs ) : ShinyMetaNode( serializedInput, fs ) {
    this->unserialize( serializedInput + ShinyMetaNode::serializedLen() );
    this->cachePrefix = NULL;
}

ShinyMetaFile::~ShinyMetaFile() {
    if( this->cachePrefix )
        delete( this->cachePrefix );
    for( uint64_t i=0; i<this->chunks.size(); ++i ) {
        this->chunks[i]->flush( true );
        delete( this->chunks[i] );
    }
}

void ShinyMetaFile::setName( const char * newName ) {
    ShinyMetaNode::setName( newName );
    if( this->cachePrefix ) {
        delete( this->cachePrefix );
        this->cachePrefix = NULL;
    }
}

void ShinyMetaFile::truncate( uint64_t newLen ) {
    uint64_t filelen = this->getFileLen();
    if( newLen < filelen ) {
        //First, erase all chunks after the end chunk
        this->chunks.erase( this->chunks.begin() + (newLen/ShinyFileChunk::MAX_CHUNK_LEN) + 1, this->chunks.end() );

        //Then, resize that ending chunk
        if( !this->chunks[this->chunks.size()-1]->truncate( (chunklen_t) (newLen - newLen/ShinyFileChunk::MAX_CHUNK_LEN) ) ) {
            ERROR( "Couldn't truncate chunk %s!", this->chunks[this->chunks.size()-1]->getFileChunkPath() );
        }
        fs->setDirty();
    } else if( newLen > filelen ) {
        //Make this resize chunks so that they are large enough (max size)
        uint64_t i = this->chunks.size();
        uint64_t newSize = ceil(((double)newLen)/ShinyFileChunk::MAX_CHUNK_LEN);
        this->chunks.resize( newSize );

        //Resize the possibly not super long chunk
        if( i )
            this->chunks[i-1]->truncate( (chunklen_t) min( newLen - filelen, ShinyFileChunk::MAX_CHUNK_LEN ) );
        if( newSize > i ) {
            //Now, create as many more as we need
            for(; i<newLen/ShinyFileChunk::MAX_CHUNK_LEN; ++i )
                this->chunks[i] = new ShinyFileChunk( this, i );
            if( newLen % ShinyFileChunk::MAX_CHUNK_LEN )
                this->chunks[i] = new ShinyFileChunk( this, i, (chunklen_t) (newLen % ShinyFileChunk::MAX_CHUNK_LEN) );
        }
        fs->setDirty();
    }
}

uint64_t ShinyMetaFile::getFileLen() {
    //File length is now implicit in ShinyFileChunk
    if( chunks.size() )
        return (chunks.size()-1)*ShinyFileChunk::MAX_CHUNK_LEN + chunks[chunks.size()-1]->getChunkLen();
    else
        return 0;
}


bool ShinyMetaFile::sanityCheck( void ) {
    //First, the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();

    //Then, we'll do sanity checks for all the chunks....
    for( uint64_t i=0; i<this->chunks.size(); ++i ) {
        //Only force flush if this chunk wasn't previously available
        bool forceFlush = !this->chunks[i]->isAvailable();
        
        //Perform the sanity check (which forces stuff into memory
        if( !this->chunks[i]->sanityCheck() ) {
            LOG( "sanity check failed for %s", this->chunks[i]->getFileChunkPath() );
            retVal = false;
        }
        
        //Force this chunk out to disk, clearing it from memory
        this->chunks[i]->flush( forceFlush );
    }

    return retVal;
}

const char * ShinyMetaFile::getCachePrefix( void ) {
    //If we already have the cache path..........uhh.......... "cached"....... then return that!
    if( this->cachePrefix )
        return this->cachePrefix;
    
    //otherwise, LAH'S DO EEET!
    const char * filecache = this->fs->getFilecache();
    
    // <filecache> + / + <name> + \0
    this->cachePrefix = new char[strlen(filecache) + 1 + strlen(this->name) + 1];
    sprintf( this->cachePrefix, "%s/%llu", filecache, this->inode );
    return this->cachePrefix;
}

uint64_t ShinyMetaFile::serializedLen( void ) {
    //Start off with the basic length
    uint64_t len = ShinyMetaNode::serializedLen();
    
    //add on sizeof number of chunks
    len += sizeof(uint64_t);
    
    //add on hashes for each chunk
    len += sizeof(Chunkhash)*this->chunks.size();
    
    //Add on length of last chunk
    len += sizeof(chunklen_t);
    return len;
}

void ShinyMetaFile::serialize( char *output ) {
    //First serialize out the basic stuff into output
    ShinyMetaNode::serialize(output);
    
    //Now, add the ShinyFileChunks on;
    uint64_t len = ShinyMetaNode::serializedLen();
    
    //First, store the number of chunks;
    *((uint64_t *)(output + len)) = this->chunks.size();
    len += sizeof(uint64_t);
    
    //Next, store out the file hashes
    for( uint64_t i=0; i<this->chunks.size(); ++i ) {
        memcpy( output + len, this->chunks[i]->getHash(), sizeof(Chunkhash) );
        len += sizeof(Chunkhash);
    }
    
    //Finally, store the last chunk's size
    *((chunklen_t *)(output + len)) = this->chunks[this->chunks.size()-1]->getChunkLen();
}

void ShinyMetaFile::unserialize( const char * input ) {
    uint64_t len = 0;

    //Let's get the number of chunks:    
    uint64_t numChunks = *((uint64_t *)(input + len));
    len += sizeof(uint64_t);

    if( numChunks ) {
        //reserve that in memory so that we don't over-allocate
        this->chunks.resize( numChunks );
        
        for( uint64_t i=0; i<numChunks-1; ++i ) {
            this->chunks[i] = new ShinyFileChunk( this, i, ShinyFileChunk::MAX_CHUNK_LEN,  *((Chunkhash *)(input + len)) );
            len += sizeof(Chunkhash);
        }
    
        //Special case for the last one, as it has a different file length
        Chunkhash * lastHash = ((Chunkhash *)(input + len));
        len += sizeof(Chunkhash);
        this->chunks[numChunks-1] = new ShinyFileChunk( this, numChunks - 1, *((chunklen_t *)(input + len)), *lastHash );
    }
}

void ShinyMetaFile::flush( void ) {
    //Pass it on to all the chunks
    for( uint64_t i=0; i<this->chunks.size(); ++i )
        this->chunks[i]->flush();
}

uint64_t ShinyMetaFile::read( uint64_t offset, char * buffer, uint64_t len ) {
    //First off, if we're _starting_ outside of the realm of operation, return zero
    uint64_t startChunk = offset/ShinyFileChunk::MAX_CHUNK_LEN;
    if( startChunk >= this->chunks.size() )
        return 0;
    
    //If we don't have the starting chunk, return zero
    if( !this->chunks[startChunk]->isAvailable() )
        return 0;
    
    //Accumulate data into buffer and dataRead
    uint64_t dataRead = 0;
    
    //Do this to get the offset into the first chunk, then we'll set it to zero afterward
    offset = offset % ShinyFileChunk::MAX_CHUNK_LEN;
    while( len - dataRead > 0 ) {
        //We'll just let the read() function truncate len - dataRead down for us
        dataRead += this->chunks[startChunk]->read( (chunklen_t)offset, buffer + dataRead, (chunklen_t) (len - dataRead) );
        
        //Just like I promised, set offset to 0 now
        offset = 0;
        
        //Move forward in the chunks, then check for existence/availability and ragequit if either fails
        startChunk++;
        if( startChunk >= this->chunks.size() || !this->chunks[startChunk]->isAvailable() )
            break;
    }
    
    //Finally, return how much we were able to read
    return dataRead;
}


uint64_t ShinyMetaFile::write( uint64_t offset, const char * buffer, uint64_t len ) {
    //First off, if we're extending the current file size, then let's do it!
    if( offset + len > this->getFileLen() )
        this->truncate( offset + len );

    //First off, if we're _starting_ outside of the realm of operation, return zero
    /*if( startChunk >= this->chunks.size() )
        return 0;
    
    //If we don't have the starting chunk, return zero
    if( !this->chunks[startChunk]->isAvailable() )
        return 0;*/

    uint64_t startChunk = offset/ShinyFileChunk::MAX_CHUNK_LEN;
    //Accumulate the amount of data written into dataWritten
    uint64_t dataWritten = 0;
    
    //Do this to get the offset into the first chunk, then we'll set it to zero afterward
    offset = offset % ShinyFileChunk::MAX_CHUNK_LEN;
    while( len - dataWritten > 0 ) {
        //Again, we'll just let the write() function truncate len - dataWritten for us
        dataWritten += this->chunks[startChunk]->write( (chunklen_t)offset, buffer + dataWritten, (chunklen_t)(len - dataWritten) );
        
        //Jut like I said I would, ZERO EET AUGHT!
        offset = 0;
        
        //Move forward in the chunks, then check for existence/availability and ragequit if either fails
        startChunk++;
        if( startChunk >= this->chunks.size() || !this->chunks[startChunk]->isAvailable() )
            break;
    }

    //Finlly, return how mcuh we were able to write
    return dataWritten;
}

ShinyNodeType ShinyMetaFile::getNodeType( void ) {
    return SHINY_NODE_TYPE_FILE;
}