//
//  ShinyDBWrapper.cpp
//  shinyfs
//
//  Created by Elliot Saba on 4/26/12.
//  Copyright (c) 2012 The Network Marines. All rights reserved.
//

#include "ShinyDBWrapper.h"
#include <base/Logger.h>

#define min( x, y ) ((x) > (y)? (y) : (x))

ShinyDBWrapper::ShinyDBWrapper( const char * path ) {
#ifdef KYOTOCABINET
    if( !this->db.open( path, kyotocabinet::PolyDB::OWRITER | kyotocabinet::PolyDB::OCREATE ) ) {
        ERROR( "Unable to open filecache in %s", filecache );
        throw "Unable to open filecache";
    }
#endif
#ifdef LEVELDB
    leveldb::Options options;
    options.create_if_missing = true;
    this->status = leveldb::DB::Open( options, path, &this->db );
    if( !this->status.ok() ) {
        ERROR( "Unable to open filecache in %s", path );
        throw "Unable to open filecache";
    }
#endif
}

ShinyDBWrapper::~ShinyDBWrapper() {
#ifdef KYOTOCABINET
    this->db.close();
#endif
#ifdef LEVELDB
    delete db;
#endif
}

uint64_t ShinyDBWrapper::get(const char *key, char *buffer, uint64_t maxsize) {
#ifdef KYOTOCABINET
    return this->db.get( key, strlen(key), buffer, maxsize );
#endif
#ifdef LEVELDB
    std::string stupidDBBuffer;
    status = this->db->Get( leveldb::ReadOptions(), key, &stupidDBBuffer );
    if( status.ok() ) {
        uint64_t len = min(maxsize, stupidDBBuffer.length());
        memcpy( buffer, stupidDBBuffer.c_str(), len );
        return len;
    }
    return -1;
#endif
}

uint64_t ShinyDBWrapper::put(const char *key, const char *buffer, uint64_t size) {
#ifdef KYOTOCABINET
    return this->db.set( key, strlen(key), buffer, maxsize );
#endif
#ifdef LEVELDB
    leveldb::Slice buffSlice( buffer, size );
    status = this->db->Put( leveldb::WriteOptions(), key, buffSlice );
    return status.ok() ? size : -1;
#endif
}

bool ShinyDBWrapper::del(const char *key) {
#ifdef KYOTOCABINET
    return db.remove( key, strlen(key) );
#endif
#ifdef LEVELDB
    status = db->Delete( leveldb::WriteOptions(), key );
    return status.ok();
#endif
}

const char * ShinyDBWrapper::getError() {
#ifdef KYOTOCABINET
    return this->db.error().name();
#endif
#ifdef LEVELDB
    return this->status.ToString().c_str();
#endif
}