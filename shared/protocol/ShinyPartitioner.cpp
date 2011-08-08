#include "ShinyPartitioner.h"
#include "base/Logger.h"

void buildWeightMap( ShinyMetaFilesystem * fs, ShinyMetaDir *root, std::map<inode_t, uint64_t> * weightMap ) {
    //Traverse through the fs, recursing on directories, summing weights into rootWeight as we go
    uint64_t rootWeight = 0;
    
    const std::list<inode_t> * children = root->getListing();
    for( std::list<inode_t>::const_iterator itty = children->begin(); itty != children->end(); ++itty ) {
        //Even though this shouldn't happen, I'd rather error on the side of safety and do a NULL check
        ShinyMetaNode * child = fs->findNode(*itty);
        if( child && child->getPrimaryParent()->getInode() == root->getInode() ) {
            //Only do this if we are the primary parent
            if( child->getNodeType() == SHINY_NODE_TYPE_FILE) {
                //We always asssign 1 here, which is possibl e a little naive.  We should profile activity on files and assign weights accordingly MAYBE
                (*weightMap)[*itty] = 1;
            } else if( child->getNodeType() == SHINY_NODE_TYPE_DIR || child->getNodeType() == SHINY_NODE_TYPE_ROOTDIR ) {
                //There's no real reason I should ever have a ROOTDIR as a child of another dir, but it makes "sense"
                //that I would want this to still happen if I did, so I'm putting it in there-STOP LOOKING AT ME LIKE THAT
                buildWeightMap( fs, (ShinyMetaDir *)child, weightMap );
            }
            
            //After those two things, update the root weight
            rootWeight += (*weightMap)[*itty];
        }
    }
    (*weightMap)[root->getInode()] = rootWeight;
}

//Helper class to sort inodes based on their weight in the weightMap
struct WeightSorter {
    WeightSorter( std::map<inode_t, uint64_t> & weightMap ) : weightMap( weightMap ) {}
    
    bool operator()(inode_t a, inode_t b) {
        return weightMap[a] < weightMap[b];
    }
    std::map<inode_t, uint64_t> weightMap;
};

