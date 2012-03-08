#include "ShinyFuse.h"
#include <base/Logger.h>
#include "filesystem/ShinyMetaDir.h"
#include "filesystem/ShinyMetaFile.h"
#include <sys/errno.h>

ShinyFilesystem * ShinyFuse::fs;
//ShinyFuse::workerStateStruct ShinyFuse::workerState;
//char * ShinyFuse::mountPoint;

bool ShinyFuse::init( ShinyFilesystem * fs, const char * mountPoint ) {
    //First, setup the callbacks
    struct fuse_operations shiny_operations;
    memset( &shiny_operations, NULL, sizeof(shiny_operations) );

    shiny_operations.init = ShinyFuse::fuse_init;
    shiny_operations.destroy = ShinyFuse::fuse_destroy;

    shiny_operations.getattr = ShinyFuse::fuse_getattr;
    shiny_operations.readdir = ShinyFuse::fuse_readdir;
    
    shiny_operations.open = ShinyFuse::fuse_open;
    shiny_operations.read = ShinyFuse::fuse_read;
    
    shiny_operations.write = ShinyFuse::fuse_write;
    shiny_operations.truncate = ShinyFuse::fuse_truncate;
    shiny_operations.create = ShinyFuse::fuse_create;
    shiny_operations.mknod = ShinyFuse::fuse_mknod;
    shiny_operations.mkdir = ShinyFuse::fuse_mkdir;
    
    shiny_operations.unlink = ShinyFuse::fuse_unlink;
    shiny_operations.rmdir = ShinyFuse::fuse_rmdir;
    
    shiny_operations.rename = ShinyFuse::fuse_rename;
    
    shiny_operations.access = ShinyFuse::fuse_access;
    shiny_operations.chmod = ShinyFuse::fuse_chmod;
    shiny_operations.chown = ShinyFuse::fuse_chown;
    
    //Save fs
    ShinyFuse::fs = fs;
    
//    ShinyMetaDir * root = (ShinyMetaDir *) fs->findNode( "/" );
    
    struct stat st;
    if( stat( mountPoint, &st ) != 0 ) {
        WARN( "Couldn't find mount point %s, attempting to create...", mountPoint );
        if( mkdir( mountPoint, S_IRWXU | S_IRWXG | S_IROTH ) != 0 ) {
            ERROR( "Could not create mount point %s!", mountPoint );
            return false;
        }
    }
    
    try {
        char * argv[] = {
            (char *)"./shinyfs",
            (char *)mountPoint,
            (char *)"-f",
        };
        fuse_main( sizeof(argv)/sizeof(char *), argv, &shiny_operations, NULL );
    } catch( ... ) {
        ERROR( "fuse_main() died!" );
    }
    return true;
}

void ShinyFuse::destroy( void ) {
}

/*
void * ShinyFuse::fuseWorkerThread( void *data ) {
    LOG( "starting fuse worker thread on thread id [%llu]", ShinyFuse::workerState.threadId );
    try {
        char * argv[] = {
            "./shinyfs",
            ShinyFuse::mountPoint,
//            "-d",
            "-f",
        };
        fuse_main( sizeof(argv)/sizeof(char *), argv, (fuse_operations *)data, NULL );
    } catch( ... ) {
        ERROR( "fuse_main() died!" );
    }
    delete( (char *)data );
    return NULL;
}*/


void * ShinyFuse::fuse_init( struct fuse_conn_info * conn ) {
    LOG( "init" );
    return NULL;
}

void ShinyFuse::fuse_destroy( void * private_data ) {
    LOG( "destroy" );
    fs->flush();
    fs->sanityCheck();
    LOG( "flushed and sanitycheck'ed!" );
}

int ShinyFuse::fuse_getattr( const char *path, struct stat * stbuff ) {
    LOG( "getattr: [%s]", path );
    
    memset( stbuff, NULL, sizeof(struct stat) );
    
    ShinyMetaNode * node = fs->findNode( path );
    if( node ) {
        stbuff->st_mode = node->getPermissions();
        switch( node->getNodeType() ) {
            case SHINY_NODE_TYPE_DIR:
            case SHINY_NODE_TYPE_ROOTDIR:
                stbuff->st_mode |= S_IFDIR;
                stbuff->st_nlink = ((ShinyMetaDir *)node)->getNumChildren();
                break;
            case SHINY_NODE_TYPE_FILE:
                stbuff->st_mode |= S_IFREG;
                stbuff->st_nlink = 1;
                
                stbuff->st_size = ((ShinyMetaFile *)node)->getFileLen();
                break;
            default:
                WARN( "Couldn't find the node type of node %s!", path );
                break;
        }
    } else {
        WARN( "Couldn't find the node for [%s]!", path );
        return -ENOENT;
    }
    return 0;
}

int ShinyFuse::fuse_readdir(const char *path, void *output, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
    LOG( "readdir [%s]", path );
    filler( output, ".", NULL, 0 );
    filler( output, "..", NULL, 0 );
  
    ShinyMetaDir * node = (ShinyMetaDir *) fs->findNode( path );
    if( node && (node->getNodeType() == SHINY_NODE_TYPE_DIR || node->getNodeType() == SHINY_NODE_TYPE_ROOTDIR) ) {
        const std::list<inode_t> * children = node->getListing();
        for( std::list<inode_t>::const_iterator itty = children->begin(); itty != children->end(); ++itty ) {
            ShinyMetaNode * child = fs->findNode( *itty );
            if( child )
                filler( output, child->getName(), NULL, 0 );
            else
                WARN( "Dir %s has a non-existent child [%llu]", node->getPath(), *itty );
        }
    } else {
        WARN( "Could not find dir %s", path );
        return -ENOENT;
    }
    return 0;
}

