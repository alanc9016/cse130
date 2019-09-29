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
        char buf[32] = "";
        size_t nbytes;
        int fd;
        fd = open(argv[i],O_RDONLY);

        if(fd == -1){
            fprintf(stderr,"open failed: %s\n", strerror(errno));
            exit(1);
        }

        nbytes = sizeof(buf);
        while(read(fd, buf, nbytes)){
            nbytes = sizeof(buf);
            write(1,buf,nbytes);
        }

        close(fd);
    }
}

void readstdin(){
    char ch[20];

    while(read(STDIN_FILENO,ch, 1) > 0){
        //TODO FIX Output
        write(1,ch,20);
    }
}
