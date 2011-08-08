#include "ShinyPeer.h"
#include <stdlib.h>
#include <stdio.h>

ShinyPeer::ShinyPeer() {
    IP = new char[16];
    sprintf( IP, "%d.%d.%d.%d", rand()%255, rand()%255, rand()%255, rand()%255 );
}

ShinyPeer::~ShinyPeer() {
    delete( IP );
}

const char * ShinyPeer::getIP( void ) {
    return IP;
}