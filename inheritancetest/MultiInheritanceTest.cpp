#include "Nodes.h"
#include <stdio.h>
#include <vector>
#include <unistd.h>

using namespace ShinyFS;

#define tabprintf(x...) {for( int tabNum=0;tabNum<(tabDepth-1);tabNum++ ) printf("     "); if(tabDepth) printf("|--->"); } printf(x)
void printNodes( NodeSnapshot * node, int tabDepth = 0 ) {
    //printf("[0x%x]", node );
    tabprintf( "%s: (%llu)", strlen(node->getName()) ? node->getName() : "/", node->getMtime().ns );

    switch( node->getNodeType() ) {
        case TYPE_DIR: {
            printf("\n");
            DirSnapshot * dir = dynamic_cast<DirSnapshot *>(node);

            std::vector<NodeSnapshot *> nodes;
            dir->getNodes( &nodes );
            for( uint64_t i=0; i<nodes.size(); ++i ) {
                printNodes( nodes[i], tabDepth + 1 );
            }
        }
            break;
        case TYPE_FILE:
            printf(" - [%llu]\n", dynamic_cast<FileSnapshot *>(node)->getLen() );
            break;
        default:
            WARN("Unknown node type (%d)", node->getNodeType() );
            break;
    }
}

RootDir * createTestData( void ) {
    RootDir * r = new RootDir(".filecache/test_filecache");
    Dir * d = new Dir("dir1", r);
    File * f = new File( "daFile", d);
    File * f2 = new File( "daFile2", d);
    f->append_str("This is a test for daFile");

    Dir * d2 = new Dir("dir2", d);
    Dir * d3 = new Dir("dir3", d2);
    Dir * d4 = new Dir("dir4", d3);
    Dir * d5 = new Dir("dir5", d4);
    Dir * d6 = new Dir("dir6", d5);
    Dir * d7 = new Dir("dir7", d6);
    File * nested = new File("nested", d7);
    nested->setLen(20);

    File * f3 = new File("zfaugh", d);
    File * f4 = new File("zfaugh", d2);
    Dir * d44 = new Dir("dir4", d4 );
    File * mostConfusingFileNA = new File("dir4", d44);

    return r;
}

// Makes sure we can find a file in this tree
bool test_findFile( Dir * d, const char * path, bool printWarnings = true ) {
    Node * n = d->findNode( path );
    if( !n ) {
        if( printWarnings )
            WARN( "Couldn't find  %s!", path );
        return false;
    }

    // Find last / in path (e.g. find basename())
    const char * basename = strrchr( path, '/' ) + 1;
    if( !basename )
         basename = path;

    if( strcmp( n->getName(), basename ) != 0 ) {
        ERROR( "Names didn't match!  %s != %s", n->getName(), basename );
        return false;
    }
    return true;
}

bool compare_nodes( Node * a, Node * b ) {
    //LOG("Comparing %s <-> %s", a->getPath(), b->getPath() );

    if( a->getNodeType() != b->getNodeType() ) {
        ERROR("%s != %s (type)", a->getPath(), b->getPath() );
        return false;
    }

    if( strcmp(a->getName(), b->getName()) != 0 ) {
        ERROR("%s != %s (name)", a->getPath(), b->getPath() );
        return false;
    }

    if( a->getBtime() != b->getBtime() ||
        a->getCtime() != b->getCtime() ||
        a->getMtime() != b->getMtime() ||
        a->getAtime() != b->getAtime() ) {
        ERROR("%s != %s (time)", a->getPath(), b->getPath() );
        return false;
    }

    if( a->getUID() != b->getUID() ||
        a->getGID() != b->getGID() ) {
        ERROR("%s != %s (GID/UID)", a->getPath(), b->getPath() );
        return false;
    }


    if( a->getNodeType() == TYPE_FILE ) {
        File * af = dynamic_cast<File *>(a);
        File * bf = dynamic_cast<File *>(b);
        if( af->getLen() != bf->getLen() ) {
            ERROR("%s != %s (len)", a->getPath(), b->getPath() );
            return false;
        }

        TODO("NABIL: Eventually fill this in with file CONTENTS comparison!");
    }

    if( a->getNodeType() == TYPE_DIR ) {
        Dir * ad = dynamic_cast<Dir *>(a);
        Dir * bd = dynamic_cast<Dir *>(b);
        if( ad->getNumNodes() != bd->getNumNodes() ) {
            ERROR("%s != %s (numnodes)", a->getPath(), b->getPath() );
            return false;
        }

        std::vector<Node *> an;
        std::vector<Node *> bn;

        ad->getNodes( &an );
        bd->getNodes( &bn );

        for( uint64_t i=0; i<an.size(); ++i ) {
            if( !compare_nodes( an[i], bn[i] ) ) {
                ERROR("%s != %s (children)", a->getPath(), b->getPath() );
                return false;
            }
        }
    }
    return true;
}

