#pragma once
#ifndef SHINYFILECHUNK_H
#define SHINYFILECHUNK_H

#include <sys/types.h>
#include <cryptopp/sha.h>
#include <sys/time.h>


using namespace CryptoPP;
//This is a hash of a small chunk of a file
typedef SHA256 Chunkhasher;
typedef unsigned char Chunkhash[Chunkhasher::DIGESTSIZE];


/*
 FileChunks load chunks in from disk, handle integrity checking,
 caching, saving to disk, etc...
 
 It will lazily load chunks into memory, and then when the flush()
 command propogates down to us, we'll evict ourselves from memory
 if we haven't seen action for a while.

 */
typedef int32_t chunklen_t;

class ShinyMetaFile;
class ShinyFileChunk {
public:
    //This constructor is used to initialize a chunk that we just received
    ShinyFileChunk( ShinyMetaFile * parent, uint64_t chunkNum, chunklen_t dataLen, Chunkhash hash );
    
    //This constructor is used to initialize a chunk that we are creating with data/is empty
    ShinyFileChunk( ShinyMetaFile * parent, uint64_t chunkNum, chunklen_t dataLen = ShinyFileChunk::MAX_CHUNK_LEN, char * data = NULL );
    ~ShinyFileChunk();
    
    //Returns the length of this chunk in bytes
    chunklen_t getChunkLen( void );
    
    //Returns if the chunk is available on disk/in memory
    bool isAvailable( void );
    
    //if can't read all of it, returns how much it _could_ read, -1 on error
    chunklen_t read( chunklen_t offset, char * buffer, chunklen_t len );
    
    //if can't write all of it, returns how much it _could_ write, -1 on error
    //Automatically updateHash()'s
    chunklen_t write( chunklen_t offset, const char * buffer, chunklen_t len );
    
    //Truncates (or extends) the current chunk to this size
    int truncate( chunklen_t len );
    
    //Writes data out to disk.  (checks to see if the last time this chunk
    //was touched was long enough ago that we can evict it from memory. Also
    //evicts if forceEvict == true)
    void flush( bool forceEvict = false );
    
    //Performs an integrity check against the hash we have stored, returning true it if passes
    bool sanityCheck( void );
    
    //Gets the path of this file chunk
    const char * getFileChunkPath( void );
    
    //Returns the hash (and recalculates if need be)
    const Chunkhash & getHash( void );
    
    //our hash
    Chunkhash hash;
private:
    //after a write(), we have to update this hash
    void updateHash();
    
    //Before read()'s and write()'s, make sure the file is open and read into memory
    bool open( void );
    
    //our parent
    ShinyMetaFile * parent;
    
    //File descriptor; if this is NULL, then we need to open() before read()/write() can continue
    int fd;

    //data loaded in
    char * data;
    chunklen_t dataLen;
    
    //filename of this chunk
    char * chunkPath;

    //our chunk index
    uint64_t chunkNum;
    
    //Last time we were touched
    clock_t lastTouch;
    
    //Are we dirty?
    bool dirty;
    
public:
    //Chunks are limited to 256KB.
    const static chunklen_t MAX_CHUNK_LEN = 256*1024;
    
    //Lifetime in seconds of a file chunk before it dies (gets thrown back on disk) through neglect
    const static float FILE_CHUNK_LIFETIME = 100.0f;
};

#endif //SHINYFILECHUNK_H