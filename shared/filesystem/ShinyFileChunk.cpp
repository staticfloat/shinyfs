#include "ShinyFileChunk.h"
#include "ShinyMetaFile.h"
#include <base/Logger.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
//#include <io.h>

#define min( x, y )     ((x) > (y) ? (y) : (x))

ShinyFileChunk::ShinyFileChunk( ShinyMetaFile * parent, uint64_t chunkNum, chunklen_t dataLen, Chunkhash hash ) : parent(parent), fd( NULL ), data(NULL), dataLen(dataLen), chunkPath( NULL ), chunkNum( chunkNum ) {
    //First, check to make sure the cached file exists:
    struct stat st;
    if( !stat( this->chunkPath, &st ) ) {
        ERROR( "Could not find chunk %s [%llu] for %s [%llu]!", this->getFileChunkPath(), this->chunkNum, parent->getPath(), parent->getInode() );
        throw "Could not find chunk";
        return;
    }
    
    memcpy( this->hash, hash, sizeof(Chunkhash) );
    lastTouch = clock();
    this->dirty = false;
}

ShinyFileChunk::ShinyFileChunk( ShinyMetaFile * parent, uint64_t chunkNum, chunklen_t dataLen, char * data ) : parent(parent), fd( NULL ), data(NULL), dataLen(dataLen), chunkPath( NULL ), chunkNum( chunkNum ) {
    if( dataLen > MAX_CHUNK_LEN )
        WARN( "dataLen (%d) > MAX_CHUNK_LEN (%d) passed into constructor!  Truncated!", dataLen, MAX_CHUNK_LEN );
    
    //Initialize dataLen and data:  (If data is NULL, then just set this->data to NULL)
    this->data = new char[this->dataLen];
    if( data )
        memcpy( this->data, data, dataLen );
    else
        memset( this->data, NULL, dataLen );
    
    //update the hash to reflect the new data
    //this->updateHash();
    lastTouch = clock();
    this->dirty = true;
}

ShinyFileChunk::~ShinyFileChunk() {
    this->flush( true );
}

chunklen_t ShinyFileChunk::getChunkLen( void ) {
    return this->dataLen;
}

bool ShinyFileChunk::isAvailable( void ) {
    return this->data || this->open();
}

chunklen_t ShinyFileChunk::read( chunklen_t offset, char * buffer, chunklen_t len ) {
    //Make sure we're open and synced
    if( !this->open() )
        return -1;
    
    //First things first, don't ever read more than we can from this chunk
    len = min( len, this->dataLen - offset );
    
    //Actually read
    memcpy( buffer, this->data + offset, len );
    
    lastTouch = clock();
    //return how much we read!
    return len;
}

chunklen_t ShinyFileChunk::write( chunklen_t offset, const char * buffer, chunklen_t len ) {
    //Make sure we're open and synced
    if( !this->open() )
        return -1;
    
    //First things first, don't ever write more than we can in one chunk
    len = min( len, MAX_CHUNK_LEN - offset );
    
    //If where we're writing to is less than the size of the buffer we have allocated right now,
    if( offset + len > dataLen ) {
        //Move over what we need to and write away!
        char * tempData = data;
        this->data = new char[offset+len];
        
        //Copy it over
        memcpy( this->data, tempData, min( offset, dataLen ) );
        
        //Don't forget to update the dataLen!
        this->dataLen = offset + len;
        
        //Delete the old stuff
        delete( tempData );
    }
    
    //Finally, write it in!
    memcpy( this->data + offset, buffer, len );
    
    //post-finally, update our haaaash!!!  (actually only do this when flushing)
    //this->updateHash();
    
    lastTouch = clock();
    this->dirty = true;
    //return how much we wrote!
    return len;
}

int ShinyFileChunk::truncate( chunklen_t len ) {
    if( !this->open() )
        return -1;
    
    //Don't let someone try and allocate a crapload of memory here
    len = min( len, MAX_CHUNK_LEN );

    if( len != this->dataLen ) {
        //Save what we _do_ have
        char * tempData = this->data;
        
        //Allocate the new memory, and copy over what we used to have
        data = new char[len];
        if( len < this->dataLen )
            memcpy( data, tempData, len );
        else {
            //Also, zero-pad if we need to
            memcpy( data, tempData, this->dataLen );
            memset( data + this->dataLen, NULL, len - this->dataLen );
        }
        
        //Finally copy over the new length, and free the old memory
        this->dataLen = len;
        delete( tempData );
        this->dirty = true;
    }
    return len;
}

