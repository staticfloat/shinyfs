#include "Nodes.h"
#include <unistd.h> // for getuid/gid()
#include <time.h>

using namespace ShinyFS;

Node::Node( const char * newName, Dir * newParent ) : NodeSnapshot( newName, newParent ) {
}

Node::Node( const uint8_t ** serializedStream, Dir * const newParent ) : NodeSnapshot( serializedStream, newParent ) {
}

Node::~Node() {
}

Node * Node::unserialize( const uint8_t ** input, Dir * const parent ) {
    uint8_t type = (*input)[0];
    *input += sizeof(uint8_t);

    switch( type ) {
        case TYPE_DIR: {
            Dir * ret = new Dir( input, parent );
            //ret->update( input );
            return ret;
        }
        case TYPE_FILE:
            return new File( input, parent );
        case TYPE_DIRSTUMP:
            ERROR("Cannot create a Node from a DirStump, that can only ever be a snapshot!  (right now)");
        default:
            WARN( "Unknown node type %d\n", type );
    }
    return NULL;
}

Dir * Node::getParent() {
    return dynamic_cast<Dir *>(parent);
}

void Node::setParent( Dir * newParent ) {
    // Purge any cached node paths that we might have previously had
    RootDirSnapshot * root = this->getRoot();
    if( root )
        root->clearCachedNodePath( this );

    parent = dynamic_cast<DirSnapshot *>(newParent);
    this->setCtime();
}

void Node::setName( const char * newName ) {
    if( name )
        delete( name );

    uint64_t len = strlen(newName) + 1;
    name = new char[len];
    memcpy( name, newName, len );
    this->setCtime();
}

void Node::setPermissions( uint16_t newPermissions ) {
    permissions = (permissions & 0xfe00) | (newPermissions & 0x01ff);
    this->setCtime();
}

void Node::setUID( const uint64_t newUID ) {
    uid = newUID;
    this->setCtime();
}

void Node::setGID( const uint64_t newGID ) {
    gid = newGID;
    this->setCtime();
}

void Node::setAtime( void ) {
    this->setAtime( getCurrTime() );
}

void Node::setCtime( void ) {
    this->setCtime( getCurrTime() );
}

void Node::setMtime( void ) {
    this->setMtime( getCurrTime() );
}

void Node::setAtime( const TimeStruct new_atime ) {
    atime = new_atime;
}

void Node::setCtime( const TimeStruct new_ctime ) {
    ctime = new_ctime;
}

void Node::setMtime( const TimeStruct new_mtime ) {
    //LOG( "Setting the Mtime of %s to <%d, %d>", name, new_mtime.s, new_mtime.ns );
    mtime = new_mtime;
    ctime = new_mtime;
    //usleep(1);
}

