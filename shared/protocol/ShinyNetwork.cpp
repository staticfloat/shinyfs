#include "ShinyNetwork.h"
#include <pthread.h>
#include "base/Logger.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/if.h>

void handleRecvFailure( zmq::context_t *ctx, zmq::socket_t **socket ) {
    //If we just ran into an error, check to see if it's just because we didn't have anything to read
    if( errno != EAGAIN ) {
        //First, get the id
        char id[255];
        size_t idLen = 255;
        (*socket)->getsockopt( ZMQ_IDENTITY, &id, &idLen );
        
        //ruh-roh
        ERROR( "recv() failed on %s: %s (%d)", id, strerror( errno ), errno );
        
        //Delete and re-create internal socket to try and fix errors
        delete( *socket );
        *socket = new zmq::socket_t( *ctx, ZMQ_ROUTER );

        //Generate the zmq identity we need to reopen the socket
        char * zmqId = new char[strlen(id) + 10];
        sprintf( zmqId, "inproc://%s", id );
        
        //Bind to that id
        (*socket)->bind( zmqId );
        
        //cleanup
        delete( zmqId );
    }
}

//Returns true if the socket has more to be read
bool checkMore( zmq::socket_t * sock ) {
    int64_t opt;
    size_t optLen = sizeof(opt);
    sock->getsockopt(ZMQ_RCVMORE, &opt, &optLen);
    return opt;
}

//Frees the list of messages passed out of recvMessages
void freeMsgList( std::vector<zmq::message_t *> * msgList ) {
    //for( std::vector<zmq::message_t *>::iterator itty = (*msgList)->begin(); itty != (*msgList)->end(); ++itty ) {
    for( int i=0; i<msgList->size(); ++i ) {
        delete( (*msgList)[i] );
    }
}

//Returns all linked messages on a socket in a vector
std::vector<zmq::message_t *> * recvMessages( zmq::context_t *ctx, zmq::socket_t ** sock ) {
    //First, initialize the return value
    std::vector<zmq::message_t *> * msgList = new std::vector<zmq::message_t *>();
    
    //Try and receive a message
    zmq::message_t * msg;
    
    do {
        //Check for new message
        msg = new zmq::message_t();
        if( (*sock)->recv( msg, ZMQ_NOBLOCK ) == false ) {
            handleRecvFailure( ctx, sock );
            freeMsgList( msgList );
            return msgList;
        }
        
        //Push it onto the vector
        msgList->push_back(msg);
    } while( checkMore( *sock ) );
    return msgList;
}


void printMsg( zmq::message_t &msg ) {
    printf("[%lu] ", msg.size() );
    char * data = (char *) msg.data();
    for( int i=0; i<msg.size(); ++i ) {
        if( isprint(data[i]) )
            printf( "%c", data[i] );
        else
            printf( "?" );
    }
    printf("\n");
}


