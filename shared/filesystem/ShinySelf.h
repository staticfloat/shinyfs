#ifndef shinyfs_node_ShinySelf_h
#define shinyfs_node_ShinySelf_h

#include "ShinyUser.h"

//This class is used to denote MESELF
class ShinySelf : public ShinyUser {
public:
    ShinySelf( PubKey publicKey, PrivKey privateKey );
    ~ShinySelf();
    
    //This encrypts a message with our private key, so everyone knows its from us
    void encryptPrivate( const char * input, size_t len, char * output );
    
    //This decrypts a message that has been encrypted with our PUBLIC key, so ONLY we can open it
    void decryptPublic( const char * input, size_t len, char * output );
    
    //Get length of this user when serialized
    virtual size_t serializedLen();
    
    //Serialize into output
    virtual void serialize( char * output );
private:
    //This is my _user_ private key
    PrivKey privateKey;
    
    //These are the private keys of each of the groups I belong to
    //Basically, the way I know I am in a group, is that I have the private key.
    std::map<group_t, PrivKey> groupKeys;
};


#endif
