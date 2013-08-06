#ifndef ZMQUTILS_H
#define ZMQUTILS_H
#include "cppzmq/zmq.hpp"
#include <vector>
#include <sys/types.h>
#include "../filesystem/ShinyMetaNode.h"

// Utility functions for zmq


bool checkMore( zmq::socket_t * sock );
void recvMessages( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList );
void freeMsgList( std::vector<zmq::message_t *> & msgList );
void sendMessages( zmq::socket_t * sock, uint64_t numMsgs, ... );
void sendMessages( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList );


void buildTypeMsg( const uint8_t type, zmq::message_t * msg );
void buildDataMsg( const void * data, uint64_t len, zmq::message_t * msg );
void buildStringMsg( const char * string, zmq::message_t * msg );
void buildNodeMsg( ShinyMetaNode * node, zmq::message_t * msg );

uint8_t parseTypeMsg( zmq::message_t * msg );
char * parseDataMsg( zmq::message_t * msg );
char * parseStringMsg( zmq::message_t * msg );
ShinyMetaNode * parseNodeMsg( zmq::message_t * msg, ShinyMetaNodeSnapshot::NodeType type, ShinyFilesystem * fs );


bool waitForEndpoint( zmq::context_t * ctx, const char * endpoint );

#endif //ZMQUTILS_H
