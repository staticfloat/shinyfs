#include "Nodes.h"
#include <stdlib.h>

using namespace ShinyFS;

Dir::Dir( const char * newName, Dir * newParent ) : NodeSnapshot( newName, newParent), Node(newName, newParent), DirSnapshot( newName, newParent ) {
}

Dir::Dir( const uint8_t ** serializedStream, Dir * newParent ) : NodeSnapshot( serializedStream, newParent ), Node( serializedStream, newParent ), DirSnapshot( serializedStream, newParent, true ) {
    update( serializedStream, true );
}

Dir::~Dir() {
    //LOG("~Dir: %s", this->getPath() );
}

bool Dir::addNode( Node * node ) {
    bool ret = DirSnapshot::addNode( node );
    if( ret )
        this->setMtime();
}

bool Dir::delNode( Node * node ) {
    bool ret = DirSnapshot::delNode( node );
    if( ret )
        this->setMtime();
}

void Dir::update( const uint8_t ** input_ptr, bool skipSelf ) {
    if( !skipSelf ) {
        // Have Node do its unserialization
        Node::update( input_ptr );
    }

    // Get the number of children in this dir
    uint64_t numNodes = *((uint64_t *)*input_ptr);
    *input_ptr += sizeof(uint64_t);

    // If so, let's raise all those nodes up!
    this->nodes.reserve( numNodes );
    for( uint64_t i = 0; i<numNodes; ++i ) {
        Node::unserialize( input_ptr, this );
    }
}

void const Dir::getNodes( std::vector<Node *> * nodes ) {
    for( std::vector<NodeSnapshot *>::iterator itty = this->nodes.begin(); itty != this->nodes.end(); ++itty ) {
        nodes->push_back( dynamic_cast<Node *>( *itty ) );
    }
}

Node * const Dir::findNode( const char * name ) {
    return dynamic_cast<Node *>(((DirSnapshot *)this)->findNode(name));
}
