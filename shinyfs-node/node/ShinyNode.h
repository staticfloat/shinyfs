#ifndef SHINYNODE_H
#define SHINYNODE_H

#include "ShinyFilesystem.h"

class ShinyNode {
public:
    //Creates a new node object, loading previous state from a file, and connecting to another peer
    ShinyNode( const char * new_filename, const char * peerAddr );
    ShinyNode();
    
    //This function will rely heavily on what kind of requests and such I permit.
    //Pending further thought on the whole protocol architecture
    // void handleRequest( unsigned char requestType, char * data, char ** outdata, uint64_t outdata_len );
    
    //Serializes the tree, and writes it out to filename
    void flush();
private:
    char * filename;
    ShinyFilesystem * fs;
};



#endif //SHINYTRACKER_H