int ShinyFuse::fuse_open( const char *path, struct fuse_file_info *fi ) {
    LOG( "open [%s]", path );
    
    ShinyMetaNode * node = fs->findNode(path);
    if( node ) {
        LOG( "Check for permissions issues!" );
        //Check if we're asking for any kind of write access
        if( fi->flags & (O_WRONLY | O_RDWR) ) {
            LOG( "Here's where I ask for a write-lock" );
            return 0;
        } else {
            //It has to be either O_WRONLY, O_RDWR or O_RDONLY
            return 0;
        }
    } else
        return -ENOENT;
}

int ShinyFuse::fuse_read(const char *path, char *buffer, size_t len, off_t offset, struct fuse_file_info * fi ) {
    LOG( "read [%s]", path );
    
    ShinyMetaFile * node = (ShinyMetaFile *) fs->findNode(path);
    if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
        return (int)node->read( offset, buffer, len );
    } else 
        return -ENOENT;
}

int ShinyFuse::fuse_write( const char * path, const char * buffer, size_t len, off_t offset, struct fuse_file_info * fi ) {
    LOG( "write [%s]", path );

    ShinyMetaFile * node = (ShinyMetaFile *) fs->findNode(path);
    if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
        return (int)node->write( offset, buffer, len );
    } else 
        return -ENOENT;
}

int ShinyFuse::fuse_truncate(const char * path, off_t len ) {
    LOG( "truncate [%s]", path );
    
    ShinyMetaFile * node = (ShinyMetaFile *) fs->findNode(path);
    if( node && node->getNodeType() == SHINY_NODE_TYPE_FILE ) {
        return (int)node->truncate( len );
    } else 
        return -ENOENT;
}

int ShinyFuse::fuse_create( const char * path, mode_t permissions, struct fuse_file_info * fi ) {
    LOG( "create [%s]", path );
    if( fs->findNode( path ) )
        return -EEXIST;

    ShinyMetaDir * parent = fs->findParentNode( path );
    if( !parent )
        return -ENOENT;

    uint64_t start = strlen(path);
    while( path[start-1] != '/' )
        start--;
    new ShinyMetaFile( fs, path + start, parent );
    return 0;
}

int ShinyFuse::fuse_mknod( const char *path, mode_t permissions, dev_t device ) {
    LOG( "mknod [%s]", path );
    //Just sub out to create()
    return fuse_create( path, permissions, NULL );
}

int ShinyFuse::fuse_mkdir( const char * path, mode_t permissions ) {
    LOG( "mkdir [%s]", path );
    if( fs->findNode( path ) )
        return -EEXIST;
    
    ShinyMetaDir * parent = fs->findParentNode( path );
    if( !parent )
        return -ENOENT;
    
    uint32_t start = (uint32_t) strlen(path) - 1;
    while( path[start-1] != '/' )   
        start--;
    new ShinyMetaDir( fs, path + start, parent );
    return 0;
}

int ShinyFuse::fuse_unlink( const char * path ) {
    LOG( "unlink [%s]", path );
    ShinyMetaNode * node = fs->findNode( path );
    if( node ) {
        delete( node );
    } else
        return -ENOENT;
    return 0;
}

int ShinyFuse::fuse_rmdir( const char * path ) {
    LOG( "rmdir [%s]", path );
    return fuse_unlink( path );
}

int ShinyFuse::fuse_rename( const char * path, const char * newPath ) {
    LOG( "rename [%s -> %s]", path, newPath );
    ShinyMetaNode * node = fs->findNode( path );
    //Return 404 if it doesn't exist
    if( !node )
        return -ENOENT;

    //Return error if the target already exists
    if( fs->findNode( newPath ) )
        return -EEXIST;

    //Return error if we can't find the dir we're supposed to move to
    ShinyMetaDir * newParent = fs->findParentNode( newPath );
    if( !newParent )
        return -ENOENT;
    
    ShinyMetaDir * parent = (ShinyMetaDir *) fs->findNode( node->getParent() );
    if( !parent ) {
        WARN( "Couldn't find parent of node %s with inode [%llu]", path, node->getParent() );
        return -EIO;
    }
    
    //Move the node over, call it all good
    if( parent != newParent ) {
        parent->delNode( node );
        newParent->addNode( node );
    }
    
    //Change the name
    uint32_t start = (uint32_t) strlen(newPath) - 1;
    while( newPath[start-1] != '/' )
        start--;
    node->setName( newPath + start );

    return 0;
}

int ShinyFuse::fuse_access( const char *path, int mode ) {
    LOG( "access [%s]", path );
    
    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;
    
    ERROR( "Need to solidify user-control stuffage here!" );
    return 0;
}

int ShinyFuse::fuse_chmod( const char *path, mode_t mode ) {
    LOG( "chmod [%s]", path );

    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;

    node->setPermissions( mode );
    return 0;
}

int ShinyFuse::fuse_chown( const char *path, uid_t uid, gid_t gid ) {
    LOG( "chown [%s]", path );

    ShinyMetaNode * node = fs->findNode( path );
    if( !node )
        return -ENOENT;

    ERROR( "Need to finish thinking about user management!!!!" );
    return 0;
}