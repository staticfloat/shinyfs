#ifndef shinyfs_ShinyDBWrapper_h
#define shinyfs_ShinyDBWrapper_h

#ifdef KYOTOCABINET
// (kyoto cabinet this clobbers my ERROR/WARN macros, so I have to include it before Logger.h)
#include <kcpolydb.h>
#else
#include <leveldb/db.h>
#endif


class ShinyDBWrapper {
public:
    ShinyDBWrapper( const char * path );
    ~ShinyDBWrapper();
    
    // Assumes key is zero-terminated
    int get( const char * key, char * buffer, int maxsize );
    int put( const char * key, const char * buffer, int size );
    bool del( const char * key );
    
    const char * getError();
private:
#ifdef KYOTOCABINET
    kyotocabinet::PolyDB db;
#else
    leveldb::DB * db;
    
    leveldb::Status status;
#endif
};


#endif