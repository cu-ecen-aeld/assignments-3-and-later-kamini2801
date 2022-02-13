/*
*   Socket program for native socket server
*   Author: Kamini Budke
*   Date: 02-12-2022
*   
*/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define MY_SOCK_PATH " /var/tmp/aesdsocketdata"
#define LISTEN_BACKLOG 50
#define SERVER_PORT "9000"
#define BUF_SIZE 500

// #define handle_error(msg)   \
//     do                      \
//     {                       \
//         perror(msg);        \
//         exit(EXIT_FAILURE); \
//     } while (0)

int main(int argc, char *argv[])
{
    int sfd, cfd, fd;
    struct sockaddr server_addr, client_addr;
    struct addrinfo hints;
    struct addrinfo *serverinfo; //points to results
    socklen_t client_addr_size;
    char recv_buf[BUF_SIZE];

    //open system logs in user mode
    openlog("assign2_log", LOG_PID, LOG_USER);

    sfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        perror("socket");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sa_family = AF_INET;

    //Setting up sockaddr using getaddrinfo()

    memset(&hints, 0, sizeof(hints));       //why??
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, SERVER_PORT, &hints, &serverinfo) != 0)
        perror("getaddrinfo");

    //server_addr.sa_addr=serverinfo->ai_addr;

    if (bind(sfd, serverinfo->ai_addr,
             sizeof(server_addr)) == -1)
        perror("bind");

    if (listen(sfd, LISTEN_BACKLOG) == -1)
        perror("listen");

    /* Now we can accept incoming connections one
              at a time using accept(2). */

    client_addr_size = sizeof(client_addr);
    cfd = accept(sfd, (struct sockaddr *)&client_addr,
                 &client_addr_size);
    if (cfd == -1)
        perror("accept");

    /* Code to deal with incoming connection(s)... */

    fd=open(MY_SOCK_PATH,O_CREAT|O_RDWR|O_APPEND, 0644);
    if(fd==-1){
        syslog(LOG_ERR, "Could not open file\n");
        return -1;
    }

    recv(cfd, &recv_buf, BUF_SIZE, MSG_DONTWAIT);

    int err=write(fd, recv_buf , BUF_SIZE);
    if(err==-1){
        syslog(LOG_ERR, "Could not write to file\n");
        return -1;
    }

    err=read(fd, recv_buf, 0);
    if(err==-1){
        syslog(LOG_ERR, "Could not write to file\n");
        return -1;
    }
    send(cfd, &recv_buf, BUF_SIZE, MSG_DONTWAIT);

    /* When no longer required, the socket pathname, MY_SOCK_PATH
              should be deleted using unlink(2) or remove(3). */
}