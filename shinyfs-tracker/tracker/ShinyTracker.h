#ifndef SHINYTRACKER_H
#define SHINYTRACKER_H

#include "ShinyMetaFilesystem.h"

class ShinyTracker {
public:
    //Creates a new tracker object, loading previous state from a file
    ShinyTracker( const char * filename );
    ~ShinyTracker();
    
    //This function will rely heavily on what kind of requests and such I permit.
    //Pending further thought on the whole protocol architecture
   // void handleRequest( unsigned char requestType, char * data, char ** outdata, uint64_t outdata_len );
    
    uint64_t serializeMetaTree( char ** output );
    
    //Serializes the tree, and writes it out to filename
    void flush( void );
private:
    int fh;
    ShinyMetaFilesystem * fs;
    bool dirty;
};



#endif //SHINYTRACKER_H
