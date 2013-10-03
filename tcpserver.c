#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main (int argc, char *argv[])
{
	int sock, listener;
    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read;


    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        perror("socket");
		
        return 0;
    }

	addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    if(bind(listener, (struct sockaddr *)&addr, sizeof(addr))<0)
    {
        perror("bind");
        return 1;
    }

    listen(listener, 1);
    
    while(1)
    {
        sock = accept(listener, NULL, NULL);
        if(sock < 0)
        {
            perror("accept");
            return 2;
        }

        while(1)
        {
            bytes_read = recv(sock, buf, 1024, 0);
            if(bytes_read <= 0) 
				break;
			else 
		send(sock,buf,bytes_read,0);
				
        }
    
        close(sock);
    }
    return 0;
}



