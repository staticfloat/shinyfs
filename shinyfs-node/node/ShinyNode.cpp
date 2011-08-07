#include "ShinyNode.h"
#include "base/Logger.h"
#include <fcntl.h>

ShinyNode::ShinyNode( const char * new_filename, const char * peerAddr ) {
    //First, copy filename over
    filename = new char[strlen(new_filename)+1];
    strcpy( filename, new_filename );
    
    //Open fd
    int fd = open( filename, O_RDONLY | O_CREAT );
    
    //Get filesize of file so we can allocate an appropriate buffer
    off_t fileSize = lseek(fd, SEEK_END, 0);
    
    //Check to see if there is data at all
    char * fileData = NULL;
    if( fileSize ) {
        //Allocate buffer, read in data, close file
        fileData = new char[fileSize];
        read(fd, &fileData, fileSize );
    }
    close( fd );
    
    //Construct fs off of file data
    this->fs = new ShinyMetaFilesystem( fileData );
}

ShinyNode::ShinyNode() {
    flush();
    delete( filename );
    delete( this->fs );
}


void ShinyNode::flush() {
    if( fs->isDirty() ) {
        //The fs will allocate for us, once it figures out how much space it's going to take
        char * serializedData;
        uint64_t len = fs->serialize(&serializedData);
        if( len ) {
            int fd = open( filename, O_WRONLY | O_TRUNC );
            write( fd, serializedData, len );
            close( fd );
            LOG( "Wrote %llu bytes of serialized goodness out to %s", len, filename );
        } else
            ERROR( "fs->serialize() gave us Nothing!  NOTHING!!!!" );
        
        //We have to clean up after the fs though
        delete( serializedData );
    }
}