void ShinyFileChunk::flush( bool forceEvict ) {
    //If nothing in data, do nothing!
    if( !this->data )
        return;
    
    //Otherwise, write stuff out to disk if we're dirty
    if( this->dirty ) {
        //Update hash only on flush(), and only if dirty
        this->updateHash();
        
        if( !this->open() )
            ERROR( "Couldn't flush() to disk!" );
        else
            ::write( this->fd, this->data, this->dataLen );
    }
    this->dirty = false;
    
    //Now, check recency
    if( forceEvict || ((double)(clock() - lastTouch))/CLOCKS_PER_SEC > FILE_CHUNK_LIFETIME ) {
        //If we've outlived what we should, free the data and close the fd!
        delete( this->data );
        this->data = NULL;
        
        //Also, free our path, as we probably don't need it that much.  :P
        if( this->chunkPath ) {
            delete( this->chunkPath );
            this->data = NULL;
        }
        
        ::close( this->fd );
        this->fd = 0;
    }
}

const char * ShinyFileChunk::getFileChunkPath( void ) {
    if( this->chunkPath )
        return this->chunkPath;
    
    const char * prefix = this->parent->getCachePrefix();
    
    //<prefix> + . + <chunk # in hex>
    this->chunkPath = new char[strlen(prefix) + 1 + 12 + 1];
    sprintf( chunkPath, "%s.%.12llX", prefix, this->chunkNum );
    
    //We'll do this for getFileChunkPath() too, as it could be an indicator that we need to keep it around....
    lastTouch = clock();
    return chunkPath;
}

bool ShinyFileChunk::open( void ) {
    //If we're already open, don't re-open
    if( !this->fd ) {
        //Our file caches will ONLY be openable by the user that is running on the LOCAL filesystem!
        this->fd = ::open( this->getFileChunkPath(), O_CREAT | O_RDWR, S_IRWXU );
        if( !this->fd ) {
            ERROR( "Could not open %s!", this->getFileChunkPath() );
            return false;
        }
    }
    
    //If we're not already read in, READ IN!!!!
    if( !this->data ) {
        //Seek to the end to find the dataLen
        this->dataLen = (chunklen_t) lseek( this->fd, 0, SEEK_END );
        lseek( this->fd, 0, SEEK_SET );
        
        //Allocate data and reeeeead!
        this->data = new char[this->dataLen];
        uint64_t bytes_read = ::read( this->fd, this->data, this->dataLen );
        if( bytes_read != this->dataLen ) {
            WARN( "Could only read %llu bytes for %s! (should be %llu)", bytes_read, this->getFileChunkPath(), this->dataLen );
        }
    }
    lseek( this->fd, 0, SEEK_SET );
    return true;
}

bool ShinyFileChunk::sanityCheck( void ) {
    //First, make sure we're all opened and synced and crap
    if( !this->open() )
        return false;
    
    //Next, check that the hash matches
    Chunkhasher hasher;
    Chunkhash checkHash;
    hasher.CalculateDigest( checkHash, (const uint8_t *) this->data, this->dataLen );
    
    return memcmp( checkHash, this->hash, sizeof(Chunkhash) ) == 0;
}

void ShinyFileChunk::updateHash() {
    //Don't do anything if we're not loaded into memory
    if( !this->data )
        return;
    
    Chunkhasher hasher;
    hasher.CalculateDigest( this->hash, (const uint8_t *) this->data, this->dataLen );
}

const Chunkhash & ShinyFileChunk::getHash( void ) {
    if( this->dirty ) {
        //This is horrifying
        if( !this->open() )
            return *((Chunkhash *)NULL);
        
        //Don't let it purge here, because it's likely we're gonna write it out soon
        this->lastTouch = clock();        
        this->flush();
    }
    return this->hash;
}