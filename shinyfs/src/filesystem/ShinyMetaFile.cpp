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

ShinyMetaFile::ShinyMetaFile( const char * newName ) : ShinyMetaNode( newName ) {
    this->fileLen = 0;
}

ShinyMetaFile::ShinyMetaFile( const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( newName ) {
    this->fileLen = 0;
    parent->addNode( (ShinyMetaNode *)this );
}

ShinyMetaFile::ShinyMetaFile( const char ** serializedInput ) : ShinyMetaNode( "" ) {
    this->unserialize( serializedInput );
}

ShinyMetaFile::~ShinyMetaFile() {
}

uint64_t ShinyMetaFile::read( uint64_t offset, char * data, uint64_t len ) {
    return this->read( this->getFS()->getDB(), this->getPath(), offset, data, len );
}

uint64_t ShinyMetaFile::write( uint64_t offset, const char * data, uint64_t len ) {
    return this->write( this->getFS()->getDB(), this->getPath(), offset, data, len );
}

void ShinyMetaFile::setLen( uint64_t newLen ) {
    this->setLen( this->getFS()->getDB(), this->getPath(), newLen );
}

uint64_t ShinyMetaFile::getLen() {
    return this->fileLen;
}

uint64_t ShinyMetaFile::read( kyotocabinet::PolyDB * db, const char * path, uint64_t offset, char * data, uint64_t len ) {
    // First, figure out what "chunk" to start from:
    uint64_t chunk = offset/CHUNKSIZE;
    
    // This is the offset within that chunk that we need to start from
    offset = offset - chunk*CHUNKSIZE;
    
    // We'll have to build a new key for every chunk
    uint64_t pathlen = strlen(path);
    char * key = new char[pathlen+16+1];   // enough room for "<path>[chunk]\0"
    memcpy( key, path, pathlen );
    
    // This is where we will plop the chunk at the end of the key
    char * keyChunk = key + pathlen;
    
    // Temporary storage where we'll put chunks as we load them in,
    // then we'll copy from the chunks into data for transport back to the user
    char buffer[CHUNKSIZE];
    
    // The total number of bytes read
    uint64_t bytesRead = 0;
    

    // Start to read in from chunks:
    while( len > bytesRead ) {
        // Build this chunk's key:
        sprintf( keyChunk, "%.16llx", chunk );
        
        /*
        kyotocabinet::PolyDB::Cursor * cur = db->cursor();
        cur->jump();
        size_t keySize = 0;
        size_t valSize = 0;
        char * newKey;
        const char * val;
        while( (newKey = cur->get(&keySize, &val, &valSize, true)) ) {
            LOG( "%s -> %s", newKey, val );
            if( strcmp( newKey, key ) == 0 ) {
                LOG( "It's a match!" );
            }
        }*/
        
        int bytesJustRead = db->get( key, pathlen + 16 + 1, buffer, CHUNKSIZE );
        if( bytesJustRead == -1 ) {
            kyotocabinet::PolyDB::Error e = db->error();
            ERROR( "Could not read chunk %s from cache: [%d] %s", key, e.code(), e.name() );
            // This means we couldn't read this chunk, so we must have run into the end of the file.
            break;
        }
        // Otherwise, we load in as many bytes into data as we can!
        uint64_t amntToCopy = min( bytesJustRead - offset, len - bytesRead );
        memcpy( data + bytesRead, buffer + offset, amntToCopy );
        bytesRead += amntToCopy;
        
        // If this chunk wasn't full, we're done
        if( bytesJustRead != CHUNKSIZE )
            break;
        
        // reset offset to zero, as we move on to the next chunk now
        offset = 0;
        chunk++;
    }
    return bytesRead;
}

uint64_t ShinyMetaFile::write( kyotocabinet::PolyDB * db, const char * path, uint64_t offset, const char * data, uint64_t len ) {
    // If we're going to overwrite, then exteeenddd..... EXTEEEENNDDDD!!!
    if( offset + len > this->getLen() )
        this->setLen( db, path, offset + len );
    
    // First, figure out what "chunk" to start from:
    uint64_t chunk = offset/CHUNKSIZE;
    
    // This is the offset within that chunk that we need to start from
    offset = offset - chunk*CHUNKSIZE;
    
    // We'll have to build a new key for every chunk
    uint64_t pathlen = strlen(path);
    char * key = new char[pathlen+16+1];   // enough room for "<path>[chunk]\0"
    memcpy( key, path, pathlen );
    
    // This is where we will plop the chunk at the end of the key
    char * keyChunk = key + pathlen;
    
    // The total number of bytes written
    uint64_t bytesWritten = 0;
    
    // Start to read in from chunks:
    while( len > bytesWritten ) {
        // Build this chunk's key:
        sprintf( keyChunk, "%.16llx", chunk );
        
        // Two possibilities; there is data before where we are writing that we need to preserve,
        // or there is data after where we are writing in the same chunk.
        if( offset != 0 || (len - bytesWritten < CHUNKSIZE && this->getLen() > offset + len) ) {
            char * previousData = new char[CHUNKSIZE];
            db->get( key, pathlen + 16 + 1, previousData, CHUNKSIZE );
            
            // copy over the stuff we need to
            uint64_t amntToCopy = min( CHUNKSIZE - offset, len - bytesWritten );
            memcpy( previousData + offset, data + bytesWritten, amntToCopy );
            
            // Set this chunk back
            if( !db->set( key, pathlen + 16 + 1, previousData, CHUNKSIZE ) ) {
                kyotocabinet::PolyDB::Error e = db->error();
                ERROR( "Could not write chunk %s to cache: [%d] %s", key, e.code(), e.name() );
            }
            
            // increment the amount we've written
            bytesWritten += amntToCopy;
            
            // cleanup cleanup
            delete( previousData );
        } else {
            // Otherwise, it's a lot simpler, and we just need to copy over as much junk as we can
            uint64_t amntToCopy = min( CHUNKSIZE, len - bytesWritten );
            if( !db->set( key, pathlen + 16 + 1, data + bytesWritten, amntToCopy ) ) {
                kyotocabinet::PolyDB::Error e = db->error();
                ERROR( "Could not write chunk %s to cache: [%d] %s", key, e.code(), e.name() );
            }

            // increment the amount we've written
            bytesWritten += amntToCopy;
        }
        
        // reset offset to zero, as we move on to the next chunk now
        offset = 0;
        chunk++;
    }
    return bytesWritten;
}

void ShinyMetaFile::setLen( kyotocabinet::PolyDB * db, const char * path, uint64_t newLen ) {
    // We'll have to build a new key for every chunk
    uint64_t pathlen = strlen(path);
    char * key = new char[pathlen+16+1];   // enough room for "<path>[chunk]\0"
    memcpy( key, path, pathlen );
    
    // This is where we will plop the chunk at the end of the key
    char * keyChunk = key + pathlen;
    
    // Figure out if we need to delete chunks, or create new ones
    if( this->fileLen > newLen ) {
        // While the lengths are not matched, delete chunks (or pieces of them)
        while( this->fileLen != newLen ) {
            // Find the length of the last chunk
            uint32_t chunkLen = this->fileLen - (this->fileLen/CHUNKSIZE)*CHUNKSIZE;
            chunkLen = (chunkLen == 0 ? CHUNKSIZE : chunkLen);
            
            // Get the key for the last chunk
            sprintf( keyChunk, "%.16llx", this->fileLen/CHUNKSIZE );
            
            // Should we take away this entire chunk, or just part of it?
            if( this->fileLen - chunkLen >= newLen ) {
                LOG( "Removing chunk %s", key );
                // TAKE THE LEG!  TAKE THE LEG DOCTOR! (remove the last chunk)
                if( !db->remove( key, pathlen + 16 + 1 ) ) {
                    kyotocabinet::PolyDB::Error e = db->error();
                    WARN( "Couldn't remove chunk %s from cache, [%d] %s", key, e.code(), e.name() );
                }
                this->fileLen -= chunkLen;
            } else {
                // Here's the tricksy part, we have to remove PART of this chunk
                // We will first get the part of the chunk that matters, and reset the key with that value
                char * data = new char[this->fileLen - newLen];
                if( !db->get( key, pathlen + 16 + 1, data, this->fileLen - newLen ) ) {
                    kyotocabinet::PolyDB::Error e = db->error();
                    ERROR( "Could not read chunk %s, from cache, [%d] %s", key, e.code(), e.name() );
                }
                if( !db->set( key, pathlen + 16 + 1, data, this->fileLen - newLen ) ) {
                    kyotocabinet::PolyDB::Error e = db->error();
                    ERROR( "Could not write chunk %s to cache, [%d] %s", key, e.code(), e.name() );
                }
                
                delete( data );
                this->fileLen = newLen;
            }
        }
    } else {
        while( this->fileLen != newLen ) {
            // Find the length that would be added by fulfilling this chunk, or adding a new one
            //uint32_t chunkLen = CHUNKSIZE - (this->fileLen - (this->fileLen/CHUNKSIZE)*CHUNKSIZE);
            
            // Get the key for the last chunk
            sprintf( keyChunk, "%.16llx", this->fileLen/CHUNKSIZE );
            
            // Create a buffer large enough to hold this entire chunk
            uint64_t newChunkLen = min(CHUNKSIZE, newLen - this->fileLen);
            char * data = new char[newChunkLen];
            
            // Do we have junk we need to save by reading it in, appending zeros, then writing back?
            if( this->fileLen % CHUNKSIZE ) {
                // Get the existing data
                if( !db->get( key, pathlen + 16 + 1, data, newChunkLen ) ) {
                    kyotocabinet::PolyDB::Error e = db->error();
                    ERROR( "Could not read chunk %s from cache, [%d] %s", key, e.code(), e.name() );
                }
            }
            // write it in again
            if( !db->set( key, pathlen + 16 + 1, data, newChunkLen ) ) {
                kyotocabinet::PolyDB::Error e = db->error();
                ERROR( "Could not write chunk %s to cache, [%d] %s", key, e.code(), e.name() );
            }
            
            this->fileLen += newChunkLen;
        }
    }
}


bool ShinyMetaFile::sanityCheck( void ) {
    //First, the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();

    TODO( "Add in file hash checking here" );
    
    return retVal;
}

uint64_t ShinyMetaFile::serializedLen( void ) {
    // Start off with the basic length
    size_t len = ShinyMetaNode::serializedLen();
    
    // Add on length of file
    len += sizeof(uint64_t);
    
    // returnamacate
    return len;
}

char * ShinyMetaFile::serialize( char *output ) {
    //First serialize out the basic stuff into output
    output = ShinyMetaNode::serialize(output);
    
    // Next, file length
    *((uint64_t *)output) = this->fileLen;
    output += sizeof(uint64_t);
    
    return output;
}

void ShinyMetaFile::unserialize( const char ** input ) {
    // First, call up the chain
    ShinyMetaNode::unserialize(input);
    
    this->fileLen = *((uint64_t *)*input);
    *input += sizeof(uint64_t);
}

ShinyMetaNode::NodeType ShinyMetaFile::getNodeType( void ) {
    return ShinyMetaNode::TYPE_FILE;
}