#ifndef SHINYMSG_H
#define SHINYMSG_H

#include <stdint.h>
#include <zmq.hpp>

class ShinyPeer;
struct ShinyMsg {
    //Message types for different ShinyMsgs
    enum ShinyMsgType {
        //Generic Error message
        //Data: String summary of error message
        SHINYMSG_ERROR,
        
        //When a peer connects to the network for the very first time, it needs to know the network information
        //This requests a dump of user keys, information, etc...
        //Data: NONE
        SHINYMSG_NETWORK_INFO,
        
        //When a peer connects to another, it first sends this challenge, which is 
        SHINYMSG_HANDSHAKE_CHALLENGE,
        SHINYMSG_HANDSHAKE_RESPONSE,
        
        //Requests a new version of a ShinyFilesystem.  Contains all metadata about a filesystem
        //Data: ignored for now (should be empty), possible inclusion of delta updates in the future requiring a revision number here
        //Sent via: node-comm
        SHINYMSG_FS_REQ,
        
        //Response to SHINYMSG_FS_REQ, gives a node the latest version of the fs metadata
        //Data: A serialized representation of the filesystem tree
        //Sent via: node-comm as response to SHINYMSG_FS_REQ, or regular publishing sent from root node
        SHINYMSG_FS_DATA,
        
        //Requests a file chunk from a node
        //Data: the byte offset(s) and lengths of the file that are requested
        //Sent via: node-comm
        SHINYMSG_FILE_REQ,
        
        //Response to SHINYMSG_FILE_REQ, gives a node a SINGLE chunk of file data
        //Data: the ShinyFileChunk data requested
        //Sent via: node-comm
        SHINYMSG_FILE_DATA,
        
        //Request for a write-lock on a file or directory
        //Data: the inode_t we're asking for a write-lock on
        //Sent via: node-comm
        SHINYMSG_WRITELOCK_REQ,
        
        //Writelock is granted
        //Data: ignored for now (should be empty)
        //Sent via: node-comm
        SHINYMSG_WRITELOCK_YES,
        
        //Writelock is refused (due to timeout in another node releasing it, etc...)
        //Data: ignored for now (should be empty)
        //Sent via: node-comm
        SHINYMSG_WRITELOCK_NO,
    };

    //////// INCOMING AND OUTGOING ////////
    ShinyMsgType type;
    char * data;
    
    //////// INCOMING ONLY ////////
    ShinyPeer * peer;
};

/*
//ShinyMsg - Class encapsulating a message sent between nodes
class ShinyPeer;
class ShinyMsg {
public:
    //Read in a message from a socket.  Note that 
    ShinyMsg( zmq::socket_t * socket, ShinyPeer * peer );
    
    //Create a new message to be sent out
    ShinyMsg( const unsigned char msgType, const char * data, const uint64_t datalen );
    
    //Cleanup
    ~ShinyMsg();
    
        
    //the void* will be replaced with whatever sender id junk I come up with
    const ShinyPeer * getPeer();
    const unsigned char getMsgType();
    const char * getData();
    const uint64_t getDataLen();
    
    //Send this message out on a socket
    void send( zmq::socket_t * socket );
private:
    //Remember to change this void * to SenderData * or soemthing once I write that
    ShinyPeer * peer;
    unsigned char msgType;
    char * data;
    uint64_t datalen;
};
/**/




#endif //SHINYMSG_H