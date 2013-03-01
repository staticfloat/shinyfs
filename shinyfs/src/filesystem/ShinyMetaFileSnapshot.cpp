#include "ShinyFilesystem.h"
#include "ShinyMetaFileSnapshot.h"
#include <base/Logger.h>

#define min( x, y ) ((x) > (y) ? (y) : (x))


ShinyMetaFileSnapshot::ShinyMetaFileSnapshot( const char ** serializedInput, ShinyMetaDirSnapshot * parent )
    : ShinyMetaNodeSnapshot( serializedInput, parent ), fileLen( 0 )
{
    this->unserialize( serializedInput );
}

ShinyMetaFileSnapshot::ShinyMetaFileSnapshot() {
    // This only to be called when we're actually creating a new node from ShinyMetaFile
}

ShinyMetaFileSnapshot::~ShinyMetaFileSnapshot() {
    // Don't do anything right now
}

uint64_t ShinyMetaFileSnapshot::serializedLen( void ) {
    // Start off with the basic length
    size_t len = ShinyMetaNodeSnapshot::serializedLen();
    
    // Add on length of file
    len += sizeof(uint64_t);
    
    // returnamacate
    return len;
}

char * ShinyMetaFileSnapshot::serialize( char *output ) {
    //First serialize out the basic stuff into output
    output = ShinyMetaNodeSnapshot::serialize(output);
    
    // Next, file length
    *((uint64_t *)output) = this->fileLen;
    output += sizeof(uint64_t);
    
    return output;
}

void ShinyMetaFileSnapshot::unserialize( const char ** input ) {
    this->fileLen = *((uint64_t *)*input);
    *input += sizeof(uint64_t);
}

uint64_t ShinyMetaFileSnapshot::getLen() {
    return this->fileLen;
}

uint64_t ShinyMetaFileSnapshot::read( uint64_t offset, char * data, uint64_t len ) {
    ShinyFilesystem * fs = this->getFS();
    return this->read( fs->getDB(), fs->getNodePath(this), offset, data, len );
}

uint64_t ShinyMetaFileSnapshot::read( ShinyDBWrapper * db, const char * path, uint64_t offset, char * data, uint64_t len ) {
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
        
        uint64_t bytesJustRead = db->get( key, buffer, CHUNKSIZE );
        if( bytesJustRead == -1 ) {
            ERROR( "Could not read chunk %s from cache: %s", key, db->getError() );
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

ShinyMetaNodeSnapshot::NodeType ShinyMetaFileSnapshot::getNodeType( void ) {
    return ShinyMetaNodeSnapshot::TYPE_FILE;
}