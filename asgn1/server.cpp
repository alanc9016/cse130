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
#include <sys/types.h>
#include <unistd.h>

char *readFile(char fileName[], char message[], int size, bool isGetRequest);

int main(int argc, char **argv) {
  char buffer[1024];
  char *hostname;
  char port[20];

  if (argc > 1) {
    hostname = argv[1];
    if (argv[2] != nullptr)
        strcpy(port, argv[2]);
    else
        strcpy(port, "80");

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
      memset(buffer, 0, 1024);
      char request[20];
      char fileName[20];

      client_socket = accept(main_socket, NULL, NULL);
      recv(client_socket, buffer, 1024, 0);

      printf("%s", buffer);

      sscanf(buffer, "%s %s", request, fileName);

      if (fileName[0] == '/')
        memmove(fileName, fileName + 1, strlen(fileName));
      if (strcmp(request, "GET") == 0) {
        if (readFile(fileName, nullptr, 0, 1) == nullptr) {
          send(client_socket, "HTTP/1.1 404 Not Found\r\n",
               strlen("HTTP/1.1 404 Not Found\r\n"), 0);
        } else
          send(client_socket, strcat(readFile(fileName, nullptr, 0, 1), ""),
               strlen(strcat(readFile(fileName, nullptr, 0, 1), "")), 0);
      } else if (strcmp(request, "PUT") == 0) {
        char *array[7];
        char word[20];
        int i = 0;
        char *line = strtok(buffer, "\r\n");

        while (line != nullptr) {
          array[i++] = line;
          line = strtok(nullptr, "\r\n");
        }
        sscanf(array[4], "%*s %s", word);
        i = atoi(word);
        recv(client_socket, buffer, 1024, 0);
        readFile(fileName, buffer, i, 0);
        send(client_socket, "Content-Length: 0 HTTP/1.1 200 OK\r\n",
             strlen("Content-Length: 0 HTTP/1.1 200 OK\r\n"), 0);
      }
    }
  }
  return 0;
}

char *readFile(char fileName[], char message[], int size, bool isGetRequest) {
  static char buffer[32000];
  int fd;

  if (isGetRequest) {
    fd = open(fileName, O_RDONLY);

    if (fd == -1) {
      warn("%s", fileName);
      return nullptr;
    }

    read(fd, buffer, 32000);

    close(fd);
    return buffer;
  } else {
    fd = open(fileName, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
      warn("%s", fileName);
      return nullptr;
    }

    write(fd, message, size);

    close(fd);
  }

  return nullptr;
}
