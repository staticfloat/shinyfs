#include "Nodes.h"
#include "DBWrapper.h"

using namespace ShinyFS;

RootDirSnapshot::RootDirSnapshot( const char * filecache_path ) : DirSnapshot( "", NULL ), NodeSnapshot( "", NULL ) {
	// Awww, we're our own parent!  How cute!
	parent = this;

	// Initialize db!
	db = new DBWrapper( filecache_path );

	// Store our own cached node path immediately.  Because why not. :P
	storeCachedNodePath( "", this );
}

RootDirSnapshot::~RootDirSnapshot() {
	// Delete all nodes I contain.  When they are deleted, they will remove themselves from this RootDirSnapshot
	// I am duplicating the work of DirSnapshot() here, because I need to do this before I forget that I am a RootDir!
    while( !nodes.empty() )
        delete nodes.front();

    // Clean up the DB
    delete db;

    // Now that we're about to forget that we're a RootDirSnapshot, we won't be able to getRoot() reliably anymore. :/
    parent = NULL;
}

RootDirSnapshot * const RootDirSnapshot::getRoot() {
    return this;
}

DBWrapper * const RootDirSnapshot::getDB() {
	return this->db;
}

const char * RootDirSnapshot::findCachedNodePath( NodeSnapshot * node ) {
	std::unordered_map<NodeSnapshot *, const char *>::iterator itty = cachedNodePaths.find( node );
	if( itty == cachedNodePaths.end() )
		return NULL;
	return (*itty).second;
}

void RootDirSnapshot::storeCachedNodePath( const char * path, NodeSnapshot * node ) {
	cachedNodePaths[node] = path;
}

void RootDirSnapshot::clearCachedNodePath( NodeSnapshot * node ) {
	std::unordered_map<NodeSnapshot *, const char *>::iterator itty = cachedNodePaths.find( node );
	if( itty == cachedNodePaths.end() )
		return;
	delete (*itty).second;
	cachedNodePaths.erase( itty );
}