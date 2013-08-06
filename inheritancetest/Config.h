#pragma once
#include <stdint.h>

// These are the two database backends we currently support
#define DB_KYOTO 1
#define DB_LEVEL 2

namespace ShinyFS {
	// this controls how large File "chunks" are: the granularity of data that is committed to/pulled from the DB
	static const uint64_t FILE_CHUNKSIZE = 64*1024;

};