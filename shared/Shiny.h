#ifndef SHINY_H
#define SHINY_H

#include "filesystem/ShinyFilesystem.h"
#include "protocol/ShinyCommunicator.h"

/*
 This is it; the big kahunah.  This is the main object that applications should deal with;
 When created, it should connect to the network, update the metadata, spawn off threads to
 deal with flushing, multithreaded communication, etc....

 Users should just have to ask for files, and it will transparently go get them and give 'em back.
 Write locks, all of that, should be completely invisible to the user, except in failure.
/**/


class Shiny {
public:
    /// CONSTRUCTORS ///
    //This creates a Shiny peer that connects to this peer and tries to download meta data from him
    Shiny( const char * filecacheLocation, const char * peerAddr );
    
    //This creates a NEW filesystem
    Shiny( const char * filecacheLocation );
    
    
    /// FILE HANDLING ///
    ShinyMetaNode * findNode( const char * path );
    ShinyMetaNode * findNode( inode_t node );
    
    
private:
};



#endif //SHINY_H