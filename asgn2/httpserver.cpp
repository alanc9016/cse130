/*********************************************
 * Name: Alan Caro
 * ID: 1650010
 * Assignment Name:  Assignment 1
 **********************************************/

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

pthread_t *threads;

sem_t empty;
sem_t full;
sem_t mutex;

int SHARED_RESOURCE;

char *LOGFILE;

// HTTP Errror Codes
const char errorCodes[4][70] = {
    "HTTP/1.1 400 Bad Request \r\nContent-Length: 0\r\n\r\n",
    "HTTP/1.1 403 Forbidden \r\nContent-Length: 0\r\n\r\n",
    "HTTP/1.1 404 Not Found \r\nContent-Length: 0\r\n\r\n",
    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n"};

// Checks if file name is valid
int isValidName(char fileName[]);

void *start(void *);

// Sends requests to processPut, processGet otherwise error
void processOneRequest(int socket);

void processPut(char fileName[], int socket, int size);
void processGet(char fileName[], int socket);

int main(int argc, char **argv) {
  char *hostname;
  char port[20];
  int N = 4;
  int option;

  while ((option = getopt(argc, argv, "N:l:")) != -1) {
    switch (option) {
    case 'N':
      N = atoi(optarg);
      break;
    case 'l':
      LOGFILE = optarg;
      break;
    case ':':
      printf("option needs a value\n");
      break;
    case '?':
      printf("unknown option: %c\n", optopt);
      break;
    }
  }

  hostname = argv[argc - 2];
  strcpy(port, argv[argc - 1]);

  threads = new pthread_t[N];

  sem_init(&mutex, 0, 1);
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, N);

  for (int i = 0; i < N; i++) {
    pthread_create(&threads[i], NULL, start, NULL);
  }

  // start of code obtained from Ethan Miller's Started Code
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
  // end of code obtained from Ethan Miller's Started Code

  while (true) {
    int client_socket = accept(main_socket, NULL, NULL);

    sem_wait(&empty);
    sem_wait(&mutex);

    SHARED_RESOURCE = client_socket;

    sem_post(&mutex);
    sem_post(&full);
  }

  return 0;
}

void *start(void *) {
  while (true) {
    sem_wait(&full);
    sem_wait(&mutex);

    int client_socket = SHARED_RESOURCE;

    sem_post(&mutex);
    sem_post(&empty);

    processOneRequest(client_socket);
  }

  return NULL;
}

void processOneRequest(int socket) {
  char request[20];
  char fileName[27];
  char buffer[1024];

  memset(buffer, 0, 1024);

  recv(socket, buffer, 1024, 0);
  sscanf(buffer, "%s %s", request, fileName);

  // ignore / on file name
  if (fileName[0] == '/')
    memmove(fileName, fileName + 1, strlen(fileName));

  // invalid file name
  if (isValidName(fileName) == -1) {
    send(socket, errorCodes[0], strlen(errorCodes[0]), 0);
  }

  if (strcmp(request, "GET") == 0) {
    processGet(fileName, socket);
  } else if (strcmp(request, "PUT") == 0) {
    char *line = strtok(buffer, "\r\n");
    char *array[7];
    char word[20];
    int i = 0;
    int j = 0;

    while (line != NULL) {
      array[i++] = line;
      line = strtok(NULL, "\r\n");
    }

    for (j = 0; j < 6; j++)
      if (strstr(array[j], "Content-Length: ") != NULL)
        break;

    // found content-length
    if (array[j] != NULL) {
      sscanf(array[j], "%*s %s", word);
      i = atoi(word);
    } else
      // set size -1 since no content-length was found
      i = -1;

    processPut(fileName, socket, i);
  } else {
    send(socket, errorCodes[0], strlen(errorCodes[0]), 0);
  }
}

int isValidName(char fileName[]) {
  int j;

  for (j = 0; fileName[j] != '\0'; ++j)
    ;

  if (j != 27)
    return -1;

  for (int i = 0; i < 27; i++) {
    char c = fileName[i];

    if (isalpha(c))
      continue;
    if (isdigit(c))
      continue;
    if (c == '-')
      continue;
    if (c == '_')
      continue;

    return -1;
  }

  return 0;
}

void processGet(char fileName[], int socket) {
  int fd;
  char buffer[32];

  fd = open(fileName, O_RDONLY);

  if (fd == -1) {
    // file was not found
    if (access(fileName, F_OK) == -1)
      send(socket, errorCodes[2], strlen(errorCodes[2]), 0);
    else
      // no read permission
      send(socket, errorCodes[1], strlen(errorCodes[1]), 0);

    return;
  }

  struct stat st;
  if (stat(fileName, &st) == -1)
    // unable to get size
    send(socket, errorCodes[3], strlen(errorCodes[3]), 0);

  int size = st.st_size;

  char str[1024];
  sprintf(str, "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n", size);

  // send file size
  send(socket, str, strlen(str), 0);

  // send file contents
  while (read(fd, buffer, 1))
    send(socket, buffer, 1, 0);

  close(fd);
}

void processPut(char fileName[], int socket, int size) {
  int fd;
  char buffer[32];
  fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);

  if (fd == -1) {
    // file is forbidden
    send(socket, errorCodes[1], strlen(errorCodes[1]), 0);
    return;
  }

  int i = 0;

  // write contents until size specified and while
  // there is still content
  while (i != size && read(socket, buffer, 1)) {
    write(fd, buffer, 1);

    i++;
  }

  close(fd);

  if (LOGFILE) {
    int fd_log = 0;
    fd = open(fileName, O_RDWR);
    fd_log = open(LOGFILE, O_CREAT | O_RDWR, 0644);
    char buffer_log[20];
    int address = 0;
    char* target = new char[20];
    
    int lineLength = sprintf(buffer_log, "PUT %s length %d\n", fileName, size);
    write(fd_log, buffer_log, lineLength);

    while(read(fd, buffer_log, 20)){
        for(int j = 0; j < 20; j++){
            target += sprintf(target, "%08x ", address);
            target += sprintf(target, "%02x ", (unsigned int) buffer_log[j]);
        }
        target += sprintf(target, "\n");
        write(fd_log, target, strlen(target));
        address+=20;
    }

    /*   sprintf(buffer_log, "%08x %02x", address, (unsigned int)*buffer); */
  }

  send(socket, "HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n",
       strlen("HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n"), 0);

}
