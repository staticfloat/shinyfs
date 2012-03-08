#ifndef SHINYNETWORK_H
#define SHINYNETWORK_H

#include <zmq.hpp>
#include "../filesystem/ShinyFilesystem.h"
#include <sys/types.h>
#include <map>
#include <vector>
#include "ShinyPeer.h"
#include <cryptopp/AES.h>
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <tr1/unordered_map>
#include <sys/socket.h>

/*
 This is the communication hub, the endpoint to talk to the outside world (at least, via node-comm)
 It will automatically create new ShinyPeer* objects when others connect to us, and will pass off
 messages to those ShinyPeers when we are talked to, etc.
 
 If we need to talk to someone, we ask for a peer via getPeer(), and if we're already connected,
 it just returns the cached ShinyPeer object.
 
 ShinyPeers automatically talk to the ShinyNetwork so that all of our outbound traffic goes through one single socket.
 ShinyPeer communications are all unencrypted, and then the ShinyNetwork compresses, encrypts, and then sends all the communication.
 */

class ShinyNetwork {
    /////// COMMUNICATION THREAD ////////
    //This puppy runs in a separate thread, doing most of the heavy lifting.  Most other methods
    //talk to this guy, via zmq sockets, to avoid threading issues.
    friend void * ShinyCommunicator( void * data ); 
public:
    //Creates a new ShinyNetwork, tries to connect to given peerAddr.
    //Listens on port SHINYCOM_PORT for incoming connections.
    ShinyNetwork( const char * peerAddr );
    ~ShinyNetwork();
protected:
    pthread_t commThread;
    int commRunLevel;
    
    enum {
        COMM_STARTUP,
        COMM_RUNNING,
        COMM_STOP,
    };
    
    zmq::context_t * context;
    
    //Maps human-readable addresses to a ShinyPeer
    std::tr1::unordered_map<std::string, ShinyPeer *> PD_addr;
    
    //Maps zmq routing codes to a ShinyPeer
    std::tr1::unordered_map<uint32_t, ShinyPeer *> PD_zmq;
    
    
    
    
    //////// PEERS AND NETWORK ////////
public:
    //Connects to a peer if not already connected, returns an object interfacing with it
    ShinyPeer * getPeer( const char * addr );
    
    //Connects to a peer that owns the given inode
    //If the result is not already cached, then we start asking people who would know, and then cache it
    ShinyPeer * getPeer( inode_t inode );
    
    //Returns the "parent" peer of this node, (returns NULL if we are the root node)
    ShinyPeer * getParentPeer( void );
    
    //Returns the "child" peers of this node, (returns an empty vector if we're a leaf node)
    const std::vector<ShinyPeer*> * getChildPeers( void );

    //Returns the number of peers we are actively connected to
    uint16_t getNumConnectedPeers();
protected:
    //Caches and maps which inodes belong to which peers.
    std::map<inode_t, ShinyPeer *> inodeToPeerMap;
    
    ShinyPeer * parentPeer;
    std::vector<ShinyPeer *> childPeers;
    
    
    
    /////// MISC ///////
public:
    const char * findExternalIP( bool forceRefresh );
    const char * findInternalIP( bool forceRefresh );
    bool resolveHost( const char * hostname, const char * port, sockaddr ** sa, socklen_t * len, char ** ip = NULL );
protected:
    char * externalIP;
    char * internalIP;
};

#endif //SHINYCOMMUMICATOR_H