std::map< std::list<inode_t> *, std::list<ShinyPeer *> *> * partitionRegion( ShinyMetaFilesystem * fs, std::list<inode_t> * rootRegion, std::list<ShinyPeer *> * peers, ShinyPeer * us, std::map<inode_t, uint64_t> * weightMap, uint64_t weightPerPeer ) {
    bool shouldFreeWeightMap = false;
    //Find the root
    ShinyMetaDir * root = (ShinyMetaDir *)fs->findNode( "/" );

    //This means its the first iteration
    if( !weightMap ) {
        weightMap = new std::map<inode_t, uint64_t>();
        //Begin by fleshing out the weight map along each directory, etc.
        buildWeightMap( fs, root, weightMap );
    
        //Calculate the ideal weight per peer
        weightPerPeer = (uint64_t)ceil( (double)(*weightMap)[root->getInode()]/peers->size() );
    }
    
    //This maps our lists of inodes to the (list of) peers that own them
    std::map< std::list<inode_t> *, uint64_t> regionSizes;
    
    //First, we know that we shall own every inode in root region.  We will also own children until we are full;
    std::list<inode_t> * newRegion = new std::list<inode_t>();
    
    //Save the first region we make, as that region contains "us" and will be checked to make sure it doesn't need to be recursed into
    std::list<inode_t> * ourRegion = newRegion;
    uint64_t regionWeight = 0;
    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty ) {
        newRegion->push_front( *itty );
        
        //Only add on the weight if this node is actually a file, as we don't count the weights
        //of the directories on the top level
        ShinyMetaNode * inode = fs->findNode( *itty );
        if( inode && inode->getNodeType() == SHINY_NODE_TYPE_FILE ) {
            newRegion->push_back( *itty );
            regionWeight += (*weightMap)[*itty];
        }
    }
    
    //Now iterate through all children, starting from the smallest to the largest dirs in the root region.
    //As we iterate, fill up a new region until the weight of that region is greater than or equal to the
    //optimal weight per peer and then mark that region up with as many peers as it needs (floored)
    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty ) {
        ShinyMetaNode * inode = fs->findNode( *itty );
        //If this inode in the root region is a dir, let's look at the children and split 'em up into regions
        if( inode && (inode->getNodeType() == SHINY_NODE_TYPE_DIR || inode->getNodeType() == SHINY_NODE_TYPE_ROOTDIR) ) {
            const std::list<inode_t> * children = ((ShinyMetaDir *)inode)->getListing();
            std::list<inode_t> sortedChildren( *children );
            sortedChildren.sort( WeightSorter( *weightMap ) );
            
            //Now iterate over the children that have been sorted by weight
            for( std::list<inode_t>::iterator child_itty = sortedChildren.begin(); child_itty != sortedChildren.end(); ++child_itty ) {
                //This time around, we _will_ add in weights for the directories, so don't check for type
                regionWeight += (*weightMap)[*child_itty];
                newRegion->push_back( *child_itty );
                
                //If we have more than the threshold of weight, then we need to define this list of inodes as its own region
                if( regionWeight >= weightPerPeer ) {
                    regionSizes[newRegion] = regionWeight/weightPerPeer;
                    newRegion = new std::list<inode_t>();
                    regionWeight = 0;
                }
            }
        }
    }
    
    //Now we have a list of regions along with the number of nodes in each region.  Let's sort the regions by number of peers
    //assigned to each region, and then dole out the peers round-robin style.
    std::list< std::list<inode_t> *> sortedRegions;
    //Basic insertion sort:
    for( std::map< std::list<inode_t> *, uint64_t>::iterator map_itty = regionSizes.begin(); map_itty != regionSizes.end(); ++map_itty ) {
        for( std::list< std::list<inode_t> *>::iterator list_itty = sortedRegions.begin(); list_itty != sortedRegions.end(); ++list_itty ) {
            //Insert before everything smaller than us, so that we go in descending order
            if( regionSizes[*list_itty] < (*map_itty).second ) {
                sortedRegions.insert(list_itty, (*map_itty).first );
                break;
            }
        }
    }
    
    //Now, dole out peers:

    //Now check for the region with "us" in it, if it has a directory in it, we need to recursively call into it until "we" are alone
    return NULL;
}



/*
void ShinyPartitioner::partitionWeightMap( std::list<inode_t> * rootRegion, std::list<ShinyPeer *> *peers ) {
    //This will sort the children of root such that the "lightest" files are at the beginning;
    std::list<inode_t> sortedRegion( *rootRegion );
    sortedRegion.sort( WeightSorter( &this->weightMap ) );
    
    //I save this guy off to the side, as he is the peer that takes the flack for all of the directories in this region
    ShinyPeer * masterOfTheHouse = peers->front();
    
    //This is a list of inodes, comprising the current "region"
    //std::list<inode_t> * currRegion = new std::list<inode_t>();
    
    for( std::list<inode_t>::const_iterator itty = sortedRegion.begin(); itty != sortedRegion.end(); ++itty ) {
        LOG( "%d", weightMap[*itty] );

        //Look at this member of the region
        ShinyMetaNode * member = this->fs->findNode( *itty );
        if( member && member->getPrimaryParent() ) {
            //If it's a file, add it onto the responsibility of the top peer of [peers]
            //Also, sum its weight into that peer's assignedWeights, and if he has too much, either split him off,
            //or make a new region of him and a few buddoes
            if( member->getNodeType() == SHINY_NODE_TYPE_FILE ) {
                peerAssignments[peers->front()].push_back( *itty );
                assignedWeights[peers->front()] += weightMap[*itty];
                
                if( assignedWeights[peers->front()] > this->weightPerNode )
                    peers->
            }
            //If it's a dir, by default it belongs to the first peer, as that peer is MASTER 'O DE 'OUSE
            if( member->getNodeType() == SHINY_NODE_TYPE_DIR || member->getNodeType() == SHINY_NODE_TYPE_ROOTDIR ) {
                peerAssignments[masterOfTheHouse].push_back( *itty );
            }
        }
    }
}*/