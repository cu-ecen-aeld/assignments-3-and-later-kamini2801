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
#include <signal.h>

#define MY_SOCK_PATH "/var/tmp/aesddata"
#define LISTEN_BACKLOG 50
#define SERVER_PORT "9000"
#define BUF_SIZE 1024
#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[])
{
    int sfd, cfd, fd;
    struct sockaddr server_addr, client_addr;
    struct addrinfo hints;
    struct addrinfo *serverinfo, *temp; //points to results
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

    /* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully bind(2).
	  If socket(2) (or bind(2)) fails, we (close the socket
	  and) try the next address. */
    // taken from man page of getaddrinfo
    for (temp = serverinfo; temp != NULL; temp = temp->ai_next)
    {

        if (bind(sfd, serverinfo->ai_addr,
                 sizeof(server_addr)) != -1)
        {
            printf("temp->ai_addr = %p --- passed\n", temp->ai_addr);
            break;
        }
        printf("temp->ai_addr = %p --- failed\n", temp->ai_addr);
    }
    printf("Bound scoket\n");

    remove(MY_SOCK_PATH);

    fd = open(MY_SOCK_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd == -1)
    {
        perror("open");
    }
    printf("Opened file\n");

    while (1)
    {
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

        memset(&recv_buf, '\0', BUF_SIZE);

        //Extracting a packet
        int i;
        int recv_len = 0;
        int recv_complete = FALSE;
        int first_cycle = TRUE;
        char *tx_buf=NULL, *temp_ptr=NULL;
        int prev_size = 0;

        while (!recv_complete)
        {
            ssize_t recv_bytes;
            recv_len=0;             

            if ((recv_bytes=recv(cfd, &recv_buf, BUF_SIZE, 0)) == -1)
            {
                syslog(LOG_DEBUG, "Connection clsoed\n");
                break;
            }
            printf("R BYTES: %d\n", recv_bytes);
            for (i = 0; i < recv_bytes; i++)
            {
                recv_len++;

                if (recv_buf[i] == '\n')
                {
                    recv_complete = TRUE;
                    break;
                }
            }
            printf("for loop count:%d\n", recv_len);

            if (first_cycle)
            {
                tx_buf = (char *)malloc(recv_len);

                memset(tx_buf, '\0', recv_len);

                first_cycle = FALSE;
            }
            else
            {   
                prev_size += recv_len;
                printf("Prev size: %d\n", prev_size);
                if ((temp_ptr = realloc(tx_buf, prev_size + recv_len)) == NULL)
                {
                    perror("realloc");
                    exit(-1);
                }
                else
                {
                    tx_buf = temp_ptr;
                    memset(tx_buf + prev_size, '\0', recv_len);
                }
            }

            strncat(tx_buf, recv_buf, recv_len); //copy present contents
        }

        printf("Write packet to file\n");

        //Appending Packet to the file
        lseek(fd, 0, SEEK_END);
        int bytes = write(fd, tx_buf, prev_size + recv_len);
        if (bytes == -1)
        {
            syslog(LOG_ERR, "Could not write to file\n");
            return -1;
        }
        printf("Writing %d bytes\n", bytes);

        lseek(fd, 0, SEEK_SET);

        while ((bytes = read(fd, recv_buf, BUF_SIZE)) != 0)
        {
            //bytes = read(fd, recv_buf, BUF_SIZE);
            if (bytes == -1)
            {
                syslog(LOG_ERR, "Could not read to file\n");
                return -1;
            }
            //printf("Reading %d bytes\n", bytes);
            if (send(cfd, &recv_buf, bytes, 0) == -1)
            {
                syslog(LOG_DEBUG, "Connection clsoed\n");
            }
        }

        free(tx_buf);
        printf("end\n");
    }

    freeaddrinfo(serverinfo);

    close(sfd);
    close(cfd);
    close(fd);
    /* When no longer required, the socket pathname, MY_SOCK_PATH
              should be deleted using unlink(2) or remove(3). */
}