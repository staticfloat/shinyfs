#include "ShinyCache.h"


ShinyCache::ShinyCache( const char * filecachePath, zmq::context_t * ctx ) : ctx( ctx ) {
    
}

const char * ShinyCache::getZMQEndpointShinyFS() {
    return "inproc://shinycache.shinyfs";
}