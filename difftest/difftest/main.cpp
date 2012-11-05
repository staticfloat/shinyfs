#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DATALEN1     1024*1000
#define DATALEN2     (DATALEN1 + 3)

#define min( x, y )  ((x) < (y) ? (x) : (y))

typedef struct {
    uint64_t idx;
    uint64_t len;
} SearchResult;

SearchResult findLongestSubstring( void * a, uint64_t aLen, void * b, uint64_t bLen, uint64_t start ) {
    uint8_t * aPtr = ((uint8_t *)a) + start;
    uint8_t * bPtr;
    const uint64_t limit = min(aLen - start, bLen);
    
    SearchResult max = { 0, 0 };
    uint64_t j;
    for( uint64_t i = 0; i < bLen; ++i ) {
        bPtr = (uint8_t *)b + i;
        j = 0;
        while( aPtr[j] == bPtr[j] && j < limit ) {
            j++;
        }
        if( j > max.len ) {
            max.len = j;
            max.idx = i;
        }
    }
    return max;
}

// Calculates a out of as much b as possible
void doDiff( void * a, uint64_t aLen, void * b, uint64_t bLen ) {
    // Start marks the position in a that we're trying to match in b
    int64_t start = 0;
    while( start < aLen - 1 ) {
        SearchResult result = findLongestSubstring( a, aLen, b, bLen, start );
        if( result.len > 16 ) {
            printf("Mapped old %lld-%lld -> new %lld-%lld\n", result.idx, result.idx + result.len, start, start + result.len );
            start += result.len;
        } else
            start++;
    }
}


int main(int argc, const char * argv[])
{
    uint8_t * data1 = new uint8_t[DATALEN1];
    for( int i = 0; i<DATALEN1; ++i ) {
        data1[i] = rand();
    }
    
    uint8_t * data2 = new uint8_t[DATALEN2];
    memcpy( data2, data1, 157 );
    
    data2[157] = 1;
    data2[158] = 1;
    data2[159] = 1;
    memcpy( data2 + 160, data1 + 157, DATALEN1 - 157 );
    
    
    
    doDiff( data1, DATALEN1, data2, DATALEN2 );
    
    delete( data1 );
    delete( data2 );
    return 0;
}

