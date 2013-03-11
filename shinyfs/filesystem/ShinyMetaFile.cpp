#include "ShinyMetaFile.h"
#include "ShinyFilesystem.h"
#include "ShinyMetaDir.h"
#include <base/Logger.h>

#define min( x, y ) ((x) > (y) ? (y) : (x))

ShinyMetaFile::ShinyMetaFile( const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( newName, parent ) {
}

ShinyMetaFile::ShinyMetaFile( const char ** serializedInput, ShinyMetaDir * parent ) : ShinyMetaNode( serializedInput, parent ) {
    ShinyMetaNode::unserialize( serializedInput );
}

ShinyMetaFile::~ShinyMetaFile() {
}

uint64_t ShinyMetaFile::write( uint64_t offset, const char * data, uint64_t len ) {
    ShinyFilesystem * fs = ShinyMetaFileSnapshot::getFS();
    return this->write( fs->getDB(), fs->getNodePath(this), offset, data, len );
}

void ShinyMetaFile::setLen( uint64_t newLen ) {
    this->setLen( ShinyMetaFileSnapshot::getFS()->getDB(), ShinyMetaFileSnapshot::getPath(), newLen );
}

uint64_t ShinyMetaFile::write( ShinyDBWrapper * db, const char * path, uint64_t offset, const char * data, uint64_t len ) {
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
            db->get( key, previousData, CHUNKSIZE );
            
            // copy over the stuff we need to
            uint64_t amntToCopy = min( CHUNKSIZE - offset, len - bytesWritten );
            memcpy( previousData + offset, data + bytesWritten, amntToCopy );
            
            // Set this chunk back
            if( !db->put( key, previousData, CHUNKSIZE ) ) {
                ERROR( "Could not write chunk %s to cache: %s", key, db->getError() );
            }
            
            // increment the amount we've written
            bytesWritten += amntToCopy;
            
            // cleanup cleanup
            delete( previousData );
        } else {
            // Otherwise, it's a lot simpler, and we just need to copy over as much junk as we can
            uint64_t amntToCopy = min( CHUNKSIZE, len - bytesWritten );
            if( !db->put( key, data + bytesWritten, amntToCopy ) ) {
                ERROR( "Could not write chunk %s to cache: %s", key, db->getError() );
            }

            // increment the amount we've written
            bytesWritten += amntToCopy;
        }
        
        // reset offset to zero, as we move on to the next chunk now
        offset = 0;
        chunk++;
    }
    
    this->set_mtime();
    return bytesWritten;
}

void ShinyMetaFile::setLen( ShinyDBWrapper * db, const char * path, uint64_t newLen ) {
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
            uint64_t chunkLen = this->fileLen - (this->fileLen/CHUNKSIZE)*CHUNKSIZE;
            chunkLen = (chunkLen == 0 ? CHUNKSIZE : chunkLen);
            
            // Get the key for the last chunk
            sprintf( keyChunk, "%.16llx", this->fileLen/CHUNKSIZE );
            
            // Should we take away this entire chunk, or just part of it?
            if( this->fileLen - chunkLen >= newLen ) {
                // TAKE THE LEG!  TAKE THE LEG DOCTOR! (remove the last chunk)
                if( !db->del( key ) ) {
                    WARN( "Couldn't remove chunk %s from cache, %s", key, db->getError() );
                }
                this->fileLen -= chunkLen;
            } else {
                // Here's the tricksy part, we have to remove PART of this chunk
                // We will first get the part of the chunk that matters, and reset the key with that value
                char * data = new char[this->fileLen - newLen];
                if( !db->get( key, data, this->fileLen - newLen ) ) {
                    ERROR( "Could not read chunk %s, from cache, %s", key, db->getError() );
                }
                if( !db->put( key, data, this->fileLen - newLen ) ) {
                    ERROR( "Could not write chunk %s to cache, %s", key, db->getError() );
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
                if( !db->get( key, data, newChunkLen ) ) {
                    ERROR( "Could not read chunk %s from cache, %s", key, db->getError() );
                }
            }
            // write it in again
            if( !db->put( key, data, newChunkLen ) ) {
                ERROR( "Could not write chunk %s to cache, %s", key, db->getError() );
            }
            
            this->fileLen += newChunkLen;
        }
    }
    
    this->set_mtime();
}

/*
bool ShinyMetaFile::sanityCheck( void ) {
    //First, the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();

    TODO( "Add in file hash checking here" );
    
    return retVal;
}
*/