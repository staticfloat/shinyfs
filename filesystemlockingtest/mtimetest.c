#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main( int argc, char ** argv ) {
    if( argc < 2 ) {
        printf( "Usage: %s \"stuff to write\"\n", argv[0] );
        return -1;
    }

    int fd = open( "asd", O_WRONLY | O_CREAT, 0644 );
    if( fd == -1 ) {
        printf( "ERROR: Cannot open asd: %s\n", strerror(errno));
        return -1;
    }
    
    write(fd, argv[1], strlen(argv[1]));
    close(fd);
    return 0;
}
