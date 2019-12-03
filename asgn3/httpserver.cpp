/*********************************************
 * Name: Alan Caro
 * ID: 1650010
 * Assignment Name:  Assignment 3
 **********************************************/

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *LOGFILE;
bool ISCACHING = 0;
int FD_LOG = 0;
void printLog(char message[]);

// HTTP Errror Codes
const char errorCodes[4][70] = {
    "HTTP/1.1 400 Bad Request \r\nContent-Length: 0\r\n\r\n",
    "HTTP/1.1 403 Forbidden \r\nContent-Length: 0\r\n\r\n",
    "HTTP/1.1 404 Not Found \r\nContent-Length: 0\r\n\r\n",
    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n"};

// Checks if file name is valid
int isValidName(char fileName[]);

// Sends requests to processPut, processGet otherwise error
void processOneRequest(int socket);

void processPut(char fileName[], int socket, int size);
void processGet(char fileName[], int socket);

struct file {
  char *name;
  std::string contents;
  bool isDirty;
  int size;
};

class Cache {
public:
  void insert(file thisFile);
  void display(char *name);
  bool hasFile(char *name);
  void setDirty(char *name, int socket, int size);
  void remove();
  void sendContents(char *name, int socket);

private:
  std::list<file> files;
};

Cache CACHE;

void Cache::sendContents(char *name, int socket) {
  for (std::list<file>::iterator it = files.begin(); it != files.end(); ++it) {
    if (strcmp((it)->name, name) == 0) {
      char str[1024];
      sprintf(str, "HTTP/1.1 200 OK \r\nContent-Length: %lu\r\n\r\n",
              (it)->contents.size());
      send(socket, str, strlen(str), 0);
      send(socket, (it)->contents.c_str(), (it)->contents.size(), 0);
    }
  }
}

void Cache::setDirty(char *name, int socket, int size) {
  for (std::list<file>::iterator it = files.begin(); it != files.end(); ++it) {
    if (strcmp((it)->name, name) == 0) {
      (it)->isDirty = 1;
      uint8_t buffer[100];
      memset(buffer, 0, 100);
      std::string result;
      int i = 0;

      while (i != size && read(socket, buffer, 1)) {
        result.append(buffer, buffer + 1);
        i++;
      }

      (it)->contents = result;
      (it)->size = size;
    }
  }
}

void Cache::remove() {
  file frontFile = files.front();

  if (frontFile.isDirty) {
    int fd = open(frontFile.name, O_CREAT | O_RDWR | O_TRUNC, 0644);
    write(fd, frontFile.contents.c_str(), frontFile.contents.size());
    files.pop_front();
    close(fd);
  } else {
    files.pop_front();
  }
}

void Cache::insert(file thisFile) {
  thisFile.isDirty = 0;

  if (files.size() == 4) {
    remove();
    files.push_back(thisFile);
  } else {
    files.push_back(thisFile);
  }
}

void Cache::display(char *name) {
  for (std::list<file>::iterator it = files.begin(); it != files.end(); ++it) {
    if (strcmp((it)->name, name) == 0) {
      int address = 0;
      int length = 0;
      char target[100];
      int i = 0;
      std::string::iterator it1 = (it)->contents.begin();
      while (it1 != (it)->contents.end()) {
        if (i % 20 == 0) {
          if (i != 0) {
            length += sprintf(target + length, "\n");
            printLog(target);
            length = 0;
          }
          length += sprintf(target + length, "%08d ", address);
          address += 20;
        }
        length += sprintf(target + length, "%02x ", (unsigned char)*it1);
        i++;
        it1++;
      }

      printLog(target);
      char buff[20];
      strcpy(buff, "\n========\n");
      printLog(buff);
      break;
    }
  }
}

bool Cache::hasFile(char *name) {
  for (std::list<file>::iterator it = files.begin(); it != files.end(); ++it) {
    if (strcmp((it)->name, name) == 0) {
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {
  char *hostname;
  char port[20];
  int option;

  while ((option = getopt(argc, argv, "l:c")) != -1) {
    switch (option) {
    case 'l':
      LOGFILE = optarg;
      break;
    case 'c':
      ISCACHING = 1;
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

  if (LOGFILE)
    FD_LOG = open(LOGFILE, O_CREAT | O_RDWR | O_TRUNC, 0644);

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
    processOneRequest(client_socket);
  }

  return 0;
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
    if (LOGFILE) {
      char message[100];
      sprintf(message, "FAIL: %s %s HTTP --- response 400\n========\n", request,
              fileName);
      printLog(message);
    }
    send(socket, errorCodes[0], strlen(errorCodes[0]), 0);
    return;
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
    if (LOGFILE) {
      char message[100];
      sprintf(message, "FAIL: %s %s HTTP --- response 400\n========\n", request,
              fileName);

      printLog(message);
    }
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
  char *buffer = new char;

  fd = open(fileName, O_RDONLY);

  if (fd == -1) {
    // file was not found
    if (access(fileName, F_OK) == -1) {
      if (LOGFILE) {
        char message[100];
        sprintf(message, "FAIL: GET %s HTTP --- response 404\n========\n",
                fileName);

        printLog(message);
      }

      send(socket, errorCodes[2], strlen(errorCodes[2]), 0);
    } else {
      // no read permission
      if (LOGFILE) {
        char message[100];
        sprintf(message, "FAIL: GET %s HTTP --- response 403\n========\n",
                fileName);

        printLog(message);
      }
    }

    send(socket, errorCodes[1], strlen(errorCodes[1]), 0);

    return;
  }

  bool fileInCache = CACHE.hasFile(fileName);

  struct stat st;
  if (stat(fileName, &st) == -1)
    // unable to get size
    send(socket, errorCodes[3], strlen(errorCodes[3]), 0);

  int size = st.st_size;

  if (!LOGFILE && !ISCACHING) {
    char str[1024];
    sprintf(str, "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n", size);
    send(socket, str, strlen(str), 0);

    // send file contents
    while (read(fd, buffer, 1)) {
      send(socket, buffer, 1, 0);
    }
    return;
  }

  char message[100];
  file f;
  f.name = (char *)malloc(strlen(fileName));
  strcpy(f.name, fileName);

  if (ISCACHING) {
    if (fileInCache) {
      if (LOGFILE) {
        sprintf(message, "GET %s length 0 [was in cache]\n========\n",
                fileName);
        printLog(message);
      }
      CACHE.sendContents(fileName, socket);
      return;
    } else {
      f.size = size;
      char str[1024];
      sprintf(str, "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n", size);
      send(socket, str, strlen(str), 0);

      std::string result;
      while (read(fd, buffer, 1)) {
        std::string buf(buffer);
        result.append(buf);
        send(socket, buffer, 1, 0);
      }

      f.contents = result;
      CACHE.insert(f);
      if (LOGFILE) {
        sprintf(message, "GET %s length 0 [was not in cache]\n========\n",
                fileName);
        printLog(message);
      }
      // send file contents
      while (read(fd, buffer, 1)) {
        send(socket, buffer, 1, 0);
      }
      return;
    }
  } else {
    char str[1024];
    sprintf(str, "HTTP/1.1 200 OK \r\nContent-Length: %d\r\n\r\n", size);
    send(socket, str, strlen(str), 0);

    // send file contents
    while (read(fd, buffer, 1)) {
      send(socket, buffer, 1, 0);
    }
  }

  if (LOGFILE) {
    sprintf(message, "GET %s length 0\n========\n", fileName);
    printLog(message);
  }

  close(fd);
}

void processPut(char fileName[], int socket, int size) {
  int fd;
  char *buffer = new char;

  if (!LOGFILE && !ISCACHING) {
    fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);
    int i = 0;

    // write contents until size specified and while
    // there is still content
    while (i != size && read(socket, buffer, 1)) {
      write(fd, buffer, 1);
      i++;
    }

    send(socket, "HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n",
         strlen("HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n"), 0);
    return;
  }

  bool fileInCache = CACHE.hasFile(fileName);

  if (ISCACHING) {
    if (!fileInCache) {
      // put files on disk
      int i = 0;
      fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);

      // write contents until size specified and while
      // there is still content
      while (i != size && read(socket, buffer, 1)) {
        write(fd, buffer, 1);
        i++;
      }

      close(fd);

      std::string result;
      file f;

      f.name = (char *)malloc(strlen(fileName));
      f.size = size;

      strcpy(f.name, fileName);

      // bringing from disk and putting it on the cache
      fd = open(fileName, O_RDWR);
      while (read(fd, buffer, 1)) {
        std::string str(buffer);
        f.contents.append(buffer);
      }
      CACHE.insert(f);
    } else {
      CACHE.setDirty(fileName, socket, size);
    }
    if (!LOGFILE) {
      send(socket, "HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n",
           strlen("HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n"), 0);
    }
  }

  fd = open(fileName, O_RDWR);
  int address = 0;
  int length = 0;
  uint8_t buffer_log[100];
  char target[100];

  if (LOGFILE && ISCACHING) {
    if (fileInCache) {
      length += sprintf(target + length, "PUT %s length %d [was in cache]\n",
                        fileName, size);
      printLog(target);
      CACHE.display(fileName);
    } else {
      length +=
          sprintf(target + length, "PUT %s length %d [was not in cache]\n",
                  fileName, size);

      while (int k = read(fd, buffer_log, 20)) {
        length += sprintf(target + length, "%08d ", address);

        for (int j = 0; j < k; j++) {
          length +=
              sprintf(target + length, "%02x ", (unsigned int)buffer_log[j]);
        }

        length += sprintf(target + length, "\n");

        printLog(target);
        length = 0;
        address += 20;
      }
      close(fd);
      char buff[20];
      strcpy(buff, "========\n");
      printLog(buff);
    }
    send(socket, "HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n",
         strlen("HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n"), 0);
  } else if (LOGFILE) {
    // put files on disk
    int i = 0;
    fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);

    // write contents until size specified and while
    // there is still content
    while (i != size && read(socket, buffer, 1)) {
      write(fd, buffer, 1);
      i++;
    }

    close(fd);
    fd = open(fileName, O_RDWR);
    length += sprintf(target + length, "PUT %s length %d\n", fileName, size);
    while (int k = read(fd, buffer_log, 20)) {
      length += sprintf(target + length, "%08d ", address);

      for (int j = 0; j < k; j++) {
        length +=
            sprintf(target + length, "%02x ", (unsigned int)buffer_log[j]);
      }

      length += sprintf(target + length, "\n");

      printLog(target);
      length = 0;
      address += 20;
    }
    close(fd);
    char buff[20];
    strcpy(buff, "========\n");
    printLog(buff);

    send(socket, "HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n",
         strlen("HTTP/1.1 201 Created \r\nContent-Length: 0\r\n\r\n"), 0);
  }
}

void printLog(char message[]) {
  int lineLength = strlen(message);
  write(FD_LOG, message, lineLength);
}
