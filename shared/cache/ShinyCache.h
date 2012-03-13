#pragma once
#ifndef ShinyCache_H
#define ShinyCache_H

#include <zmq.hpp>
#include <pthread.h>

/*
 Deals with actual loading and serving of file data.  Created by ShinyFilesystem,
 and talks exclusively with that same filesystem. In this way, ShinyFilesystem acts as 
 a kind of "broker" between the FUSE layer and the file cache
 */

class ShinyCache {
/////// MAIN ////////
public:
    // Creates our heroic filecache and a separate thread, with the zmq context passed in by ShinyFilesystem
    ShinyCache( const char * filecachePath, zmq::context_t * ctx );
    
    // Clean everything up!
    ~ShinyCache();
    
    // returns the endpoint for talking to the ShinyFilesystem
    const char * getZMQEndpointShinyFS();
    
private:
    //Main thread loop, deals with zmq communication, cache purging, etc...
    void threadLoop();
    
    // threading object 
    pthread_t thread;
    
    
/////// DATA ////////
public:
private:
    zmq::context_t * ctx;
};


#endif
