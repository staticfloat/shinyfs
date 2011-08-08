#include "ShinyMsg.h"
#include <base/Logger.h>

#if defined( _DEBUG )

#define CHECK_MORE( socket ) { \
    int64_t opt; \
    size_t optlen = sizeof( int64_t ); \
    socket->getsockopt( ZMQ_RCVMORE, &opt, &optlen ); \
    if( !opt ) { \
        ERROR( "Expected another message part!" ); \
    } \
}

#define CHECK_NOT_MORE( socket ) { \
    int64_t opt; \
    size_t optlen = sizeof( int64_t ); \
    socket->getsockopt( ZMQ_RCVMORE, &opt, &optlen ); \
    if( opt ) { \
        ERROR( "Unexpected message part(s)!" ); \
    } \
}
#else
#define CHECK_MORE( socket )
#define CHECK_NOT_MORE( socket )
#endif //_DEBUG

#include <stdio.h>

ShinyMsg::ShinyMsg( zmq::socket_t * socket ) {
    try { 
        zmq::message_t msg;
        
        //RECEIVE SENDER INFO
        socket->recv( &msg );
        //Don't do anything with it right now because we don't have a sender class yet

        
        //RECEIVE MESSAGE TYPE
        CHECK_MORE( socket );
        msg.rebuild();
        socket->recv( &msg );
        if( msg.size() != sizeof(unsigned char) ) {
            ERROR( "msgtype length was %d, not %d!", msg.size(), sizeof(unsigned char) );
        } else
            memcpy( &this->msgType, msg.data(), sizeof(msgType) );
        
        
        //RECEIVE DATA
        CHECK_MORE( socket );
        msg.rebuild();
        socket->recv( &msg );
        
        datalen = msg.size();
        data = new char[datalen];
        void * temp = msg.data();
        memcpy( &data[0], temp, datalen );
        
        CHECK_NOT_MORE( socket );
    } catch( zmq::error_t e ) {
        ERROR( "ZMQ error: %s", e.what() );
    } catch( ... ) {
        ERROR( "HHHWWWHHAAAT?!" );
    }
}

ShinyMsg::ShinyMsg( void * senderData, unsigned char msgType, const char * data, uint64_t datalen ) {
    rebuild( senderData, msgType, data, datalen );
}

ShinyMsg::~ShinyMsg() {
    delete( data );
}

const void * ShinyMsg::getSender() {
    return senderData;
}

const unsigned char ShinyMsg::getMsgType() {
    return msgType;
}

const char * ShinyMsg::getData() {
    return data;
}

const uint64_t ShinyMsg::getDataLen() {
    return datalen;
}

void ShinyMsg::rebuild(void *senderData, const unsigned char msgType, const char *data, const uint64_t datalen) {
    this->msgType = msgType;
    this->senderData = senderData;
    this->datalen = datalen;
    this->data = new char[datalen];
    memcpy( this->data, data, datalen );
}

void delete_dummy (void *data, void *hint) {
    delete( (char *)data );
}

void ShinyMsg::send( zmq::socket_t * socket ) {
    //First, send the senderData, but as we don't have a class for that yet, just leave it empty
    try {
        zmq::message_t msg(0);
        socket->send( msg, ZMQ_SNDMORE );
        
        //Next, send the message type
        msg.rebuild( &msgType, sizeof(msgType), NULL );
        socket->send( msg, ZMQ_SNDMORE );
        
        //Aaaaand send the data
        msg.rebuild( data, datalen, delete_dummy );
        data = NULL;    //Now it's up to zmq to delete it
        datalen = 0;

        socket->send( msg );
    } catch( zmq::error_t e ) {
        ERROR( "ZMQ error: %s", e.what() );
    } catch( ... ) {
        ERROR( "HHHWWWHHAAAT?!" );
    }
}