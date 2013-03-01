#include "ShinyMetaFileHandle.h"
#include "ShinyFilesystem.h"
#include "base/Logger.h"

ShinyMetaFileHandle::ShinyMetaFileHandle( const char ** serializedInput, ShinyFilesystem * fs, const char * path ) : ShinyMetaFile( serializedInput, NULL ) {
    // Store away fs and path
    this->fs = fs;
    this->path = path;
    
    this->unserialize( serializedInput );
}

ShinyMetaFileHandle::~ShinyMetaFileHandle() {
}

uint64_t ShinyMetaFileHandle::read( uint64_t offset, char *data, uint64_t len ) {
    return this->ShinyMetaFile::read( this->fs->getDB(), this->path, offset, data, len );
}

uint64_t ShinyMetaFileHandle::write( uint64_t offset, const char *data, uint64_t len ) {
    return this->ShinyMetaFile::write( this->fs->getDB(), this->path, offset, data, len );
}

void ShinyMetaFileHandle::setLen( uint64_t newLen ) {
    this->ShinyMetaFile::setLen( this->fs->getDB(), this->path, newLen );
}

ShinyMetaNode::NodeType ShinyMetaFileHandle::getNodeType() {
    return ShinyMetaNode::TYPE_FILEHANDLE;
}