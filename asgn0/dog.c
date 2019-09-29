#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv){
    char buf[20];
    size_t nbytes;
    int fd;


    if(argc > 1){
        fd = open(argv[1],O_RDONLY);

        if(fd == -1){
            fprintf(stderr,"Error");
            return 1;
        }

        nbytes = sizeof(buf);
        while(read(fd, buf, nbytes)){
            nbytes = sizeof(buf);
            write(1,buf,nbytes);
        }

        close(fd);
    }
    else{
        char ch[20];
        while(read(STDIN_FILENO,ch, 1) > 0){
            write(1,ch,20);
        }
    }
    return 0;
}
