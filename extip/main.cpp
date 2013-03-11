#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main()
{
	struct sockaddr_in sin;
	struct in_addr in;
	int sd, client_sd;
    unsigned int len;
    
	/* create a socket */
	sd = socket(AF_INET,SOCK_STREAM,0);
    
	/* let the OS use any unused port */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(6970);
    
	/* bind */
	if(bind(sd,(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("bind");
		exit(1);
	}
    
	/* listen */
	listen(sd,SOMAXCONN);
    
	while(1) {
		client_sd = accept(sd,NULL,NULL);
		if(client_sd < 0) {
			perror("accept");
			break;
		}
        
		memset(&sin,0,sizeof(&sin));
		len = sizeof(sin);
		getpeername(client_sd,(struct sockaddr*)&sin, &len);
        
		memset(&in,0,sizeof(in));
		in.s_addr = sin.sin_addr.s_addr;
        
        char * addr = inet_ntoa(in);
        printf(".");
        send( client_sd, addr, strlen(addr), NULL );
		close(client_sd);
	}
	close(sd);
	return 0;
}