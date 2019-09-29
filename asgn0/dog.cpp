#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

void readFiles(int argc, char **argv);
void readstdin();

int main(int argc, char **argv){
    if(argc > 1)
        readFiles(argc,argv);
    else
        readstdin();
    return 0;
}

void readFiles(int argc, char **argv){
    for(int i = 1; i < argc; i++){
        char buffer[32] = "";
        int fd;

        fd = open(argv[i], O_RDONLY);

        if(fd == -1){
            fprintf(stderr,"open failed: %s\n", strerror(errno));
            exit(1);
        }

        while(read(fd, buffer, 1))
            write(1,buffer,1);

        close(fd);
    }
}

void readstdin(){
    char buffer[32];

    while(read(STDIN_FILENO,buffer, 1) > 0)
        write(1,buffer,1);
}
