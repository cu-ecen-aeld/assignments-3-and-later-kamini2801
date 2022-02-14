#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MY_SOCK_PATH "/var/tmp/aesddata"
#define BUF_SIZE 1024

int fd; //file descriptor

int main(){

    int fd, bytes;
    char recv_buf[BUF_SIZE];

    fd = open(MY_SOCK_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd == -1)
    {
        perror("open");
    }
    printf("Opened file\n");


    bytes = read(fd, recv_buf, BUF_SIZE);
    if (bytes == -1)
    {
        syslog(LOG_ERR, "Could not read to file\n");
        return -1;
    }
    printf("Reading %d bytes\n", bytes );
    close(fd);
}