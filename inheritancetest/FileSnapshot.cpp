#include "Nodes.h"

#define min(x,y) ((x)<(y)?(x):(y))

using namespace ShinyFS;

FileSnapshot::FileSnapshot( const char * newName, DirSnapshot * const newParent ) : NodeSnapshot( newName, newParent ) {
    this->fileLen = 0;
}

FileSnapshot::FileSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent ) : NodeSnapshot( serializedStream, newParent ) {
    // Because vtable's don't update until construction is done, we need to call update manually in each constructor
    update( serializedStream, true );
}

FileSnapshot::~FileSnapshot() {
}

uint64_t FileSnapshot::serializedLen() {
    uint64_t len = NodeSnapshot::serializedLen();

    // Only thing we need to serialize out is our length
    len += sizeof(uint64_t);

    return len;
}

void FileSnapshot::serialize( uint8_t ** output_ptr ) {
    uint8_t * start = *output_ptr;
    // Let NodeSnapshot do what it does best
    NodeSnapshot::serialize( output_ptr );

    // Throw our file length in there
    *((uint64_t *)*output_ptr) = this->fileLen;
    *output_ptr += sizeof(uint64_t);
}

void FileSnapshot::update( const uint8_t ** input_ptr, bool skipSelf ) {
    if( !skipSelf ) {
        // First, call up to the Elder Nodes
        NodeSnapshot::update( input_ptr );
    }

    // Load in file length
    this->fileLen = *((uint64_t *)*input_ptr);
    *input_ptr += sizeof(uint64_t);
}


uint64_t FileSnapshot::getLen() {
	return this->fileLen;
}

uint64_t FileSnapshot::read( uint64_t offset, char * data, uint64_t len ) {
    RootDirSnapshot * root = this->getRoot();
    return this->read( root->getDB(), this->getPath( root ), offset, data, len );
}

uint64_t FileSnapshot::read( DBWrapper * db, const char * path, uint64_t offset, char * data, uint64_t len ) {
    // First, figure out what "chunk" to start from:
    uint64_t chunk = offset/FILE_CHUNKSIZE;
    
    // This is the offset within that chunk that we need to start from
    offset = offset - chunk*FILE_CHUNKSIZE;
    
    // We'll have to build a new key for every chunk
    uint64_t pathlen = strlen(path);
    char * key = new char[pathlen+16+1];   // enough room for "<path>[chunk]\0"
    memcpy( key, path, pathlen );
    
    // This is where we will plop the chunk at the end of the key
    char * keyChunk = key + pathlen;
    
    // Temporary storage where we'll put chunks as we load them in,
    // then we'll copy from the chunks into data for transport back to the user
    char buffer[FILE_CHUNKSIZE];
    
    // The total number of bytes read
    uint64_t bytesRead = 0;
    
    // Start to read in from chunks:
    while( len > bytesRead ) {
        // Build this chunk's key:
        sprintf( keyChunk, "%.16" PRIx64, chunk );
        
        uint64_t bytesJustRead = db->get( key, buffer, FILE_CHUNKSIZE );
        if( bytesJustRead == -1 ) {
            // This means we couldn't read this chunk, so we must have run into the end of the file, or the DB is corrupt
            ERROR( "Could not read chunk %s from cache: %s", key, db->getError() );
            break;
        }
        // Otherwise, we load in as many bytes into data as we can!
        uint64_t amntToCopy = min( bytesJustRead - offset, len - bytesRead );
        memcpy( data + bytesRead, buffer + offset, amntToCopy );
        bytesRead += amntToCopy;
        
        // If this chunk wasn't full, we're done
        if( bytesJustRead != FILE_CHUNKSIZE )
            break;
        
        // reset offset to zero, as we move on to the next chunk now
        offset = 0;
        chunk++;
    }

    return bytesRead;
}

const NodeType FileSnapshot::getNodeType() {
	return TYPE_FILE;
}