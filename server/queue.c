#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"


// Add thread to taila nd return pointer to last element (i.e. tail)
node_t* _add_thread(head_t * head, const unsigned long thread, const int client_fd, char* ip_string)
{
    int c = 0;
    //for (c = 0; c < strlen(string); ++c)
    //{
        struct node * e = malloc(sizeof(struct node));
        if (e == NULL)
        {
            fprintf(stderr, "malloc failed");
            exit(EXIT_FAILURE);
        }
        e->thread_id = thread;
        e-> cfd = client_fd;
        e->comp_flag = 0;
        
        strncpy(e->ip, ip_string, 16);
    
        TAILQ_INSERT_TAIL(head, e, nodes);
        e = NULL;
    //}
    return TAILQ_LAST(head, head_s);
}

// Removes all of the elements from the queue before free()ing them.
void _free_queue(head_t * head)
{
    struct node * e = NULL;
    while (!TAILQ_EMPTY(head))
    {
        e = TAILQ_FIRST(head);
        TAILQ_REMOVE(head, e, nodes);
        free(e);
        e = NULL;
    }
}

void _remove_thread(head_t * head, unsigned long thread_id)
{
    struct node * e = NULL;
     TAILQ_FOREACH(e, head, nodes)
    {
        if(e->thread_id==thread_id)
        {
            TAILQ_REMOVE(head, e, nodes);
            free(e);
            break;
        }
    }
    
}


// Prints the queue by traversing the queue forwards.
void _print_queue(head_t * head)
{
    struct node * e = NULL;
    TAILQ_FOREACH(e, head, nodes)
    {
        printf("%ld", e->thread_id);
    }
}
