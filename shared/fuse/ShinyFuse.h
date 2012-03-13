#pragma once
#ifndef shinyfs_node_ShinyFuse_h
#define shinyfs_node_ShinyFuse_h

#include <pthread.h>
#include <zmq.hpp>
#include <vector>

//Include FUSE here
#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION  26
#include <fuse/fuse.h>

#include "../filesystem/ShinyFilesystem.h"

class ShinyFuse {
/////// ENUMS ///////
public:
    enum MsgType {
        // [ACK] broker -> fuse (says things are alright. response data varies by originating type)
        ACK,
        
        // [NACK] broker -> fuse (says things are HORRIBLY BROKEN!)
        NACK,
        
        // [DESTROY] fuse -> broker
        // [ACK] broker -> fuse
        DESTROY,
        
        // [GETATTR] fuse -> broker
        //   - path
        // [ACK] broker -> fuse
        //   - stbuff
        // [NACK] broker -> fuse
        GETATTR,
        
        // [READDIR] fuse -> broker
        //   - path
        // [ACK] broker -> fuse
        //   - numNodes
        //   - msg per node name
        // [NACK] broker -> fuse
        READDIR,
    };
    
/////// CREATION ///////
public:
    //Initializes the FUSE interface, sets up the callbacks, etc....
    static bool init( ShinyFilesystem * fs, zmq::context_t * ctx, const char * mountPoint = "/tmp/shiny/" );
private:
    
    
/////// FUSE ////////
private:
    //Initialization and desruction
    static void * fuse_init( struct fuse_conn_info * conn );
    static void fuse_destroy( void * private_data );
    
    //Gets info about a file
    static int fuse_getattr( const char * path, struct stat * stbuf );
    
    //Gets a directory listing
    static int fuse_readdir( const char * path, void * output, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi );
    
    static int fuse_open( const char * path, struct fuse_file_info * fi );
    static int fuse_read( const char * path, char * buffer, size_t len, off_t offset, struct fuse_file_info * fi );
    static int fuse_write( const char * path, const char * buffer, size_t len, off_t offset, struct fuse_file_info * fi );
    
    static int fuse_truncate( const char * path, off_t len );
    
    static int fuse_mknod( const char * path, mode_t permissions, dev_t device );
    static int fuse_create( const char * path, mode_t permissions, struct fuse_file_info * fi );
    static int fuse_mkdir( const char * path, mode_t permissions );
    
    static int fuse_unlink( const char * path );
    static int fuse_rmdir( const char * path );
    
    static int fuse_rename( const char * path, const char * newPath );
    
    static int fuse_access( const char * path, int mode );
    
    static int fuse_chmod( const char * path, mode_t mode );
    static int fuse_chown( const char * path, uid_t uid, gid_t gid );
    
    static ShinyFilesystem * fs;
    
    
/////// ZMQ BROKER ///////
private:
    // This is the thread that all the fuse stuff will talk to, and it will hold the fs tree
    static void * broker( void * data );
    
    // This is where the healing begins.  Or the real work happens.  whichever, really
    static bool handleMessage( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList );
    
    // Returns the ZMQ endpoint for the broker to talk to FUSE peeps
    static const char * getZMQEndpointFuse();

    // The thread object used for pthread_join() in fuse_destroy()
    static pthread_t brokerThread;
    
    // The context object used to create all sockets
    static zmq::context_t * ctx;
    
    
/////// ZMQ FUSE ///////
private:
    static zmq::socket_t * getBroker();
};

#endif
