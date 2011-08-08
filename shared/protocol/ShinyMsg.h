#ifndef SHINYMSG_H
#define SHINYMSG_H

#include <stdint.h>
#include <zmq.hpp>


//Class encapsulating a message sent between tracker <-> node, or node <-> node
class ShinyMsg {
public:
    //Read in a message from a socket
    ShinyMsg( zmq::socket_t * socket );
    
    //Create a new message to be sent out
    ShinyMsg( void * senderData, const unsigned char msgType, const char * data, const uint64_t datalen );
    
    //Cleanup
    ~ShinyMsg();
    
    //Message types for different ShinyMsgs
    enum {
        //Used node -> tracker
        //Requests a new version of a ShinyMetaFilesystem.  Contains all metadata about a filesystem
        //Data: a single uint64_t, denoting the timestamp of the last revision for delta-update purposes
        //A revision of "0" means a full tree, (no delta'ing needed)
        FS_REQ,
        
        //Used tracker -> node
        //Gives ShinyMetaFilesystem data to a node
        //Data: A revision number as the "base", e.g. what the delta update should modify off of (0 for a full, non-delta update)
        //  Followed by the actual tree itself, probably zlib'ed or something
        FS_DATA
    };
    
    //the void* will be replaced with whatever sender id junk I come up with
    const void * getSender();
    const unsigned char getMsgType();
    const char * getData();
    const uint64_t getDataLen();
    
    //loads new data into this msg, ready for the next send() a la zmq_msg_t
    void rebuild( void * senderData, const unsigned char msgType, const char * data, const uint64_t datalen );
    
    //Send this message out on a socket.  You MUST call rebuild() before calling send() again!
    void send( zmq::socket_t * socket );
private:
    //Remember to change this void * to SenderData * or soemthing once I write that
    void * senderData;
    unsigned char msgType;
    char * data;
    uint64_t datalen;
};





#endif //SHINYMSG_H