/*
*   Socket program for native socket server
*   Author: Kamini Budke
*   Date: 02-12-2022
*   @ref: https://beej.us/guide/bgnet/html/
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
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "queue.h"

#define MY_SOCK_PATH "/var/tmp/aesdsocketdata"
#define LISTEN_BACKLOG 50
#define SERVER_PORT "9000"
#define BUF_SIZE 1024
#define TRUE 1
#define FALSE 0

int sfd, fd;                                      // file descriptors
struct addrinfo *serverinfo = NULL, *temp = NULL; //points to results

pthread_mutex_t mutex;

head_t head;

void handler(int signo, siginfo_t *info, void *context)
{
    syslog(LOG_DEBUG, "Caught signnal, exiting");

    int ret = EXIT_SUCCESS;

    struct node *temp_thread = NULL;
    TAILQ_FOREACH(temp_thread, &head, nodes)
    {
            //close fd if not done in cleanup
            int close_ret=shutdown(temp_thread->cfd, SHUT_RDWR);

            if(close_ret)
                syslog(LOG_ERR, "close client fd failed with error: %s", strerror(close_ret));

            printf("Killing thread: %ld\n", temp_thread->thread_id);

            //kill thread
            int p_ret = -1;

            p_ret = pthread_kill((pthread_t)temp_thread->thread_id, SIGKILL);
            if (p_ret)
            {
                syslog(LOG_ERR, "pthread_kill failed with error: %s", strerror(p_ret));
                exit(EXIT_FAILURE);
            }

            // //remove thread from list
            // _remove_thread(&head, temp_thread->thread_id);

        
    }
    _free_queue(&head);

    pthread_mutex_destroy(&mutex);

    if (shutdown(sfd, SHUT_RDWR))
        ret = EXIT_FAILURE;

    if (close(sfd))
        ret = EXIT_FAILURE;

    if (close(fd))
        ret = EXIT_FAILURE;

    if (unlink(MY_SOCK_PATH))
        ret = EXIT_FAILURE;

    closelog();

    _exit(ret);
}

void signal_init()
{
    struct sigaction act = {0};

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &handler;
    if (sigaction(SIGTERM, &act, NULL) == -1)
    {
        syslog(LOG_ERR, "Sigaction");
       _exit(EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        syslog(LOG_ERR, "Sigaction");
        _exit(EXIT_FAILURE);
    }
}

void *socket_func(void *arg)
{
    node_t *node = arg;
    //Extracting a packet
    int i;
    char recv_buf[BUF_SIZE];
    int recv_len = 0;
    int recv_complete = FALSE;
    int first_cycle = TRUE;
    char *tx_buf = NULL, *temp_ptr = NULL;
    int prev_size = BUF_SIZE, total_len = 0;
    int bound = FALSE;         //flag for bind condition
    char ip4[INET_ADDRSTRLEN]; // space to hold the IPv4 string

    syslog(LOG_DEBUG, "Accepted connection from %s\n", node->ip);

    memset(&recv_buf, '\0', BUF_SIZE);

    while (!recv_complete)
    {
        ssize_t recv_bytes;
        recv_len = 0;

        if ((recv_bytes = recv(node->cfd, &recv_buf, BUF_SIZE, 0)) == -1)
        {
            syslog(LOG_ERR, "recv failed with error: %s", strerror(recv_bytes));
            exit(EXIT_FAILURE);
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
                syslog(LOG_ERR, "malloc");
                exit(EXIT_FAILURE);
            }

            memset(tx_buf, '\0', recv_len);

            first_cycle = FALSE;

            total_len = recv_len;
        }
        else
        {
            // printf("Prev size: %d\n", prev_size);
            if ((temp_ptr = realloc(tx_buf, prev_size + recv_len + 1)) == NULL)
            {
                syslog(LOG_ERR, "realloc");
                exit(EXIT_FAILURE);
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

    //Locking before accessing file
    pthread_mutex_lock(&mutex);

    //Appending Packet to the file
    lseek(fd, 0, SEEK_END);

    int bytes = write(fd, tx_buf, total_len);
    if (bytes < 0)
    {
        syslog(LOG_ERR, "write\n");
        exit(EXIT_FAILURE);
    }
    // printf("Writing %d bytes\n", bytes);

    lseek(fd, 0, SEEK_SET);

    while ((bytes = read(fd, recv_buf, BUF_SIZE)) != 0)
    {
        
        if (bytes < 0)
        {
            syslog(LOG_ERR, "read\n");
            exit(EXIT_FAILURE);
        }
        
        if (send(node->cfd, &recv_buf, bytes, 0) < 0)
        {
            syslog(LOG_ERR, "send\n");
            exit(EXIT_FAILURE);
        }
    }

    free(tx_buf);

    node->comp_flag = 1;

    //Unlock mutex after completing transactions with file and updating thread status
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *cleanup_func(void *arg)
{

    head_t *head = arg;
    while (1)
    {
        struct node *temp_thread = NULL;
        TAILQ_FOREACH(temp_thread, head, nodes)
        {
            if (temp_thread->comp_flag)
            {
                //printf("Found dead thread: %ld\n", temp_thread->thread_id);

                // close client fds
                close(temp_thread->cfd);
                syslog(LOG_DEBUG, "Closed connection from %s\n", temp_thread->ip);

                //join thread
                int p_ret = -1;
                p_ret = pthread_join((pthread_t)temp_thread->thread_id, NULL);
                if (p_ret)
                {
                    syslog(LOG_ERR, "pthread_join failed with error: %s", strerror(p_ret));
                    exit(EXIT_FAILURE);
                }

                //remove thread from list
                TAILQ_REMOVE(head, temp_thread, nodes);
                free(temp_thread);


                break;

            }
        }
        usleep(10);
    }
    return NULL;
}

void *time_func(void *arg)
{

    while (1)
    {
        // get time
        char outstr[200];
        time_t t;
        struct tm *tmp;

        t = time(NULL);
        tmp = localtime(&t);
        if (tmp == NULL)
        {
            perror("localtime");
            exit(EXIT_FAILURE);
        }

        if (strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T", tmp) == 0)
        {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }

        //printf("Result string is \"%s\"\n", outstr);

        //strncat(outstr, "\n\0", 2);

        //write
        char outstr_complete[100];
        strcpy(outstr_complete, "timestamp:\0");
        strncat(outstr_complete, outstr, strlen(outstr));

        strcat(outstr_complete, "\n");

        //Locking before accessing file
        pthread_mutex_lock(&mutex);

        int bytes = write(fd, outstr_complete, strlen(outstr_complete));

        //Unlock mutex after completing transactions with file and updating thread status
        pthread_mutex_unlock(&mutex);

        //sleeps
        sleep(10);
    }
}

int main(int argc, char *argv[])
{
    struct sockaddr server_addr, client_addr;
    struct addrinfo hints;
    socklen_t client_addr_size;
    //char recv_buf[BUF_SIZE];
    int bound = FALSE;         //flag for bind condition
    char ip4[INET_ADDRSTRLEN]; // space to hold the IPv4 string

    // Signal Handler setup for handling SIGTERM, SIGINT
    signal_init();

    //open system logs in user mode
    openlog("assign6.1_log", LOG_PID | LOG_PERROR | LOG_CONS, LOG_USER);

    //open socket

    //Setting up sockaddr using getaddrinfo()
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, SERVER_PORT, &hints, &serverinfo) != 0)
    {
        syslog(LOG_ERR, "getaddrinfo\n");
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
	  Try each address until we successfully bind(2).
	  If socket(2) (or bind(2)) fails, we (close the socket
	  and) try the next address. */
    // ref: man getaddrinfo

    for (temp = serverinfo; temp != NULL; temp = temp->ai_next)
    {
        sfd = socket(PF_INET, SOCK_STREAM, 0);

        if (sfd == -1)
        {
            syslog(LOG_ERR, "socket failed with error code: %s\n", strerror(sfd));
            exit(EXIT_FAILURE);
        }
        syslog(LOG_DEBUG, "Successfully opened Socket\n");

        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        {
            syslog(LOG_ERR, "socketopt\n");
            exit(EXIT_FAILURE);
        }

        if (bind(sfd, temp->ai_addr, //temp->ai_addrlen)!= -1)
                 sizeof(server_addr)) != -1)
        {
            //printf("temp->ai_addr = %p address allocated \n", temp->ai_addr);
            bound = TRUE;
            break;
        }
        //printf("temp->ai_addr = %p address failed\n", temp->ai_addr);
        close(sfd);
    }
    if (!bound)
    {
        syslog(LOG_ERR, "bind ");
        freeaddrinfo(serverinfo);
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "Bind socket success\n");

    freeaddrinfo(serverinfo);

    //Daemonize if -d argument passed
    int daemon_rec = -1;
    if (argc > 1)
    {
        if (!strncmp((char *)argv[1], "-d", 2))
        {
            syslog(LOG_DEBUG, "Running as Daemon\n");
            daemon_rec = daemon(0, 0);
        }
        if (daemon_rec < 0)
        {
            syslog(LOG_ERR, "daemon failed with error code: %s\n", strerror(daemon_rec));
            exit(EXIT_FAILURE);
        }
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1)
    {
        syslog(LOG_ERR, "listen\n");
        exit(EXIT_FAILURE);
    }

    remove(MY_SOCK_PATH);

    fd = open(MY_SOCK_PATH, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd == -1)
    {
        syslog(LOG_ERR, "open failed with error code: %s\n", strerror(fd));
        exit(EXIT_FAILURE);
    }

    syslog(LOG_DEBUG, "Opened %s\n", MY_SOCK_PATH);

    /*****************ASSIGNMENT 6 additions*****************/

    /* 
    *   QUEUE Initilialization
    *
    *   The queue is used to keep a track of threads created. 
    *   Each node in the queue stores the following elements
    *   1. thread_id
    *   2. client_fd
    *   3. thread_complete_flag
    * 
    *   The queue is updated and cleaned by a cleanup thread
    * 
    * */
    // declare the head
    
    TAILQ_INIT(&head); // initialize the head

    //Threads

    pthread_t thread;
    int p_ret = -1;

    //Global mutex required to access file
    p_ret = pthread_mutex_init(&mutex, NULL);
    if (p_ret)
    {
        syslog(LOG_ERR, "pthread_mutex_init failed with error: %s", strerror(p_ret));
        exit(EXIT_FAILURE);
    }

    printf("Creating pthread\n");
    //Cleanup thread for servicing all connection threads
    p_ret = pthread_create(&thread, NULL, cleanup_func, &head);
    if (p_ret)
    {
        syslog(LOG_ERR, "pthread_create failed with error: %s", strerror(p_ret));
        exit(EXIT_FAILURE);
    }

    //Time update thread for servicing all connection threads
    p_ret = pthread_create(&thread, NULL, time_func, &head);
    if (p_ret)
    {
        syslog(LOG_ERR, "pthread_create failed with error: %s", strerror(p_ret));
        exit(EXIT_FAILURE);
    }

    /**************************************************************************/

    while (1)
    {

        /*Accept incoming connections one at a time */

        client_addr_size = sizeof(client_addr);
        int cfd = accept(sfd, (struct sockaddr *)&client_addr,
                         &client_addr_size);
        if (cfd == -1)
        {
            syslog(LOG_ERR, "accept failed with error: %s\n", strerror(cfd));
            raise(SIGTERM);
        }

        struct sockaddr_in *sa = (struct sockaddr_in *)&client_addr;

        inet_ntop(AF_INET, &(sa->sin_addr), ip4, INET_ADDRSTRLEN);

        //Spawning new thread for connection handling

        node_t *new = _add_thread(&head, 0, cfd, ip4);

        p_ret = pthread_create(&thread, NULL, socket_func, new);
        if (p_ret != 0)
        {
            syslog(LOG_ERR, "pthread_create failed with error: %s", strerror(p_ret));
            exit(EXIT_FAILURE);
        }

        new->thread_id = (pthread_t)thread;
    }

    raise(SIGTERM);
}