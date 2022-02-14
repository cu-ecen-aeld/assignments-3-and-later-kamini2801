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
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#define MY_SOCK_PATH "/var/tmp/aesddata"
#define LISTEN_BACKLOG 50
#define SERVER_PORT "9000"
#define BUF_SIZE 1024
#define TRUE 1
#define FALSE 0

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
    openlog("assign2_log", LOG_PID | LOG_PERROR | LOG_CONS, LOG_USER);

    sfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        perror("socket");
    printf("Created socket\n");

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sa_family = AF_INET;

    //Setting up sockaddr using getaddrinfo()

    memset(&hints, 0, sizeof(hints)); //why??
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, SERVER_PORT, &hints, &serverinfo) != 0)
        perror("getaddrinfo");

    //server_addr.sa_addr=serverinfo->ai_addr;

    if (bind(sfd, serverinfo->ai_addr,
             sizeof(server_addr)) == -1)
    {       
        perror("bind");
        exit(-1);
    }
    printf("Bound scoket\n");

    if (listen(sfd, LISTEN_BACKLOG) == -1)
        perror("listen");

    /* Now we can accept incoming connections one
              at a time using accept(2). */

    client_addr_size = sizeof(client_addr);
    cfd = accept(sfd, (struct sockaddr *)&client_addr,
                 &client_addr_size);
    if (cfd == -1)
        perror("accept");
    printf("Accepted connection\n");

    /* 
    *   Code to deal with incoming connection(s)... 
    *
    * 
    * */
   remove(MY_SOCK_PATH);

    fd = open(MY_SOCK_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd == -1)
    {
        perror("open");
    }
    printf("Opened file\n");

    memset(&recv_buf, '\0', BUF_SIZE);

    //Extracting a packet
    int i;
    int recv_len = 0;
    int recv_complete = FALSE;
    int first_cycle = TRUE;
    char *tx_buf;

    while (!recv_complete)
    {
        int prev_size=0;

       if(recv(cfd, &recv_buf, BUF_SIZE, 0)==-1)
       {
           syslog(LOG_DEBUG, "Connection clsoed\n");
            break;
       }

        for (i = 0; i < BUF_SIZE; i++)
        {   
            recv_len++;

            if (recv_buf[i] == '\n')
            {
                recv_complete=TRUE;
                break;
            }
        }

        if(first_cycle)
        {
            tx_buf = (char*)malloc(recv_len);

            first_cycle=FALSE; 
        }
        else
        {   
            prev_size+= recv_len;
            
            tx_buf= (char*)realloc(tx_buf, recv_len); 

            if(tx_buf==NULL)    
                perror("realloc");
        }

        strncpy(tx_buf + prev_size, recv_buf, recv_len);    //copy present contents
    }

    printf("Write packet to file\n");

    //Appending Packet to the file
    int bytes = write(fd, tx_buf, recv_len);
    if (bytes == -1)
    {
        syslog(LOG_ERR, "Could not write to file\n");
        return -1;
    }
     printf("Writing %d bytes\n", bytes );

    lseek(fd, 0, SEEK_SET);
    bytes = read(fd, recv_buf, BUF_SIZE);
    if (bytes == -1)
    {
        syslog(LOG_ERR, "Could not read to file\n");
        return -1;
    }
    printf("Reading %d bytes\n", bytes );
    if(send(cfd, &recv_buf, bytes, 0)==-1)
    {
        syslog(LOG_DEBUG, "Connection clsoed\n");

    }
    freeaddrinfo(serverinfo);

    close(sfd);
    close(cfd);
    close(fd);


    printf("end\n");

    /* When no longer required, the socket pathname, MY_SOCK_PATH
              should be deleted using unlink(2) or remove(3). */
}