#include "Nodes.h"

using namespace ShinyFS;

DirSnapshot::DirSnapshot(const char * newName, DirSnapshot * const newParent) : NodeSnapshot( newName, newParent ) {
}

DirSnapshot::DirSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent ) : NodeSnapshot( serializedStream, newParent ) {
    // Because vtable's don't update until construction is done, we need to call update manually in each constructor
    update( serializedStream, true );
}

DirSnapshot::DirSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent, bool onlyToBeUsedByDir ) : NodeSnapshot( serializedStream, newParent ) {
    // This method only to be invoked by Dir( serializedStream, newParent )
}

DirSnapshot::~DirSnapshot() {
    // Delete all nodes I contain.  When they are deleted, they will remove themselves from this DirSnapshot
    while( !nodes.empty() )
        delete nodes.front();
}

uint64_t DirSnapshot::serializedLen() {
    // Start with the length given by NodeSnapshot
    uint64_t len = NodeSnapshot::serializedLen();

    // Add in the length of the number of children we write out
    len += sizeof(uint64_t);

    // Count up the lengths of all our children:
    for( uint64_t i=0; i<nodes.size(); ++i )
        len += nodes[i]->serializedLen();

    return len;
}

void DirSnapshot::serialize( uint8_t ** output_ptr ) {
    uint8_t * start = *output_ptr;
    // First things first, serialize out the NodeSnapshot parts of ourselves
    NodeSnapshot::serialize(output_ptr);

    // Second things second, write out the number of children we have
    *((uint64_t *)*output_ptr) = this->nodes.size();
    *output_ptr += sizeof(uint64_t);

    // Third things third, serialize out our childrens
    for( uint64_t i=0; i<nodes.size(); ++i )
        nodes[i]->serialize( output_ptr );
}

void DirSnapshot::update( const uint8_t ** input_ptr, bool skipSelf ) {
    if( !skipSelf ) {
        // Have NodeSnapshot do its unserialization
        NodeSnapshot::update( input_ptr );
    }

    // Get the number of children in this dir
    uint64_t numNodes = *((uint64_t *)*input_ptr);
    *input_ptr += sizeof(uint64_t);

    // If so, let's raise all those nodes up!
    this->nodes.reserve( numNodes );
    for( uint64_t i = 0; i<numNodes; ++i ) {
        NodeSnapshot::unserialize( input_ptr, this );
    }
}


void const DirSnapshot::getNodes( std::vector<NodeSnapshot *> * nodes ) {
    for( std::vector<NodeSnapshot *>::iterator itty = this->nodes.begin(); itty != this->nodes.end(); ++itty ) {
        nodes->push_back( *itty );
    }
}

const std::vector<NodeSnapshot *> * const DirSnapshot::getNodesView( void ) {
    return &nodes;
}

// Returns the sign of a numeric type
template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

#define min(x,y) ((x)>(y)?(y):(x))

// Searches for where to insert a string.  If the string already exists in the vector,
// returns the index of the string.  If it does not exist, returns the location it should occupy
uint64_t binaryStringSearch( std::vector<NodeSnapshot *> &nodes, const char * name, uint64_t nameLen ) {
    int64_t idxMin = 0;
    int64_t idxMax = nodes.size();
    while( idxMax > idxMin ) {
        // Calculate midpoint between [min, max]
        uint64_t idxMid = idxMin + (idxMax - idxMin) / 2;

        uint64_t testNameLen = strlen(nodes[idxMid]->getName());
        // Check if this string is "greater" or "lesser"
        switch( sgn(memcmp( nodes[idxMid]->getName(), name, min( testNameLen, nameLen) )) ) {
            case -1:
                // If nodes[idxMid]->name is LESS than name, traverse forward through the list
                idxMin = idxMid + 1;
                break;
            case 1:
                // If nodes[idxMid]->name is GREATER than name, back it up a bit
                idxMax = idxMid - 1;
                break;
            case 0:
                // If everything matches up to the maximum lengths, we must compare lengths
                switch( sgn((int64_t)(testNameLen - nameLen)) ) {
                    case -1:
                        idxMin = idxMid + 1;
                        break;
                    case 1:
                        idxMax = idxMid - 1;
                        break;
                    case 0:
                        return idxMid;
                }
        }
    }
    return idxMin;
}

bool DirSnapshot::addNode( NodeSnapshot *newNode ) {
    // Don't forget to reject NULL.  :P
    if( !newNode )
        return false;

    if( newNode->getParent() != this ) {
        ERROR( "Cannot add a node whose parent is not us!" );
        return false;
    }

    // Insert so that list is always sorted in ascending order of name
    int64_t idx = binaryStringSearch( nodes, newNode->getName(), strlen(newNode->getName()) );
    if( idx < nodes.size() && strcmp(this->nodes[idx]->getName(), newNode->getName()) == 0 ) {
        WARN( "File %s already exists in %s! Duplicates are not allowed!", newNode->getName(), this->getName() );
        return false;
    }

    nodes.insert(nodes.begin() + idx, newNode );
    return true;
}

bool DirSnapshot::delNode(NodeSnapshot *delNode) {
    // No go if nothing here (Also avoids crash, so that's good!)
    if( nodes.size() == 0 )
        return false;

    uint64_t idx = binaryStringSearch( nodes, delNode->getName(), strlen(delNode->getName()) );
    if( nodes[idx] == delNode ) {
        this->nodes.erase( this->nodes.begin() + idx );
        return true;
    }
    return false;
}


NodeSnapshot * DirSnapshot::findNode( const char *name ) {
    // Strip off initial '/'
    if( name[0] == '/' )
        name++;

    // If there's actually nothing, return ourselves
    if( name[0] == 0 )
        return this;

    // Find the length of the name of the node at the top level
    uint64_t i=0;
    while( i < strlen(name) && name[i] != '/' ){
        ++i;
    }

    // Find the closest node to this name
    uint64_t idx = binaryStringSearch( nodes, name, i );

    // Did we find it exactly?
    if( strncmp( nodes[idx]->getName(), name, i ) >= 0 ) {
        // If so, and this name ends with a slash, it's a directory so we should continue on
        if( name[i] == '/' ) {
            if( nodes[idx]->getNodeType() == TYPE_DIR )
                return dynamic_cast<DirSnapshot *>(nodes[idx])->findNode( name + i );
        }
        else
            return nodes[idx];
    }

    // If not, tough luck for us, return NULL!
    return NULL;
}

uint64_t DirSnapshot::getNumNodes( void ) {
    return nodes.size();
}

const NodeType DirSnapshot::getNodeType( void ) {
    return TYPE_DIR;
}

uint16_t DirSnapshot::getDefaultPermissions( void ) {
    // For a directory, give it executable permissions!
    return NodeSnapshot::getDefaultPermissions() | S_IXUSR | S_IXGRP | S_IXOTH;
}