void * ShinyCommunicator( void * data ) {
    TODO( "Do less allocations in the loop, especially msgList" );
    ShinyNetwork * net = (ShinyNetwork *)data;
    
    //Create our internal listening socket....
    zmq::socket_t * internal = new zmq::socket_t( *net->context, ZMQ_ROUTER );
    internal->bind( "inproc://comm.internal" );
    
    //Create our external node-comm socket....
    zmq::socket_t * external_node = new zmq::socket_t( *net->context, ZMQ_ROUTER );
    external_node->bind("tcp://*:5040");
    
    //Create our external subscription socket....
    //We can't connect it yet, as we haven't found the highest peer possible yet to subscribe to.
    zmq::socket_t * external_sub = NULL; // new zmq::socket_t( *net->context, ZMQ_SUB );
    
    //Run forever until someone signals us to stop by setting our runlevel to COMM_STOP
    net->commRunLevel = ShinyNetwork::COMM_RUNNING;
    while( net->commRunLevel != ShinyNetwork::COMM_STOP ) {
        //Check for outgoing messages
        std::vector<zmq::message_t *> * msgList = recvMessages( net->context, &internal );
        //Note that even though this is called msgList, it is really just one message, it's just all the parts
        if( !msgList->empty() ) {
            //dump everything out for fun
            for( int i=0; i<msgList->size(); ++i )
                printMsg( *(*msgList)[i] );
            printf( "------------------------------------------------------------\n" );
            
            //Now, let's make sure we're following protocol:
            if( (*msgList)[1]->size() < strlen("::1") ) {
                WARN( "Peer address should be of the form x.x.x.x:yyyy, or xxxx:xxxx::1:yyyy! [%lu]", (*msgList)[1]->size() );
            } else {
                //This gets the address of the peer I'm trying to talk to
                std::string peerAddr( (char *)(*msgList)[1]->data(), (*msgList)[1]->size() );
                                
                //Search for this peer in our ShinyPeer database.  If it's not there yet, we'll have to reach out to him
                std::tr1::unordered_map<std::string, ShinyPeer *>::iterator itty = net->PD_addr.find( peerAddr );
                if( itty == net->PD_addr.end() ) {
                    try {
                        //Try to connect to other peer
                        external_node->connect( ("tcp://" + peerAddr).c_str() );
                        
                        //If it works (we would have thrown by now if we couldn't do that)
                        internal->send( *(*msgList)[0], ZMQ_SNDMORE );    //internal routing code
                        
                        //Send back the OK so that the worker thread can initiate handshake etc....
                        zmq::message_t okMsg( (void *)"OK", 2, NULL );
                        internal->send( okMsg );
                    } catch( zmq::error_t e ) {
                        //Ruh-roh, we have a problemo!
                        WARN( "Could not connect to peer %s: %s", peerAddr.c_str(), e.what() );
                        
                        //Send back blank message to indicate failure
                        internal->send( *(*msgList)[0], ZMQ_SNDMORE );    //internal routing code                        
                        zmq::message_t blankMsg;
                        internal->send( blankMsg );
                    }
                } else {
                    //We begin to direct crap at that unforunate peer
                    external_node->send( *(*msgList)[1], ZMQ_SNDMORE );   //route to unfortunate peer
                    external_node->send( *(*msgList)[0], ZMQ_SNDMORE );   //internal routing id, so that WE know where to send replies
                    
                    //All subsequent message parts are to be sent to the node:
                    for( int i=3; i<msgList->size() - 1; ++i )
                        external_node->send( *(*msgList)[i], ZMQ_SNDMORE );
                    external_node->send( *(*msgList)[msgList->size()-1] );
                }
            }
        }
        freeMsgList(msgList);
        
        //Check for incoming messages from other nodes....
        msgList = recvMessages( net->context, &external_node );
        if( !msgList->empty() ) {
            //If it knows where it's going, it must be a reply from something we sent out
            if( (*msgList)[1]->size() && !(*msgList)[2]->size() ) {
                //So now we just pass on the joy
                for( int i=0; i<msgList->size()-1; ++i )
                    internal->send( *(*msgList)[i], ZMQ_SNDMORE );
                internal->send( *(*msgList)[msgList->size()-1] );
            }
        }        
        
        //Check if subscription socket is still unconnected...
        if( !external_sub ) {
            TODO( "Connect to highest peer here...." );
            //external_sub = new zmq::socket_t( *net->context, ZMQ_SUB );
            //external_sub->connect( "tcp://SOMEWHERE:5040" );
        } else {
            TODO( "Check for updates here, then pass them on to our pool of listening threads" );
        }
        
        //Sleep for .5 msecs
        usleep( 500 );
    }
    
    //Then exit.  :)
    return NULL;
}



ShinyNetwork::ShinyNetwork( const char * peerAddr ) : externalIP(NULL), internalIP(NULL) {
    //First things first, create the context
    this->context = new zmq::context_t(2);
    
    //Find our external address by asking e.saba.us
    this->findInternalIP( true );
    if( !this->findExternalIP( true ) ) {
        //If it doesn't work, we'll just use our internal address
        if( this->internalIP ) {
            this->externalIP = new char[strlen(this->internalIP)+1];
            strcpy( this->externalIP, this->internalIP );
        } else
            this->externalIP = NULL;
    }
    
    //Next, setup the communication thread;
    this->commRunLevel = ShinyNetwork::COMM_STARTUP;
    int err;
    if( (err = pthread_create(&commThread, NULL, &ShinyCommunicator, this)) != 0 ) {
        ERROR( "pthread_create() failed!  Error code: %d", err );
        throw "pthread_create() failed!";
    }
    
    //Sleep for 1ms periods while we wait for the network interface to come online
    while( this->commRunLevel == ShinyNetwork::COMM_STARTUP )
        usleep( 1*1000 );
    
    //Connect to the first peer we know about
    ShinyPeer * bootstrap = this->getPeer( peerAddr );

    LOG("Ask for stuff here" );
}


ShinyNetwork::~ShinyNetwork() {
    //Signal to the ShinyCommunicator thread that it's time to cleanup
    this->commRunLevel = ShinyNetwork::COMM_STOP;
    
    pthread_join(commThread, NULL);
    LOG( "commThread exited!" );
}

