#ifndef shinyfs_node_ShinyPartitioner_h
#define shinyfs_node_ShinyPartitioner_h

#include <list>
#include <map>
#include "ShinyPeer.h"
#include "../filesystem/ShinyMetaNode.h"
#include "../filesystem/ShinyMetaDir.h"
#include "../filesystem/ShinyMetaFilesystem.h"

/* partitionRegion:
    Takes in number of peers available, spits out all peers assigned to each region, where a region is
    the set of all inodes strictly under inodes in the root region.
*/

//
//the set of all nodes strictly UNDER elements in the rootRegion.
std::map< std::list<inode_t> *, std::list<ShinyPeer *> *> * partitionRegion( ShinyMetaFilesystem * fs, std::list<inode_t> * rootRegion, std::list<ShinyPeer *> * peers, ShinyPeer * us, std::map<inode_t, uint64_t> * weightMap = NULL );

/*
class ShinyPartitioner {
public:
    //Takes in a filesystem, and a list of peers to partition across.
    //Our local "root" is the dir over which we have control, and which we will start partitioning
    //peers should be sorted in order of desireability.  Sort by ping, randomly, whatever
    ShinyPartitioner( ShinyMetaFilesystem *fs, std::list<inode_t> *rootRegion, std::list<ShinyPeer *> *peers );
    
    //Prints out a weight map
    void printWeightMap( ShinyMetaDir *root, uint64_t depth = 0 );
private:
    //Builds a weight map for root, recursing on dirs
    void buildWeightMap( ShinyMetaDir *root );
    
    //Partitions the given region of the weight map amongst the given list of peers
    void partitionWeightMap( std::list<inode_t> *region, std::list<ShinyPeer *> *peers );
    
    //This maps goes from a list of inodes, (a "region") to the list of peers assigned to that
    //region.  The individual entries of these 
    std::map< std::list<inode_t> *, std::list<ShinyPeer *> > regions;
    
    
    
    
    
    
    ShinyMetaFilesystem * fs;
    
    //Maps inodes to weights
    std::map<inode_t, uint64_t> weightMap;
    
    //Maps peers to the weights of what they've been assigned
    std::map<ShinyPeer *, uint64_t> assignedWeights;
    std::map<ShinyPeer *, std::list<inode_t> > peerAssignments;
    
    uint64_t weightPerNode;
};
*/

#endif