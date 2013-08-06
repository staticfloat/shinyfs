#include "Nodes.h"

using namespace ShinyFS;

DirStumpSnapshot::DirStumpSnapshot(const char * newName, DirSnapshot * const newParent) : NodeSnapshot( newName, newParent ) {
}

DirStumpSnapshot::DirStumpSnapshot( DirSnapshot * const clone ) : NodeSnapshot( clone ) {
    this->numNodes = clone->getNumNodes();
}

DirStumpSnapshot::DirStumpSnapshot( const uint8_t ** serializedStream, DirSnapshot * const newParent ) : NodeSnapshot( serializedStream, newParent ) {
    // Because vtable's don't update until construction is done, we need to call update manually in each constructor
    update( serializedStream, true );
}

DirStumpSnapshot::~DirStumpSnapshot() {
    
}

uint64_t DirStumpSnapshot::serializedLen() {
    // Start with the length given by NodeSnapshot
    uint64_t len = NodeSnapshot::serializedLen();

    // Add in the length of the number of children we write out
    len += sizeof(uint64_t);

    return len;
}

void DirStumpSnapshot::serialize( uint8_t ** output_ptr ) {
    // First things first, serialize out the NodeSnapshot parts of ourselves
    NodeSnapshot::serialize(output_ptr);

    // Second things second, write out the number of children we have
    *((uint64_t *)*output_ptr) = this->numNodes;
    *output_ptr += sizeof(uint64_t);
}

void DirStumpSnapshot::update( const uint8_t ** input_ptr, bool skipSelf ) {
    if( !skipSelf ) {
        // Have NodeSnapshot do its unserialization
        NodeSnapshot::update( input_ptr );
    }

    // Get the number of children in this dir
    this->numNodes = *((uint64_t *)*input_ptr);
    *input_ptr += sizeof(uint64_t);
}


const uint64_t DirStumpSnapshot::getNumNodes() {
    return this->numNodes;
}

const NodeType DirStumpSnapshot::getNodeType( void ) {
    return TYPE_DIRSTUMP;
}