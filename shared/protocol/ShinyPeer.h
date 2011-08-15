//
//  ShinyPeer.h
//  shinyfs-node
//
//  Created by Elliot Saba on 7/26/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef shinyfs_node_ShinyPeer_h
#define shinyfs_node_ShinyPeer_h

class ShinyPeer {
public:
    ShinyPeer();
    ShinyPeer( const char * name );
    ~ShinyPeer();
    
    
    const char * getIP( void );
private:
    char * IP;
};


#endif
