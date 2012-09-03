#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main( int argc, char ** argv ) {
    int fd = open( "asd", O_WRONLY | O_CREAT | O_EXLOCK, 0644 );
    if( fd == -1 ) {
        printf( "ERROR: Cannot open asd: %s\n", strerror(errno));
        return -1;
    }
    
    write(fd, argv[1], strlen(argv[1]));
    printf("first test written...\n");
    getchar();
    write(fd, "\nDone!\n", 7);
    printf( "second test written\n");
    return 0;
}