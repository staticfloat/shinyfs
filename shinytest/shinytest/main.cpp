//
//  main.cpp
//  shinytest
//
//  Created by Elliot Saba on 12/25/12.
//  Copyright (c) 2012 Elliot Saba. All rights reserved.
//

#include "../../shinyfs/src/filesystem/ShinyMetaNode.h"
#include "../../shinyfs/src/filesystem/ShinyMetaRootDir.h"
#include "../../shinyfs/src/filesystem/ShinyMetaFile.h"
#include "../../shinyfs/src/filesystem/ShinyMetaDir.h"
#include "../../shinyfs/src/filesystem/ShinyFilesystem.h"


int main(int argc, const char * argv[])
{
    ShinyFilesystem * fs = new ShinyFilesystem( "/tmp/shinytest.fscache" );
    ShinyMetaRootDir * root = dynamic_cast<ShinyMetaRootDir *>(fs->findNode("/"));
    ShinyMetaFile * newFile = new ShinyMetaFile( "test", root );
    const char * msg = "test one two, yo!\n";
    newFile->write(0, msg, strlen(msg) );
    return 0;
}