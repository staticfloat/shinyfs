#include "ShinyPartitioner.h"
#include "base/Logger.h"


//Helper class to sort inodes based on their weight in the weightMap
struct WeightSorter {
    WeightSorter( std::map<inode_t, uint64_t> & weightMap ) : weightMap( weightMap ) {}
    
    bool operator()(inode_t a, inode_t b) {
        return weightMap[a] < weightMap[b];
    }
    std::map<inode_t, uint64_t> weightMap;
};

ShinyPartitioner::ShinyPartitioner( ShinyMetaFilesystem *fs, std::list<inode_t> *rootRegion, std::list<ShinyPeer *> *peers, ShinyPeer * us ) : fs( fs ), us( us ) {
    //First things first, get the root of the filesystem;
    ShinyMetaDir * root = (ShinyMetaDir *)fs->findNode( "/" );
    if( !root ) {
        ERROR( "Could not find root node!" );
        return;
    }
    
    //Build the weight map for the entire FS:
    this->buildWeightMap( root );
    //this->printWeightMap( root );
    
    //Calculate ideal weight per peer
    uint64_t rootWeight = 0;
    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty )
        rootWeight += weightMap[*itty];
    this->weightPerPeer = (uint64_t)ceil( (double)rootWeight/(peers->size()+1) );
    
    //Finally, start partitioning out!
    this->regionPeers_final = this->partitionRegion( rootRegion, peers );
    
    //Find "our" region, and construct the list of "our" children
    ourChildren_final = new std::list<ShinyPeer *>();
    for( std::map< std::list<inode_t> *, std::list<ShinyPeer *> >::iterator itty = regionPeers_final->begin(); itty != regionPeers_final->end(); ++itty ) {
        if( (*itty).second.front() == us )
            ourRegion_final = (*itty).first;
        else
            ourChildren_final->push_back( (*itty).second.front() );
    }
}

ShinyPartitioner::~ShinyPartitioner() {
    delete( this->ourChildren_final );
    for( std::map< std::list<inode_t> *, std::list<ShinyPeer *> >::iterator itty = this->regionPeers_final->begin(); itty != this->regionPeers_final->end(); ++itty )
        delete( (*itty).first );
    delete( this->regionPeers_final );
}