// Serializes a subtree, removes it from the dir, then adds it back in again
bool test_serializeSubtree( Dir * d, const char * subtree ) {
    Node * n = d->findNode( subtree );
    if( !n ) {
        ERROR( "Could not find %s!", subtree );
        return false;
    }

    Dir * parent = n->getParent();
    if( !parent ) {
        ERROR( "Could not get parent of %s!", subtree );
        return false;
    }

    uint64_t sLen = n->serializedLen();
    uint8_t * serialized = new uint8_t[sLen];

    // Copy over a second pointer as n->serialize() is going to clobber this pointer
    n->serialize( &serialized );
    serialized -= sLen; // Un-clobber

    // Reconstitute the tree under a fake root, and compare:
    RootDir * fakeroot = new RootDir( ".filecache/fakeroot" );

    Node * newN = Node::unserialize( (const uint8_t **) &serialized, fakeroot );
    delete[] (serialized - sLen);
    if( !n ) {
        ERROR( "Could not unserialize!" );
        return false;
    }

    // Compare subtrees
    if( !compare_nodes( n, newN ) ) {
        ERROR( "Failure during compare_nodes!  (Un)Serialization process is flawed!");
        delete fakeroot;
        return false;
    }

    // Clean up, yo!
    delete fakeroot;
    return true;
}

bool test_fileReadWrite( Dir * r ) {
    File * f = dynamic_cast<File *>(r->findNode( "/dir1/daFile" ));

    if( !f ) {
        ERROR( "Could not find /dir1/daFile!" );
        return false;
    }

    if( !f->getLen() ) {
        ERROR( "/dir1/daFile has no data!");
        return false;
    }

    char * inData = new char[f->getLen()];
    f->read( 0, inData, f->getLen() );

    printf( "%s\n", inData );
    return true;
}


int main( void ) {
    Logger::getGlobalLogger()->setPrintId(false);
    Logger::getGlobalLogger()->setPrintThread(false);
    Logger::getGlobalLogger()->setPrintPrettyName(false);
    Logger::getGlobalLogger()->setPrintName(true);

    // Create the test data
    Dir * r = createTestData();

    LOG("data created, beginning tests");

    // Print out nodes for first, initial state
    printNodes( r );

    // Do a simple query to make sure the tree was created properly
    if( !test_findFile( r, "/dir1/daFile2" ) ) {
        ERROR( "test_findFile( \"/dir1/daFile2\") failed!" );
        return -1;
    }

    if( test_findFile( r, "/dir1/daFile22", false) ) {
        ERROR( "test_findFile( \"/dir1/daFile22\") passed! (it shouldn't!)" );
        return -1;
    }

    if( !test_serializeSubtree(r, "/dir1/dir2" ) ) {
        ERROR( "test_serializeSubtree() failed!" );
        return -1;
    }

    if( !test_fileReadWrite( r ) ) {
        ERROR("test_fileReadWrite() failed!");
        return -1;
    }

    delete r;
    return 0;
}
