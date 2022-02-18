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

int sfd, cfd, fd;

void handler(int signo, siginfo_t *info, void *context)
{
    struct sigaction oldact;

    syslog(LOG_DEBUG, "\n\nKilling process\n\n");

    shutdown(cfd, SHUT_RDWR);
    close(cfd);
    close(sfd);

    if (!remove(MY_SOCK_PATH))
    {
        _exit(EXIT_SUCCESS);
    }
    _exit(EXIT_FAILURE);
}

void signal_init()
{
    struct sigaction act = {0};

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &handler;
    if (sigaction(SIGTERM, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
}


int main(int argc, char *argv[])
{
    struct sockaddr server_addr, client_addr;
    struct addrinfo hints;
    struct addrinfo *serverinfo, *temp; //points to results
    socklen_t client_addr_size;
    char recv_buf[BUF_SIZE];
    int connected = FALSE;

    signal_init();

    if (argc > 1)
    {
        printf("Supposed to run as daemon\n");
        if (!strncmp((char *)argv[1], "-d", 2))
        {
            syslog(LOG_DEBUG, "Running as Daemon\n");
            daemon(0, 0);
        }
    }

    //open system logs in user mode
    openlog("assign2_log", LOG_PID | LOG_PERROR | LOG_CONS, LOG_USER);

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

        sfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        printf("Created socket\n");

        if (bind(sfd, serverinfo->ai_addr,
                 sizeof(server_addr)) != -1)
        {
            printf("temp->ai_addr = %p --- passed\n", temp->ai_addr);
            connected = TRUE;
            break;
        }
        printf("temp->ai_addr = %p --- failed\n", temp->ai_addr);
        connected = FALSE;
        close(sfd);
    }
    if (!connected)
    {
        printf("\n\nSOCKET FAILED\n\n");
        raise(SIGTERM);
    }
    printf("Bound socket\n");

    freeaddrinfo(serverinfo);

    if (listen(sfd, LISTEN_BACKLOG) == -1)
        perror("listen");

    remove(MY_SOCK_PATH);

    fd = open(MY_SOCK_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd == -1)
    {
        perror("open");
    }
    printf("Opened file\n");

    while (1)
    {

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
        char *tx_buf = NULL, *temp_ptr = NULL;
        int prev_size = BUF_SIZE, total_len = 0;

        while (!recv_complete)
        {
            ssize_t recv_bytes;
            recv_len = 0;

            if ((recv_bytes = recv(cfd, &recv_buf, BUF_SIZE, 0)) == -1)
            {
                syslog(LOG_DEBUG, "Connection clsoed\n");
                break;
            }
            for (i = 0; i < recv_bytes; i++)
            {
                recv_len++;

                if (recv_buf[i] == '\n')
                {
                    recv_complete = TRUE;
                    break;
                }
            }

            if (first_cycle)
            {
                tx_buf = (char *)malloc(recv_len + 1);

                if (tx_buf == NULL)
                {
                    perror("malloc");
                    exit(-1);
                }

                memset(tx_buf, '\0', recv_len);

                first_cycle = FALSE;

                total_len = recv_len;
            }
            else
            {
                printf("Prev size: %d\n", prev_size);
                if ((temp_ptr = realloc(tx_buf, prev_size + recv_len + 1)) == NULL)
                {
                    perror("realloc");
                    exit(-1);
                }
                else
                {
                    tx_buf = temp_ptr;
                    memset(tx_buf + prev_size, '\0', recv_len);
                    prev_size += recv_len;
                }
                total_len = prev_size;
            }

            strncat(tx_buf, recv_buf, recv_len); //copy present contents
        }

        printf("Write packet to file\n");

        //Appending Packet to the file
        lseek(fd, 0, SEEK_END);
        int bytes = write(fd, tx_buf, total_len);
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
    /* When no longer required, the socket pathname, MY_SOCK_PATH
              should be deleted using unlink(2) or remove(3). */
}