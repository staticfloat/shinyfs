#include "ShinyPeer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "base/Logger.h"
#include <cryptopp/gcm.h>
#include <cryptopp/osrng.h>
#include <string.h>


ShinyPeer::ShinyPeer( zmq::socket_t &internal, const char * addr )  {
    //Copy over peerAddr
    this->peerAddr = new char[strlen(addr)+1];
    strcpy( this->peerAddr, addr );
    
    //The first time we ever try to talk to a peer, ShinyNetwork will reach out and get an actual connection for us
    //zmq::message_t msg( this->peerAddr, strlen(this->peerAddr) + 1, NULL );
    zmq::message_t msg( (void*)addr, strlen(addr), NULL);    
    //This is all we send for this "initiation" message
    internal.send( msg );

    //Wait for a response
    //msg.rebuild();
    zmq::message_t recvMsg;
    internal.recv( &recvMsg );
    
    printf("[%lu]\n", recvMsg.size() );
    if( recvMsg.size() > 0 ) {
        LOG( "Successfully connected! BEGIN HANDSHAKE NOW" );
    } else {
        WARN( "Unable to connect to %s", addr );
    }
    
    
    TODO( "Here is where I handshake, get user ID, etc...." );
}

ShinyPeer::~ShinyPeer() {
    delete( this->peerAddr );
}

void ShinyPeer::send( zmq::socket_t &internal, ShinyMsg::ShinyMsgType type, const char * data, uint64_t datalen ) {
    //Encode the data using AES, and also append a MAC via GCM
    std::string ciphertext;
    
    //Random Number Generator to create IV with
    CryptoPP::AutoSeededRandomPool rnd;
    
    //Generate a random block to use as the iv
    unsigned char iv[CryptoPP::AES::BLOCKSIZE];
    rnd.GenerateBlock( iv, CryptoPP::AES::BLOCKSIZE);
    
    //Create Encryption object
    CryptoPP::GCM< CryptoPP::AES >::Encryption enc;
    enc.SetKeyWithIV( this->key, CryptoPP::AES::MAX_KEYLENGTH, iv, CryptoPP::AES::BLOCKSIZE );

    //Create the aef to automagically output the MAC as well
    CryptoPP::AuthenticatedEncryptionFilter aef( enc, new CryptoPP::StringSink(ciphertext) );
    
    //Put in the type and then the data
    aef.Put( (const byte *) &type, 1 );
    aef.Put( (const byte *) data, datalen );
    aef.MessageEnd();
    
    //This 
    const char * peerAddr = this->getPeerAddr();
    zmq::message_t peerAddrMsg( (void *)peerAddr, strlen(peerAddr), NULL );
    internal.send( peerAddrMsg );
    TODO("Somehow send the messagepart with this peer's id in it!" );
    TODO("Compress stuff here before sending!" );
    
    //Now, send [encrypted]:
    zmq::message_t dataMsg( (void *) ciphertext.c_str(), ciphertext.length(), NULL );
    internal.send( dataMsg );
}


ShinyMsg * ShinyPeer::recv( zmq::socket_t &internal ) {
    //Decode into plaintext
    std::string plaintext;
    
    
    return NULL;
}



const char * ShinyPeer::getPeerAddr( void ) {
    return this->peerAddr;
}