#ifndef shinyfs_node_ShinyPartitioner_h
#define shinyfs_node_ShinyPartitioner_h

#include <list>
#include <map>
#include "ShinyPeer.h"
#include "../filesystem/ShinyMetaNode.h"
#include "../filesystem/ShinyMetaDir.h"
#include "../filesystem/ShinyMetaFilesystem.h"

//std::map< std::list<inode_t> *, std::list<ShinyPeer *> *> * partitionRegion( ShinyMetaFilesystem * fs, std::list<inode_t> * rootRegion, std::list<ShinyPeer *> * peers, ShinyPeer * us, std::map<inode_t, uint64_t> * weightMap = NULL );


class ShinyPartitioner {
public:
    //Takes in a filesystem, and a list of peers to partition across.
    //Our local "root" is the dir over which we have control, and which we will start partitioning
    //peers should be sorted in order of desireability.  Sort by ping, randomly, whatever
    ShinyPartitioner( ShinyMetaFilesystem *fs, std::list<inode_t> *rootRegion, std::list<ShinyPeer *> *peers, ShinyPeer * us );
    
    ~ShinyPartitioner();
    
    //Returns the union of all regions for which we and we alone are responsible
    const std::list<inode_t> * getOurRegion( void );
    
    //Returns the set of peers directly underneath us (just the front()'s of all regions)
    const std::list<ShinyPeer *> * getOurChildren( void );
    
    //Returns a map mapping regions to lists of peers
    const std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * getRegionPeers( void );
    
    //Prints out a weight map starting at root
    void printWeightMap( ShinyMetaDir *root, uint64_t depth = 0 );
private:
    //Builds a weight map for root, recursing on dirs
    void buildWeightMap( ShinyMetaDir *root );
    
    //Partitions the given region of the weight map amongst the given list of peers
    std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * partitionRegion( std::list<inode_t> *region, std::list<ShinyPeer *> *peers, uint64_t initialWeight = 0 );
    
    
    //Convenience handle to the actual fs object
    ShinyMetaFilesystem * fs;
    
    //Convenience pointer to "us"
    ShinyPeer * us;
    
    //This maps goes from a list of inodes, (a "region") to the list of peers assigned to that
    //region.  The first element in the list is the "owner" of that region.
    std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * regionPeers_final;
    std::list<inode_t> * ourRegion_final;
    std::list<ShinyPeer *> * ourChildren_final;
    
    //Maps inodes to weights
    std::map<inode_t, uint64_t> weightMap;
    
    //Ideal weight per peer
    uint64_t weightPerPeer;
};

#endif