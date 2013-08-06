#include "DBWrapper.h"
#include <base/Logger.h>

using namespace ShinyFS;

// If we're using leveldb, might as well import that namespace as well
#if SHINY_DB == DB_LEVEL
using namespace leveldb;
#endif

#define min( x, y ) ((x) > (y)? (y) : (x))

DBWrapper::DBWrapper( const char * path ) {
    if( !path || !path[0] ) {
        ERROR( "Invalid path \"%s\"!  Cannot open filecache!");
        throw "Invalid path";
    }

#if SHINY_DB == DB_KYOTO
    if( !this->db.open( path, kyotocabinet::PolyDB::OWRITER | kyotocabinet::PolyDB::OCREATE ) ) {
        ERROR( "Unable to open filecache in %s", path );
        throw "Unable to open filecache";
    }
#elif SHINY_DB == DB_LEVEL
    Options options;
    options.create_if_missing = true;
    this->status = DB::Open( options, path, &this->db );
    if( !this->status.ok() ) {
        ERROR( "Unable to open filecache in %s", path );
        throw "Unable to open filecache";
    }
#endif
}

DBWrapper::~DBWrapper() {
#if SHINY_DB == DB_KYOTO
    this->db.close();
#elif SHINY_DB == DB_LEVEL
    delete db;
#endif
}

uint64_t DBWrapper::get(const char *key, char *buffer, uint64_t maxsize) {
#if SHINY_DB == DB_KYOTO
    return this->db.get( key, strlen(key), buffer, maxsize );
#elif SHINY_DB == DB_LEVEL
    std::string stupidDBBuffer;
    status = this->db->Get( ReadOptions(), key, &stupidDBBuffer );
    if( status.ok() ) {
        uint64_t len = min(maxsize, stupidDBBuffer.length());
        memcpy( buffer, stupidDBBuffer.c_str(), len );
        return len;
    }
    return -1;
#endif
}

uint64_t DBWrapper::put(const char *key, const char *buffer, uint64_t size) {
#if SHINY_DB == DB_KYOTO
    return this->db.set( key, strlen(key), buffer, size );
#elif SHINY_DB == DB_LEVEL
    Slice buffSlice( buffer, size );
    status = this->db->Put( WriteOptions(), key, buffSlice );
    return status.ok() ? size : -1;
#endif
}

bool DBWrapper::del(const char *key) {
#if SHINY_DB == DB_KYOTO
    return db.remove( key, strlen(key) );
#elif SHINY_DB == DB_LEVEL
    status = db->Delete( WriteOptions(), key );
    return status.ok();
#endif
}

const char * DBWrapper::getError() {
#if SHINY_DB == DB_KYOTO
    return this->db.error().name();
#elif SHINY_DB == DB_LEVEL
    return this->status.ToString().c_str();
#endif
}