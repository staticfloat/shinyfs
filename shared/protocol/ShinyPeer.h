#ifndef shinyfs_node_ShinyPeer_h
#define shinyfs_node_ShinyPeer_h

#include "ShinyMsg.h"
#include "ShinyUser.h"
#include <zmq.hpp>
#include <cryptopp/AES.h>

//We'll use the maximum length possible AES key for our symmetric peer encryption
typedef unsigned char PeerKey[CryptoPP::AES::MAX_KEYLENGTH];

//I will comminucate through this interface to all peers.  I will do this using a
//message hub, (probably called ShinyCommunicator or something like that) which will
//be the endpoint for all messaging and such

//You will need to supply all sockets to talk to this beast
class ShinyPeer {
public:
    ShinyPeer( zmq::socket_t &internal, const char * addr );
    ~ShinyPeer();
    
    //Sends a message to this peer
    void send( zmq::socket_t &internal, ShinyMsg::ShinyMsgType type, const char * data, uint64_t datalen );
    void send( zmq::socket_t &internal, ShinyMsg * msg );
    
    //Receives a message from the peer (blocking)
    ShinyMsg * recv( zmq::socket_t &internal );
    
    //Returns the address (e.g. whatever was passed into the constructor) of this particular peer
    const char * getPeerAddr( void );
    
    //Returns the user id of this particular peer
    user_t getUserId( void );
private:
    //These are filled in during/after the handshake phase
    user_t uid;
    unsigned char key[CryptoPP::AES::MAX_KEYLENGTH];
    unsigned char iv[CryptoPP::AES::BLOCKSIZE];
    
    //Address of the peer, in human-readable format.  (e.g. "localhost:5040")
    //Is also the zmq routing code!
    char * peerAddr;
    CryptoPP::AuthenticatedEncryptionFilter * aef;
};


#endif