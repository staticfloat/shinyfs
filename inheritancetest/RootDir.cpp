#include "Nodes.h"

using namespace ShinyFS;

RootDir::RootDir( const char * filecache_path ) : RootDirSnapshot(filecache_path), Dir("", NULL), DirSnapshot( "", NULL ), NodeSnapshot( "", NULL ), Node( "", NULL ) {
}

RootDir::~RootDir() {
}