bool ShinyNetwork::resolveHost( const char * hostname, const char * port, sockaddr ** sa, socklen_t * len, char ** ip ) {
    //Get address info for hostname....
    struct addrinfo *ai;
    struct addrinfo hints;
    hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    int error = getaddrinfo( hostname, port, NULL, &ai );
    if( error != 0 ) {
        ERROR( "getaddrinfo() failed for %s: %s [error code %d]", hostname, gai_strerror(error), error );
        return false;
    }
    
    TODO( "test ipv6 support!" );
    
    if( ip ) {
/*        for( struct addrinfo * res = ai; res != NULL; res = res->ai_next ) {
            inet_ntop( res->ai_family, &res->ai_addr->sa_data[2], *ip, INET6_ADDRSTRLEN );
            LOG( "%s -> %s", hostname, ip );
        }*/
        *ip = (char *) malloc( INET6_ADDRSTRLEN );
        inet_ntop( ai->ai_family, &ai->ai_addr->sa_data[2], *ip, INET6_ADDRSTRLEN );
    }
    
    //Save the sockaddr we're gonna return
    *sa = (sockaddr *) new char[ai->ai_addrlen];
    *len = ai->ai_addrlen;
    memcpy( *sa, ai->ai_addr, ai->ai_addrlen );
    
    //Clean up afterwards
    freeaddrinfo( ai );
    
    return true;
}

//Gets the local IP of our default interface
const char * ShinyNetwork::findInternalIP( bool forceRefresh ) {
    if( !this->internalIP || forceRefresh ) {
        delete( this->internalIP );
        this->internalIP = NULL;
        
        struct ifaddrs * ifAddrStruct=NULL;
        struct ifaddrs * ifa=NULL;
        
        getifaddrs(&ifAddrStruct);
        
        //Loop through all interface thingys, and just take the first, non-loopback ipv4 address we find.  Failing that, take an ipv6 address.
        for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
            if( !(ifa->ifa_flags & IFF_LOOPBACK) ) {
                if( ifa->ifa_addr->sa_family == AF_INET ) { // check it is IP4
                    delete( this->internalIP );
                    this->internalIP = new char[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, this->internalIP, INET_ADDRSTRLEN);
                    break;  //break out of for loop
                } else if( ifa->ifa_addr->sa_family == AF_INET6 && !this->internalIP ) { // check it is IP6
                    this->internalIP = new char[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, this->internalIP, INET6_ADDRSTRLEN);
                }
            }
        }
        if( ifAddrStruct != NULL )
            freeifaddrs( ifAddrStruct );
    }
    return this->internalIP;
}

//Queries e.saba.us:6970 to get our external IP
const char * ShinyNetwork::findExternalIP( bool forceRefresh ) {
    if( !this->externalIP || forceRefresh ) {
        //First, try to resolve "e.saba.us":
        sockaddr * sa;
        socklen_t sa_len;
        const char * extIPServer = "e.saba.us";
        const char * port = "6970";
        if( !resolveHost( extIPServer, port, &sa, &sa_len ) )
            return this->externalIP;
        
        int extSock = socket(sa->sa_family, SOCK_STREAM, NULL );
        if( ::connect( extSock, (const sockaddr *) sa, sa_len ) != 0 ) {
            ERROR( "connect() failed to %s: %s", extIPServer, strerror(errno) );
            return this->externalIP;
        }
        
        char ip[INET6_ADDRSTRLEN];
        long read = recv( extSock, (void *) &ip[0], INET6_ADDRSTRLEN, NULL );
        
        if( read == -1 ) {
            ERROR( "recv() failed: %s", strerror(errno) );
            return this->externalIP;
        }
        
        delete( this->externalIP );
        this->externalIP = new char[read];
        memcpy( this->externalIP, &ip[0], read );
        
        delete( sa );
    }
    return this->externalIP;
}

ShinyPeer * ShinyNetwork::getPeer( const char * addr ) {
    //First, check to see if we have him stored anywhere;
    std::tr1::unordered_map<std::string, ShinyPeer *>::iterator itty = this->PD_addr.find( addr );
    if( itty != this->PD_addr.end() ) {
        return (*itty).second;
    }
    
    //here's where we try to connect to this jerkface
    zmq::socket_t * tempInternal = new zmq::socket_t( *this->context, ZMQ_DEALER );
    tempInternal->connect( "inproc://comm.internal" );
    
    ShinyPeer * newPeer = new ShinyPeer( *tempInternal, addr );
    delete( tempInternal );
    return newPeer;
}