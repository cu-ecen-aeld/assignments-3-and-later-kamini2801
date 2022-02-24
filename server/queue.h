#include <sys/queue.h>
#include <stdint.h>

// Note the standard typedef of struct node to node_t for convenient use but
// the TAILQ_ENTRY macro uses the struct name.
typedef struct node
{
    int thread_id;
    int cfd;
    char ip[16];
    uint8_t comp_flag;
    TAILQ_ENTRY(node) nodes;

} node_t;

typedef TAILQ_HEAD(head_s, node) head_t;

node_t* _add_thread(head_t * head, const int thread, const int client_fd);
void _free_queue(head_t * head);
void _remove_thread(head_t * head, int thread_id);
void _print_queue(head_t * head);