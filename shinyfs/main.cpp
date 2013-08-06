#include "util/cppzmq/zmq.hpp"
#include <base/Logger.h>
//#include "protocol/ShinyMsg.h"
//#include "protocol/ShinyPartitioner.h"
//#include "ShinyNode.h"

#include "filesystem/ShinyMetaNode.h"
#include "filesystem/ShinyMetaRootDir.h"
#include "filesystem/ShinyMetaFile.h"
#include "filesystem/ShinyMetaDir.h"
//#include "protocol/ShinyNetwork.h"

#include "fuse/ShinyFuse.h"
#include "fuse/ShinyFilesystemMediator.h"

//#include "filesystem/

#define NODE_VERSION        "ShinyFS Node v0.3"

#define TRACKER_ADDRESS     "127.0.0.1"

int main (int argc, const char * argv[]) {
    //getGlobalLogger()->setPrintLoc(0);
    Logger::getGlobalLogger()->setPrintId(0);
    Logger::getGlobalLogger()->setPrintThread(0);
    LOG( "%s starting up....", NODE_VERSION );
//    ShinyFuse::init( "/tmp/shiny" );
    return 0;
}


/*
void printMap( ShinyFilesystem * fs, ShinyMetaDir * root, std::map<inode_t, const std::list<ShinyPeer *> *> * map, uint64_t depth = 0 ) {
    const std::list<inode_t> * children = root->getListing();
    for( std::list<inode_t>::const_iterator itty = children->begin(); itty != children->end(); ++itty ) {
        for( uint64_t i=0;i<depth;++i )
            printf("  ");
        printf("[%llu] %s [", *itty, fs->findNode( *itty )->getName() );
        if( map->find( *itty ) != map->end() ) {
            for( std::list<ShinyPeer *>::const_iterator pitty = (*map)[*itty]->begin(); pitty != (*map)[*itty]->end(); ++pitty ) {
                printf( "%s, ", (*pitty)->getIP() );
            }
        }
        printf( "]\n" );
        
        if( fs->findNode( *itty )->getNodeType() == SHINY_NODE_TYPE_DIR ) {
            printMap( fs, (ShinyMetaDir *)fs->findNode( *itty ), map, depth + 1 );
        }
    }
}


std::map<inode_t, const std::list<ShinyPeer *> *> * testPartitioning( ShinyFilesystem * fs, std::list<inode_t> * rootRegion, std::list<ShinyPeer *> * peers, ShinyPeer * us ) {
    ShinyPartitioner part( fs, rootRegion, peers, us );
    
    const std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * partitioning = part.getRegionPeers();
    for( std::map< std::list<inode_t> *, std::list<ShinyPeer *> >::const_iterator itty = partitioning->begin(); itty != partitioning->end(); ++itty ) {
        printf( "Peers [" );
        for( std::list<ShinyPeer *>::const_iterator peer_itty = (*itty).second.begin(); peer_itty != (*itty).second.end(); ++peer_itty )
            printf( "%s, ", (*peer_itty)->getIP() );
        printf( "] own:\n" );
        for( std::list<inode_t>::iterator inode_itty = (*itty).first->begin(); inode_itty != (*itty).first->end(); ++inode_itty )
            printf("    [%llu] %s\n", *inode_itty, fs->findNode( *inode_itty )->getPath() );
    }
    
    std::map<inode_t, const std::list<ShinyPeer *> *> * inodeToPeer = new std::map<inode_t, const std::list<ShinyPeer *> *>();
    for( std::map< std::list<inode_t> *, std::list<ShinyPeer *> >::const_iterator itty = partitioning->begin(); itty != partitioning->end(); ++itty ) {
        if( (*itty).second.front() != us ) {
            printf("sending in:\n");
            for( std::list<inode_t>::iterator region_itty = (*itty).first->begin(); region_itty != (*itty).first->end(); ++region_itty )
                printf( "[%llu] %s\n", *region_itty, fs->findNode( *region_itty )->getPath() );
            printf( "to be split by:\n" );
            for( std::list<ShinyPeer *>::const_iterator peer_itty = (*itty).second.begin(); peer_itty != (*itty).second.end(); ++peer_itty )
                printf( "%s\n", (*peer_itty)->getIP() );
            std::list<ShinyPeer *> * otherPeers = new std::list<ShinyPeer *>( (*itty).second );
            ShinyPeer * them = otherPeers->front();
            otherPeers->pop_front();
            std::map<inode_t, const std::list<ShinyPeer *> *> * otherMap = testPartitioning( fs, (*itty).first, otherPeers, them );
            for( std::map<inode_t, const std::list<ShinyPeer *> *>::iterator insert_itty = otherMap->begin(); insert_itty != otherMap->end(); ++insert_itty )
                (*inodeToPeer)[(*insert_itty).first] = (*insert_itty).second;
//            inodeToPeer->insert( otherMap->begin(), otherMap->end() );
            
            printf( "post-insert:\n" );
            for( std::map<inode_t, const std::list<ShinyPeer *> *>::iterator nitty = otherMap->begin(); nitty != otherMap->end(); ++nitty ) {
                printf( "[%llu] [", (*nitty).first );
                for( std::list<ShinyPeer *>::const_iterator pitty = (*nitty).second->begin(); pitty != (*nitty).second->end(); ++pitty )
                    printf( "%s", (*pitty)->getIP() );
                printf( "]\n" );
            }
            delete( otherPeers );
        } else {
            for( std::list<inode_t>::iterator inode_itty = (*itty).first->begin(); inode_itty != (*itty).first->end(); ++inode_itty )
                (*inodeToPeer)[*inode_itty] = new std::list<ShinyPeer *>( ((*itty).second) );
        }

    }

    return inodeToPeer;
}

int main (int argc, const char * argv[]) {
    getGlobalLogger()->setPrintLoc(0);
    LOG( "%s starting up....", NODE_VERSION );
    
    LOG( "Sometime, search through and replace list with slist where you can" );

    char * serializedData = NULL;
    FILE * fd = fopen( "serializedData.txt", "r" );
    if( fd ) {
        fseeko( fd, 0, SEEK_END );
        uint64_t size = ftello(fd);
        serializedData = new char[size];
        fseeko( fd, 0, SEEK_SET );
        fread( (void *)serializedData, size, 1, fd );
        fclose(fd);
    }
    
    ShinyFilesystem fs( serializedData );
    if( serializedData ) {
        delete( serializedData );
        serializedData = NULL;
    } else {
        ShinyMetaRootDir * root = (ShinyMetaRootDir *) fs.findNode("/");
        ShinyMetaFile * f00 = new ShinyMetaFile( &fs, "f00", root );
    
/*  ShinyMetaDir * dir0 = new ShinyMetaDir( &fs, "dir0", root );
    new ShinyMetaFile( &fs, "f01", dir0 );
    new ShinyMetaFile( &fs, "f02", dir0 );
    new ShinyMetaFile( &fs, "f03", dir0 );
    new ShinyMetaFile( &fs, "f04", dir0 );
    new ShinyMetaFile( &fs, "f05", dir0 );
    ShinyMetaDir * dir1 = new ShinyMetaDir( &fs, "dir1", dir0 );
    for( int i=0;i<16; ++i ) {
        char name[10];
        sprintf( name, "f%.2d", i+6 );
        new ShinyMetaFile( &fs, name, dir1 );
    }
    dir1->addNode( f0 );
//
    
        f00->write( 0, "This is a test\n", 16 );
        f00->write( ShinyFileChunk::MAX_CHUNK_LEN - 10, "This is also a test\n", 20 );
    }
    fs.print();
    fs.flush();
    if( !fs.sanityCheck() )
        WARN( "Sanity check failed!" );

    ShinyMetaFile * f00 = (ShinyMetaFile *) fs.findNode("/f00");
    char buffer[1024];
    uint64_t bytes_read = f00->read( ShinyFileChunk::MAX_CHUNK_LEN - 10, buffer, 20 );
    buffer[bytes_read] = 0;
    printf( "%s\n", buffer );
    
    
    
/*
    //Make 3 dummy peers
    std::list<ShinyPeer *> peers;
    peers.push_back( new ShinyPeer("a") );
    peers.push_back( new ShinyPeer("b") );
    peers.push_back( new ShinyPeer("c") );
    peers.push_back( new ShinyPeer("d") );    
    std::list<inode_t> rootRegion;
    rootRegion.push_back( fs.findNode("/")->getInode());
    
    ShinyPeer us( "X" );
    std::map<inode_t, const std::list<ShinyPeer *> *> * inodeToPeer = testPartitioning( &fs, &rootRegion, &peers, &us );
    printMap( &fs, (ShinyMetaDir *)fs.findNode("/"), inodeToPeer );
    delete( inodeToPeer );
    
    for( std::list<ShinyPeer *>::iterator itty = peers.begin(); itty != peers.end(); ++itty )
        delete( *itty );
/

    fd = fopen( "serializedData.txt", "w" );
    uint64_t serializedLen = fs.serialize( &serializedData );
    fwrite( serializedData, serializedLen, 1, fd );
    fclose( fd );

    
    return 0;
}
*/
