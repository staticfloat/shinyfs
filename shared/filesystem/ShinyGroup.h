#ifndef ShinyGroup_H
#define ShinyGroup_H

#include "ShinyUser.h"

class ShinyGroup {
public:
    ShinyGroup();
    
    //Returns the length of this group when serialized
    virtual uint64_t serializedLen();
    
    //Serializes into output
    virtual void serialize( char * output );
    
    //Returns the name of this group
    const char * getName();
    
    //Returns the group id
    const group_t getGID();
private:
    //Human-readable identifier for this group
    char * name;
    
    //Group ID used in ShinyMetaFS
    group_t gid;
    
    //This is the public key of this group
    PubKey publicKey;
};


#endif //ShinyGroup_H
