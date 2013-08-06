#pragma once
#include <stdint.h>
#include "Config.h"

#ifndef SHINY_DB
  #error "No database backend selected!"
#endif

#if SHINY_DB == DB_KYOTO
  #include <kcpolydb.h>
#elif SHINY_DB == DB_LEVEL
  #include <leveldb/db.h>
#else
  #error "Invalid database backend selected!"
#endif

namespace ShinyFS {
    // This class wraps our varying database backends; providing a simple keyed blob store abstraction
    class DBWrapper {
    public:
        DBWrapper( const char * path );
        ~DBWrapper();
        
        // Assumes key is zero-terminated
        uint64_t get( const char * key, char * buffer, uint64_t maxsize );
        uint64_t put( const char * key, const char * buffer, uint64_t size );
        bool del( const char * key );
        
        // Returns the last error that occured
        const char * getError();
    private:
    #if SHINY_DB ==  DB_KYOTO
        kyotocabinet::PolyDB db;
    #endif
        
    #if SHINY_DB == DB_LEVEL
        leveldb::DB * db;
        leveldb::Status status;
    #endif
    };
};