/*********************************************
 * Name: Alan Caro
 * ID: 1650010
 * Assignment Name:  Assignment 1
 **********************************************/

#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *readFile(char fileName[]);

int main(int argc, char **argv) {
  char buffer[20];
  char *hostname;
  char *port;
  char *p = strtok(argv[1], ":");

  hostname = p;
  p = strtok(NULL, ":");
  port = p;

  struct addrinfo *addrs, hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  getaddrinfo(hostname, port, &hints, &addrs);
  int main_socket =
      socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
  int enable = 1;
  setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);

  listen(main_socket, 16);

  int client_socket;
  while (1) {
    char request[20];
    char fileName[20];

    client_socket = accept(main_socket, NULL, NULL);
    recv(client_socket, buffer, 1024, 0);

    printf("%s", buffer);

    sscanf(buffer, "%s %s", request, fileName);

    if (strcmp(request, "GET") == 0) {
      memmove(fileName, fileName + 1, strlen(fileName));
      if (readFile(fileName) == nullptr) {
        send(client_socket, "HTTP/1.1 404 Not Found\r\n",
             strlen("HTTP/1.1 404 Not Found\r\n"), 0);
      } else
        send(client_socket, strcat(readFile(fileName), ""),
             strlen(strcat(readFile(fileName), "")), 0);
    } else if (strcmp(request, "PUT") == 0) {
        char *array[7];
        char word[20];
        int i = 0;
        char *line = strtok(buffer, "\r\n");

        while(line != nullptr){
            array[i++]= line;
            line = strtok(nullptr,"\r\n");
        }
        sscanf(array[4],"%*s %s",word);
        i = atoi(word);
        recv(client_socket, buffer, 1024, 0);
        printf("%s", buffer);
        send(client_socket, "HTTP/1.1 200 OK\r\n",
                strlen("HTTP/1.1 200 OK\r\n"), 0);
    }

    close(client_socket);
  }

  return 0;
}

char *readFile(char fileName[]) {
  struct stat path_stat;

  static char buffer[32000];
  int fd;

  // used to check if it is a directory
  stat(fileName, &path_stat);

  if (S_ISDIR(path_stat.st_mode)) {
    /* warnx("%s: Is a directory", argv[i]); */
  }

  fd = open(fileName, O_RDONLY);

  if (fd == -1) {
    warn("%s", fileName);
    return nullptr;
  }

  read(fd, buffer, 32000);

  close(fd);
  return buffer;
}
