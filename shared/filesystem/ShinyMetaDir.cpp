#include "ShinyMetaDir.h"
#include "ShinyFilesystem.h"
#include "base/Logger.h"

ShinyMetaDir::ShinyMetaDir( ShinyFilesystem * fs, const char * newName ) : ShinyMetaNode( fs, newName ) {
}

ShinyMetaDir::ShinyMetaDir( ShinyFilesystem * fs, const char * newName, ShinyMetaDir * parent ) : ShinyMetaNode( fs, newName ) {
    parent->addNode( this );
}

ShinyMetaDir::ShinyMetaDir( const char * serializedInput, ShinyFilesystem * fs ) : ShinyMetaNode( serializedInput, fs ) {
    this->unserialize( serializedInput + ShinyMetaNode::serializedLen() );
}

ShinyMetaDir::~ShinyMetaDir( void ) {
    while( !nodes.empty() ) {
        //If there _is_ a child that doesn't exist, that's fine, 'cause deleting NULL doesn't do squat!
        delete( nodes.front() );
    }
}

void ShinyMetaDir::addNode( ShinyMetaNode *newNode ) {
    if( newNode->getParent() ) {
        ERROR( "Cannot add a node that already has a parent!" );
        return;
    }
    //Insert so that list is always sorted in ascending inode order
    int64_t i;
    for( i=0; i<this->nodes.size(); ++i ) {
        int ret = strcmp( this->nodes[i]->getName(), newNode->getName() );
        // This means we are in a position to insert our new node into the list
        if( ret == -1 ) {
            // Do a quick check against the next node (if there is one) to make sure they're no duplicates
            if( i < this->nodes.size() - 1 && strcmp( this->nodes[i+1]->getName(), newNode->getName() ) == 0 ) {
                WARN( "Cannot add node %s to %s again!  duplicates are not allowed!", newNode->getName(), this->getName() );
                return;
            }
            break;
        }
    }
    nodes.insert(nodes.begin() + i, newNode );
    newNode->setParent( this );
    
    TODO( "Change this to work on parents, not the whole tree!" );
    //this->fs->setDirty();
}

void ShinyMetaDir::delNode(ShinyMetaNode *delNode) {
    for( uint64_t i=0; i<this->nodes.size(); ++i ) {
        if( this->nodes[i] == delNode ) {
            this->nodes.erase( this->nodes.begin() + i );
            return;
        }
    }
}

const std::vector<ShinyMetaNode *> * ShinyMetaDir::getNodes() {
    return &nodes;
}

uint64_t ShinyMetaDir::getNumNodes( void ) {
    return nodes.size();
}

bool ShinyMetaDir::check_childrenHaveUsAsParent( void ) {
    bool retVal = true;
    //Iterate through all nodes
    for( uint64_t i = 0; i<this->nodes.size(); ++i ) {
        if( this->nodes[i]->getParent() != this ) {
            // Temp variable to make it a little easier to read this code
            const char * parentStr = this->nodes[i]->getParent() ? this->nodes[i]->getParent()->getName() : "<null>";
            WARN( "Child %s/%s had %s as its parent!", this->getName(), this->nodes[i]->getName(), parentStr );
            retVal = false;
        }
    }
    return retVal;
}

bool ShinyMetaDir::sanityCheck() {
    //First, do the basic stuff
    bool retVal = ShinyMetaNode::sanityCheck();
    
    //Check no dups in children
    retVal &= check_noDuplicates( &this->nodes, "nodes" );
    
    //Finally, check all nodes point to us
    retVal &= check_childrenHaveUsAsParent();
    
    return retVal;
}

size_t ShinyMetaDir::serializedLen( void ) {
    /*
    //base amount
    size_t len = ShinyMetaNode::serializedLen();
    
    //plus the size of the number of nodes
    len += sizeof(size_t);
    
    //Plus the nodes themselves
    len += sizeof(inode_t)*this->nodes.size();
    
    //retuuuuuuuuuurn
    return len;
     */
    return 0;
}

void ShinyMetaDir::serialize( char * output ) {
    // First, resize the vector to exactly the size it should be to compact memory as much as possible
    // If already at optimal capacity, this function does nothing. Note that this is not necessary when
    // serializing, but since we do this every so often anyway (to write out to disk and such) it saves memory
    this->nodes.reserve( this->nodes.size() );
    
    
    /*
    //First, the normal stuffage
    ShinyMetaNode::serialize(output);
    output += ShinyMetaNode::serializedLen();
    
    //Now, save out the list.  First, the number of inodes contained
    *((size_t *)output) = nodes.size();
    output += sizeof(size_t);
    
    //Next, we pound out all the inodes contained in nodes
    inode_t * inode_output = (inode_t *)output;
    for( std::list<inode_t>::iterator itty = nodes.begin(); itty != nodes.end(); ++itty ) {
        *inode_output = *itty;
        inode_output++;
    }
     */
}

void ShinyMetaDir::unserialize(const char *input) {
    /*
    //Next, the number of nodes to read in
    size_t numNodes = *((size_t *)input);
    input += sizeof(size_t);
    
    //Finally, read in all the nodes
    for( uint64_t i = 0; i<numNodes; ++i ) {
        inode_t newNode = *((inode_t *)input);
        input += sizeof(inode_t);
        
        nodes.push_back( newNode );
    }
     */
}

ShinyMetaNode::NodeType ShinyMetaDir::getNodeType( void ) {
    return ShinyMetaNode::TYPE_DIR;
}