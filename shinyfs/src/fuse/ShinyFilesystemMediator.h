#ifndef SHINYFILESYSTEMMEDIATOR_H
#define SHINYFILESYSTEMMEDIATOR_H
#include <pthread.h>
#include <zmq.hpp>
#include "../filesystem/ShinyFilesystem.h"
#include "../filesystem/ShinyMetaFileHandle.h"
#include <vector>
#include <map>
#include <string>
#include <list>

class ShinyFilesystemMediator {
friend void *mediatorThreadLoop( void * );
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
        //   - NodeType
        //   - ShinyMetaNode
        // [NACK] broker -> fuse
        GETATTR,
        
        // [SETATTR] fuse -> broker
        //   - path
        //   - ShinyMetaNode
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        SETATTR,
        
        // [READDIR] fuse -> broker
        //   - path
        // [ACK] broker -> fuse
        //   - numNodes
        //   - msg per node name
        // [NACK] broker -> fuse
        READDIR,
        
        // [OPEN] fuse -> broker (This "checks out" the file, saves a ShinyMetaFileHandle into the map openFiles, for later use)
        //  - path
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        OPEN,
        
        // [WRITEREQ] fuse -> broker (must have "opened" before)
        //  - path
        // [ACK] broker -> fuse
        //  - ShinyMetaFileHandle (allows the fuse thread to do its business)
        // [NACK] broker -> fuse
        WRITEREQ,
        
        // [WRITEDONE] fuse -> broker (used to allow other writes and closing)
        //  - path
        //  - node
        // NO RESPONSE!  Unnecessary!
        WRITEDONE,
        
        // [READREQ] fuse -> broker (must have "opened" before)
        //  - path
        // [ACK] broker -> fuse
        //  - ShinyMetaFileHandle (allows the fuse thread to do its business)
        // [NACK] broker -> fuse
        READREQ,
        
        // [READDONE] fuse -> broker (used to allow closing)
        //  - path
        //  - node
        // NO RESPONSE!  Unnecessary!
        READDONE,
        
        // [TRUNCREQ] fuse -> broker (resizes the file, requires same privileges as WRITE)
        //  - path
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        TRUNCREQ,
        
        // [TRUNCDONE] fuse -> broker (used to allow writing and closing)
        //  - path
        //  - node
        // No response
        TRUNCDONE,
        
        // [CLOSE] fuse -> broker (must have finished all READ/WRITE's by now)
        //  - path
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        CLOSE,
        
        // [CREATEFILE] fuse -> broker
        //  - path
        //  - permissions (uint16_t)
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        CREATEFILE,
        
        // [CREATEDIR] fuse -> broker
        //  - path
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        CREATEDIR,
        
        // [DELETE] fuse -> broker
        //  - path
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        DELETE,
        
        // [RENAME] fuse -> broker
        //  - path
        //  - newPath
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        RENAME,
        
        // [CHMOD] fuse->broker
        //  - path
        //  - [permissions (uint16_t)]
        // [ACK] broker -> fuse
        // [NACK] broker -> fuse
        CHMOD,
    };

/////// CREATION ////////
public:
    /// Sets up the Mediator, spawns off its own thread, etc....
    ShinyFilesystemMediator( ShinyFilesystem * fs, zmq::context_t * ctx );
    ~ShinyFilesystemMediator();
    
/////// COMMUNICATION ///////
public:
    // Returns the endpoint that FUSE threads talk to
    const char * getZMQEndpointFuse();
    
    // Returns a REQ socket, ready to talk to the mediator, used exstensively by ShinyFuse
    zmq::socket_t * getMediator();
protected:
    // handles messages sent from the FUSE layer
    bool handleMessage( zmq::socket_t * sock, std::vector<zmq::message_t *> & msgList );
    
    // The actual ZMQ context
    zmq::context_t * ctx;
        
    
/////// DATA ///////
protected:
    // The handle to the actual fs that we will manipulate
    ShinyFilesystem * fs;
    
    // The broker thread
    pthread_t thread;
    
    // Stores the route back to the FUSE thread wanting this READ/WRITE, and what kind of file operation it is
    typedef std::pair<zmq::message_t *, uint8_t> QueuedFO;
    
    // Map of open files onto FileHandles, and # of times they've been opened
    struct OpenFileInfo {
        ShinyMetaFile * file;   // The cached "file" object, so that we can give it to people wanting to read/write to it
        uint16_t opens;         // The number of times it's been opened, so that we know when it's actually closed
        bool shouldDelete;      // Whether unlink() was called on this guy before everything was closed, so we should delete it when it gets closed
        bool writeLocked;       // Whether a WRITE or TRUNC is underway, so we can't give out further WRITEs, TRUNCs, READs or fully CLOSE
        bool shouldClose;       // Whether a CLOSE was called while a WRITE or READ was underway, so we defer fully closing until later
        uint16_t reads;         // How many READs are currently underway (not queued)
        std::list<QueuedFO> queuedFileOperations;   // The routing paths and type of each queued read/write, due to a writelock
    };
    std::map<std::string, OpenFileInfo *> openFiles;
    
private:
    // Utility function to send an ACK and a node, routed to fuseRoute
    void sendACK_Node( zmq::socket_t * sock, zmq::message_t *fuseRoute, ShinyMetaNode * node );
    
    // Tries to start up as many queued reads, writes and truncates for a file as it can
    void startQueuedFO( zmq::socket_t * sock, OpenFileInfo * ofi );
    
    // Some simple cleanup to close a file
    void closeOFI( std::map<std::string, OpenFileInfo *>::iterator itty );
};


#endif // SHINYFILESYSTEMMEDIATOR_H