std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * ShinyPartitioner::partitionRegion( std::list<inode_t> * rootRegion, std::list<ShinyPeer *> * peers, uint64_t initialWeight ) {
//    printf( "Our root region:\n" );
//    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty )
//        printf("[%llu] %s\n", *itty, fs->findNode( (*itty) )->getName() );
    
    //This maps our lists of inodes to the (list of) peers that own them
    std::map< std::list<inode_t> *, uint64_t> regionSizes;
    
    //The list of indoes belonging to each new region we will create
    std::list<inode_t> * newRegion = new std::list<inode_t>();
    //The weight of "newRegion", aka "what is assigned to us"
    uint64_t regionWeight = initialWeight;
    
    //Save the first region we make as ourRegion, as that region contains "us" and will be checked for directories (and most importantly, other peers)
    std::list<inode_t> * ourRegion = newRegion;
    
    //First, we know that we shall own every inode in the root region.  We will also start owning children until we are full;    
    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty ) {
        //Only add on the weight if this node is actually a file, as we don't count the weights
        //of the directories on the top level
        ShinyMetaNode * inode = fs->findNode( *itty );
        if( inode && inode->getNodeType() == SHINY_NODE_TYPE_FILE )
            regionWeight += this->weightMap[*itty];
    }
    
    //Save this, as we'll use it later when recursing
    uint64_t rootWeight = regionWeight;
    
    //Now iterate through all children, starting from the smallest to the largest dirs in the root region.
    //As we iterate, fill up a new region until the weight of that region is greater than or equal to the
    //optimal weight per peer and then mark that region up with as many peers as it needs (floored)
    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty ) {
        ShinyMetaNode * inode = fs->findNode( *itty );
        //If this inode in the root region is a dir, let's look at the children and split 'em up into regions
        if( inode && (inode->getNodeType() == SHINY_NODE_TYPE_DIR || inode->getNodeType() == SHINY_NODE_TYPE_ROOTDIR) ) {
            const std::list<inode_t> * children = ((ShinyMetaDir *)inode)->getListing();
            std::list<inode_t> sortedChildren( *children );
            sortedChildren.sort( WeightSorter( weightMap ) );
            
//            printf( "sorted children of %s:\n", inode->getName() );
//            for( std::list<inode_t>::iterator sitty = sortedChildren.begin(); sitty != sortedChildren.end(); ++sitty )
//                printf( "[%llu] %s\n", weightMap[*sitty], fs->findNode( *sitty)->getName() );
            
            //Now iterate over the children that have been sorted by weight
            for( std::list<inode_t>::iterator child_itty = sortedChildren.begin(); child_itty != sortedChildren.end(); ++child_itty ) {
                //This time around, we _will_ add in weights for the directories, so don't check for type
                regionWeight += this->weightMap[*child_itty];
                newRegion->push_back( *child_itty );
                
                //If we have more than the threshold of weight, then we need to define this list of inodes as its own region
                if( regionWeight >= weightPerPeer ) {
                    regionSizes[newRegion] = (uint64_t) round((double)regionWeight/weightPerPeer);
                    newRegion = new std::list<inode_t>();
                    regionWeight = 0;
                }
            }
        }
    }
    
    if( newRegion->size() )
        regionSizes[newRegion] = regionWeight;
    else if( newRegion != ourRegion )
        delete( newRegion );
    
    //Now we have a list of regions along with the number of nodes in each region.  Let's sort the regions by number of peers
    //assigned to each region:
    std::list< std::list<inode_t> *> sortedRegions;
    //Basic insertion sort:
    for( std::map< std::list<inode_t> *, uint64_t>::iterator map_itty = regionSizes.begin(); map_itty != regionSizes.end(); ++map_itty ) {
//        for( std::list<inode_t>::iterator itty = (*map_itty).first->begin(); itty != (*map_itty).first->end(); ++itty )
//            printf( "[%llu] %s\n", *itty, fs->findNode( (*itty) )->getName() );
        std::list< std::list<inode_t> *>::iterator list_itty;
        for( list_itty = sortedRegions.begin(); list_itty != sortedRegions.end(); ++list_itty ) {
            //Insert before everything smaller than us, so that we go in descending order
            if( regionSizes[*list_itty] < (*map_itty).second )
                break;
        }
        sortedRegions.insert(list_itty, (*map_itty).first );
    }
    
    //Now, dole out peers round-robin style amongst all the regions: (but only if we _have_ regions, LOL)
    std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * regionPeers = new std::map< std::list<inode_t> *, std::list<ShinyPeer *> >();
    if( !sortedRegions.empty() ) {
        std::list<std::list<inode_t> *>::iterator region_itty = sortedRegions.begin();
        
        //Push ourselves onto the very end, so that we can put ourselves on when we hit "our" region;
        peers->push_back( us );
        uint64_t numPeersLeft = peers->size();
        while( !peers->empty() ) {
            if( (*regionPeers)[*region_itty].size() < regionSizes[*region_itty] ) {
                //Special case for "our" region; if it's empty, put us in first!
                if( *region_itty == ourRegion && (*regionPeers)[*region_itty].empty() ) {
                    (*regionPeers)[*region_itty].push_back( us );
                    peers->remove( us );
                }
                else {
                    (*regionPeers)[*region_itty].push_back( peers->front() );
                    peers->pop_front();
                }
            }
//            printf("%llu < %llu\n", (*regionPeers)[*region_itty].size(), regionSizes[*region_itty] );
            ++region_itty;
            if( region_itty == sortedRegions.end() ) {
                //Check to make sure that we made any progress at all this last run through.  If we didn't RAGEQUIT
                if( numPeersLeft == peers->size() )
                    break;
                //If we did, mark that progress and wait until next time.....
                numPeersLeft = peers->size();
                region_itty = sortedRegions.begin();
            }
        }
    }

    //Now check for the region with "us" in it, if it has a directory in it, we need to recursively call into it until "we" are alone
    bool hasAnyDirectories = false;
    for( std::list<inode_t>::iterator itty = ourRegion->begin(); itty != ourRegion->end(); ++itty ) {
        ShinyMetaNode * node = fs->findNode( *itty );
        if( node && node->getNodeType() == SHINY_NODE_TYPE_DIR ) {
            hasAnyDirectories = true;
            break;
        }
    }

    //If there are directories in here, then recurse and pass in the peers assigned to us;
    if( hasAnyDirectories ) {        
        uint64_t ourWeight = rootWeight;
        for( std::list<inode_t>::iterator itty = ourRegion->begin(); itty != ourRegion->end(); ++itty ) {
            ShinyMetaNode * node = fs->findNode( *itty );
            if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
                printf( "Adding on %llu for [%s]\n", weightMap[*itty], fs->findNode(*itty)->getName() );
                ourWeight += weightMap[*itty];
            }
        }
        
        //pop off ourself from this region so that we don't get added in twice to the list of peers
        (*regionPeers)[ourRegion].pop_front();
        
        //Get all the subregion assignments
        std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * subRegionPeers = partitionRegion( ourRegion, &(*regionPeers)[ourRegion], rootWeight );
        
