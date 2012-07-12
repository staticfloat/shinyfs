#ifndef shinyfs_ShinyDBWrapper_h
#define shinyfs_ShinyDBWrapper_h

#ifdef KYOTOCABINET
#include <kcpolydb.h>
#else
#include <leveldb/db.h>
#endif

class ShinyDBWrapper {
public:
    ShinyDBWrapper( const char * path );
    ~ShinyDBWrapper();
    
    // Assumes key is zero-terminated
    uint64_t get( const char * key, char * buffer, uint64_t maxsize );
    uint64_t put( const char * key, const char * buffer, uint64_t size );
    bool del( const char * key );
    
    // Returns the last error that occured
    const char * getError();
private:
#ifdef KYOTOCABINET
    kyotocabinet::PolyDB db;
#else
    leveldb::DB * db;
    
    leveldb::Status status;
#endif // KYOTOCABINET
};


#endif // shinyfs_ShinyDBWrapper_h