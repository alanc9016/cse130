/*********************************************
 * Name: Alan Caro
 * ID: 1650010
 * Assignment Name:  Assignment 0
 **********************************************/

#include <cerrno>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void readFiles(int argc, char **argv);
void readstdin();

int main(int argc, char **argv)
{
    if(argc > 1)
        // if arguments are passed
        readFiles(argc, argv);
    // read from stdin if no args are passed
    else
        readstdin();

    return 0;
}

void readFiles(int argc, char **argv)
{
    int isInvalidFile = 0;
    struct stat path_stat;

    for(int i = 1; i < argc; i++) {
        char buffer[32];
        int fd;

        // if '-' is passed called readstdin
        if(strcmp(argv[i], "-") == 0) {
            readstdin();
            continue;
        }

        // used to check if it is a directory
        stat(argv[i], &path_stat);

        if(S_ISDIR(path_stat.st_mode)) {
            warnx("%s: Is a directory", argv[i]);
            isInvalidFile = 1;
            continue;
        }

        fd = open(argv[i], O_RDONLY);

        if(fd == -1) {
            warn("%s", argv[i]);
            isInvalidFile = 1;
            continue;
        }

        while(read(fd, buffer, 1))
            write(STDOUT_FILENO, buffer, 1);

        close(fd);
    }

    if(isInvalidFile)
        exit(1);
}

void readstdin()
{
    char buffer[32];

    while(read(STDIN_FILENO, buffer, 1))
        write(STDOUT_FILENO, buffer, 1);
}
