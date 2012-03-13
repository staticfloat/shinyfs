#ifndef ShinyUser_H
#define ShinyUser_H

#include <cryptopp/rsa.h>
#include <vector>

//Just shorten the names down a bit
typedef CryptoPP::RSA::PrivateKey PrivKey;
typedef CryptoPP::RSA::PublicKey PubKey;

//We'll support a maximum of a kabajillion users
typedef uint64_t user_t;
//We'll support a maximum of a crapton of groups, as well
typedef uint64_t group_t;


//This class is used to denote OTHER users; not me.  :P
class ShinyUser {
public:
    //Construct a shinyuser 
    ShinyUser( const char * name, user_t uid, PubKey publicKey, std::vector<group_t> * groups );
    
    //Construct a shinyuser from serializedData
    ShinyUser( const char * serializedInput );
    
    //Starscream, DECONSTRUCT!!!!
    ~ShinyUser();
    
    //This decrypts a message that has been encrypted with this users private key
    virtual void decryptPrivate( const char * input, size_t len, char * output );
    
    //This encrypts a message with this users PUBLIC key, so ONLY they can open it.
    virtual void encryptPublic( const char * input, size_t len, char * output );
    
    //Get length of this user when serialized
    virtual size_t serializedLen();
    
    //Serialize into output, (basically, just save the name + key)
    virtual void serialize( char * output );
    
    //Unserialize from input
    virtual void unserialize( const char * input );
    
    //Returns the name of this user
    const char * getName();
    
    //Returns the user id
    const user_t getUID();
    
    //Checks to see if a user is a part of a given group
    const bool isInGroup( group_t group );
private:
    //Human-readable identifier for this user
    char * name;
    
    //User ID used in ShinyMetaFS
    user_t uid;
    
    //This is the public key of this user, used to verify handshakes with peers pretending to be him, etc...
    PubKey publicKey;
    
    //Keep track of the groups via their groupid, rather than a pointer to the actual group
    std::vector<group_t> groups;
};



#endif //ShinyUser_H