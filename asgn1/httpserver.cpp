/*********************************************
 * Name: Alan Caro
 * ID: 1650010
 * Assignment Name:  Assignment 1
 **********************************************/

#include <arpa/inet.h>
#include <ctype.h>
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

int isValidName(char fileName[]);
void readFile(char fileName[], int socket, bool isGetRequest, int size);

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

    while (true) {
      memset(buffer, 0, 1024);
      char request[20];
      char fileName[27];

      client_socket = accept(main_socket, NULL, NULL);
      recv(client_socket, buffer, 1024, 0);

      printf("%s", buffer);

      sscanf(buffer, "%s %s", request, fileName);

      if (fileName[0] == '/')
        memmove(fileName, fileName + 1, strlen(fileName));

      if (strcmp(request, "GET") == 0)
        readFile(fileName, client_socket, true, 0);
      else if (strcmp(request, "PUT") == 0) {
        char *array[7];
        char word[20];
        int i = 0;
        char *line = strtok(buffer, "\r\n");

        if (isValidName(fileName) == -1){
          send(client_socket,
               "HTTP/1.1 400 Bad Request \r\nContent-Length: 0\r\n\r\n",
               strlen("HTTP/1.1 400 Bad Request \r\nContent-Length: 0\r\n\r\n"),
               0);
          continue;
        }

        while (line != nullptr) {
          array[i++] = line;
          line = strtok(nullptr, "\r\n");
        }

        sscanf(array[4], "%*s %s", word);
        i = atoi(word);

        readFile(fileName, client_socket, false, i);
      }
    }
  }

  return 0;
}

int isValidName(char fileName[]){
    int j;

    for(j = 0; fileName[j] != '\0'; ++j);

    if(j != 27)
        return -1;

    for(int i = 0; i < 27; i++){
        char c = fileName[i];

        if(isalpha(c))
           continue;
        if(isdigit(c))  
            continue;
        if(c=='-')
            continue;
        if(c=='_')
            continue;

        return -1;
    }

    return 0;
}

void readFile(char fileName[], int socket, bool isGetRequest, int size) {
  int fd;
  char buffer[32];

  if (isGetRequest) {
    fd = open(fileName, O_RDONLY);

    if (fd == -1) {
      send(socket, "HTTP/1.1 404 Not Found \r\nContent-Length: 0\r\n\r\n",
           strlen("HTTP/1.1 404 Not Found \r\nContent-Length: 0\r\n\r\n"), 0);
      return;
    }

    struct stat st;
    stat(fileName, &st);
    size = st.st_size;

    char str[1024];
    sprintf(str, "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n", size);

    send(socket, str, strlen(str), 0);

    while (read(fd, buffer, 1)) {
      size += 1;
      send(socket, buffer, 1, 0);
    }
  } else {
    fd = open(fileName, O_CREAT | O_RDWR, 0644);

    if (fd == -1) {
      send(socket, "HTTP/1.1 500 \r\nContent-Length: 0\r\n\r\n",
           strlen("HTTP/1.1 500 Internal Server Error \r\nContent-Length: "
                  "0\r\n\r\n"),
           0);
      return;
    }

    recv(socket, buffer, 1024, 0);
    write(fd, buffer, size);

    send(socket, "HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n",
         strlen("HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n"), 0);
  }
}
