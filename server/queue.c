#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

// gcc queue_example.c -o queue_example

// Note the standard typedef of struct node to node_t for convenient use but
// the TAILQ_ENTRY macro uses the struct name.
// typedef struct node
// {
//     //char c;
//     int thread_id;
//     uint8_t comp_flag;
//     TAILQ_ENTRY(node) nodes;

// } node_t;
// This typedef creates a head_t that makes it easy for us to pass pointers to
// head_t without the compiler complaining.
//typedef TAILQ_HEAD(head_s, node) head_t;


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
        printf("%d", e->thread_id);
    }
}
/*
int main (int arc, char * argv [])
{
    // declare the head
    head_t head;
    TAILQ_INIT(&head); // initialize the head

    // fill the queue with "Hello World\n"
   // _fill_queue(&head, "Hello World!\n");
   for(int a=0;  a<10; a++){
       _add_thread(&head, a, a);
   }

    printf("Forwards: ");
    _print_queue(&head); // prints "Hello World!\n"
 

    int thread_to_be_rem=-1;
    printf("Enter thread_id to be removed: \n");

    thread_to_be_rem=6;

    _remove_thread(&head, thread_to_be_rem);
    printf("After removing:\n");
    _print_queue(&head); // prints ""
    
    // free the queue
    _free_queue(&head);
    _print_queue(&head); // prints ""

    return EXIT_SUCCESS;
}*/