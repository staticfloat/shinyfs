#include "ShinyUser.h"
#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include "base/Logger.h"

ShinyUser::ShinyUser( const char * name, user_t uid, PubKey publicKey, std::vector<group_t> * groups ) {
    this->name = new char[strlen(name)+1];
    strcpy( this->name, name );
    
    this->uid = uid;
    this->publicKey = publicKey;

    //Reserve space, and then move the groups in!
    this->groups.reserve(groups->size());
    for( size_t i=0; i<groups->size(); ++i )
        this->groups[i] = (*groups)[i];
    
    //Sort so we can quit out early when 
    std::sort( this->groups.begin(), this->groups.end() );
}

ShinyUser::~ShinyUser() {
    delete( this->name );
}

void ShinyUser::decryptPrivate( const char *input, size_t len, char *output ) {
    WARN( "I don't know if this works, as I don't know what the sizes for encryption/decryption look like!" );
    CryptoPP::AutoSeededRandomPool rng;
    
    std::string decrypted;
    CryptoPP::RSAES_OAEP_SHA_Decryptor d(publicKey);
    CryptoPP::PK_DecryptorFilter * filter = new CryptoPP::PK_DecryptorFilter( rng, d, new CryptoPP::StringSink( decrypted ) );
    CryptoPP::StringSource( (const byte*)input, len, true, filter );
    
    decrypted.copy( output, len );
}

void ShinyUser::encryptPublic( const char *input, size_t len, char *output ) {
    WARN( "I don't know if this works, as I don't know what the sizes for encryption/decryption look like!" );
    CryptoPP::AutoSeededRandomPool rng;
    
    std::string encrypted;
    CryptoPP::RSAES_OAEP_SHA_Encryptor e(publicKey);
    CryptoPP::PK_EncryptorFilter * filter = new CryptoPP::PK_EncryptorFilter( rng, e, new CryptoPP::StringSink( encrypted ) );
    CryptoPP::StringSource( (const byte*) input, len, true, filter );
    
    encrypted.copy( output, len );
}


size_t ShinyUser::serializedLen() {
    size_t len = 0;
    
    //First, the uid
    len += sizeof(user_t);
    
    //The number of groups, and then the groups themselves
    len += sizeof(size_t) + groups.size()*sizeof(group_t);
    
    //Finally, the name
    len += strlen(name) + 1;
    
    return len;
}

void ShinyUser::serialize( char *output ) {
    *((user_t *)output) = uid;
    output += sizeof(user_t);
    
    //Output the number of groups
    *((size_t *)output) = groups.size();
    output += sizeof(size_t);
    
    //And then the groups themselves
    for( size_t i=0; i<groups.size(); ++i ) {
        *((group_t *)output) = groups[i];
        output += sizeof(group_t);
    }
    
    //Finally, output the name
    strcpy( output, name );
}

void ShinyUser::unserialize( const char *input ) {
    uid = *((user_t *)input);
    input += sizeof(user_t);
    
    size_t numGroups = *((size_t *)input);
    groups.reserve( numGroups );
    input += sizeof(size_t);
    
    for( size_t i=0; i<numGroups; ++i ) {
        groups[i] = *((group_t *)input);
        input += sizeof(group_t);
    }
    
    name = new char[strlen(input)+1];
    strcpy( name, input ); 
}


const char * ShinyUser::getName( void ) {
    return this->name;
}

const user_t ShinyUser::getUID( void ) {
    return this->uid;
}

const bool ShinyUser::isInGroup( group_t group ) {
    size_t i=0;
    while( i < this->groups.size() ) {
        if( groups[i] == group )
            return true;
        
        //We get to quit out early because it's sorted!  wooooooo!
        if( groups[i] > group )
            return false;
        ++i;
    }
    return false;
}