//        //put ourselves back in
//        (*regionPeers)[ourRegion].push_front( us );
                
        //Now, take all the regions that have us (and only us) in them, and glob them together into one ginormous region
        for( std::map< std::list<inode_t> *, std::list<ShinyPeer *> >::iterator itty = subRegionPeers->begin(); itty != subRegionPeers->end(); ++itty ) {
            //If it's us, then merge it into "ourRegion"
            if( (*itty).second.front() == us ) {
                ourRegion->clear();
                ourRegion->merge( *(*itty).first );
/*                
                printf("Post-merge:\n");
                for( std::list<inode_t>::iterator inode_itty = ourRegion->begin(); inode_itty != ourRegion->end(); ++inode_itty ) {
                    printf( "[%llu] %s\n", *inode_itty, fs->findNode( *inode_itty )->getPath() );
                }*/
            } else {
                //Otherwise, merge this region into the big picture
                (*regionPeers)[(*itty).first].merge( (*itty).second );
            }
//            delete( (*itty).first );
        }
        delete( subRegionPeers );
    }
    
    //This covers two cases; first, if there were other people assigned to this region, it should now
    //be only ourselves because we split our region off to other places.  Second, if there is
    //only the "root" region, (e.g. no directories in root) then this will ensure that "ourRegion"
    //gets added to regionPeers
    (*regionPeers)[ourRegion].clear();
    (*regionPeers)[ourRegion].push_front( us );

    
    //Add all the root region inodes in at the very end, so that none of it interferes with our stuffage
    for( std::list<inode_t>::iterator itty = rootRegion->begin(); itty != rootRegion->end(); ++itty )
        ourRegion->push_back( *itty );

    
    //And finally, return the whole map.  ^_^
    return regionPeers;
}

void ShinyPartitioner::buildWeightMap( ShinyMetaDir *root ) {
    //Traverse through the fs, recursing on directories, summing weights into rootWeight as we go
    uint64_t rootWeight = 0;
    
    const std::list<inode_t> * children = root->getListing();
    for( std::list<inode_t>::const_iterator itty = children->begin(); itty != children->end(); ++itty ) {
        //Even though this shouldn't happen, I'd rather error on the side of safety and do a NULL check
        ShinyMetaNode * child = fs->findNode(*itty);
        if( child ) {
            //Only do this if we are the primary parent
            if( child->getNodeType() == SHINY_NODE_TYPE_FILE) {
                //We always asssign 1 here, which is possibl e a little naive.  We should profile activity on files and assign weights accordingly MAYBE
                this->weightMap[*itty] = 1;
            } else if( child->getNodeType() == SHINY_NODE_TYPE_DIR || child->getNodeType() == SHINY_NODE_TYPE_ROOTDIR ) {
                //There's no real reason I should ever have a ROOTDIR as a child of another dir, but it makes "sense"
                //that I would want this to still happen if I did, so I'm putting it in there-STOP LOOKING AT ME LIKE THAT
                buildWeightMap( (ShinyMetaDir *)child );
            }
            
            //After those two things, update the root weight
            rootWeight += this->weightMap[*itty];
        }
    }
    this->weightMap[root->getInode()] = rootWeight;
}

void ShinyPartitioner::printWeightMap( ShinyMetaDir * root, uint64_t depth ) {
    const std::list<inode_t> * children = root->getListing();
    for( std::list<inode_t>::const_iterator itty = children->begin(); itty != children->end(); ++itty ) {
        ShinyMetaNode * child = fs->findNode(*itty);
        if( child) {
            for( uint64_t i=0; i<depth; ++i )
                printf("  ");
            printf("[%llu] %s\n", this->weightMap[*itty], child->getName() );
            if( child->getNodeType() == SHINY_NODE_TYPE_DIR || child->getNodeType() == SHINY_NODE_TYPE_ROOTDIR )
                printWeightMap( (ShinyMetaDir *)child, depth + 1 );
        }
    }
}

const std::list<inode_t> * ShinyPartitioner::getOurRegion( void ) {
    return this->ourRegion_final;
}

const std::list<ShinyPeer *> * ShinyPartitioner::getOurChildren( void ) {
    return this->ourChildren_final;
}

const std::map< std::list<inode_t> *, std::list<ShinyPeer *> > * ShinyPartitioner::getRegionPeers( void ) {
    return this->regionPeers_final;
}