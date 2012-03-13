#include "ShinyTracker.h"
#include "base/Logger.h"
#include <fcntl.h>

ShinyTracker::ShinyTracker( const char * filename ) {
    LOG( "O_NONBLOCK on open() for flushing tracker data?" );
    this->fh = open( filename, O_CREAT );

    this->fs = new ShinyMetaFilesystem( NULL );
    dirty = false;
}

ShinyTracker::~ShinyTracker() {
    flush();
    close( fh );
}

/*
void ShinyTracker::handleRequest( unsigned char requestType, char * data, char ** outdata, uint64_t * outdata_len ) {
    LOG( "Received requestType %d with data %s", requestType, data );
    WARN( "Outputting \"The cake is a lie\"" );

    (*outdata_len) = strlen("The cake is a lie")+1;
    (*outdata) = new char[*outdata_len];
    strcpy( *outdata, "The cake is a lie" );
    //Boom.  roasted.

    //Just always do this right now
    dirty = true;
}*/

uint64_t ShinyTracker::serializeMetaTree( char ** output ) {
    return fs->serialize( output );
}

void ShinyTracker::flush() {
    if( dirty ) {
        char * data = NULL;
        uint64_t len = fs->serialize( &data );
        LOG( "Serialized into %ull bytes!", len );
        delete( data );
    }
}