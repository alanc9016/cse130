/*********************************************
 * Name: Alan Caro
 * ID: 1650010
 * Assignment Name:  Assignment 1
 **********************************************/

#include <stdio.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

char* readFile(char fileName[]);

int main(int argc, char **argv){
    char buffer[20];
    char* hostname;
    char* port;
    char* p = strtok(argv[1],":");
    char http_header[1024]="HTTP/1.1 200 OK\r\n";

    hostname = p;
    p = strtok(NULL,":");
    port = p;

    struct addrinfo *addrs, hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port, &hints, &addrs);
    int main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
    int enable = 1;
    setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);

    listen (main_socket, 16);

    int client_socket;
    while(1){
        char request[20];
        char fileName[20];

        client_socket = accept(main_socket, NULL, NULL);
        recv(client_socket, buffer,1024,0);

        printf("%s",buffer);

        sscanf(buffer,"%s %s", request, fileName);

        if(strcmp(request,"GET") == 0){
            memmove(fileName, fileName+1, strlen(fileName));
            readFile(fileName);
            send(client_socket,strcat(readFile(fileName),http_header),
                    sizeof(strcat(readFile(fileName),http_header)),1);
        }
        else if(strcmp(request,"PUT") == 0){
        }

        close(client_socket);
    }

    return 0;
}

char* readFile(char fileName[]){
    struct stat path_stat;

    char buffer[32000];
    int fd;

    // used to check if it is a directory
    stat(fileName, &path_stat);

    if(S_ISDIR(path_stat.st_mode)) {
        /* warnx("%s: Is a directory", argv[i]); */
    }

    fd = open(fileName, O_RDONLY);

    if(fd == -1){
        warn("%s", fileName);
        return nullptr;
    }

    read(fd, buffer, 32000);

    close(fd);
    return buffer;
}
