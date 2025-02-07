#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include <stdio.h>
#include "queue.h"

queue* create_queue() {
    queue *q = (queue *)malloc(sizeof(queue));
    assert(q);
    q->front = q->back = NULL;
    return q;
}

bool is_queue_empty(queue *q) {
    if (q == NULL) {
        return true;
    }

    return (q->front == NULL);
}

void enqueue(queue *q, node_t *new_data) {
    queue_node *new_node = (queue_node *)malloc(sizeof(queue_node));
    assert(new_node);
    new_node->data = new_data; 
    new_node->next = NULL;
    
    if (q->back == NULL) {
        // If the queue is empty, both front and back are the same
        q->front = q->back = new_node;
        return;
    }

    // Add the new node to the end of the queue and update rear
    q->back->next = new_node;
    q->back = new_node;
}

node_t* dequeue(queue *q) {
    if (is_queue_empty(q)) {
        return NULL; 
    }
    // Get the front node and move the front pointer forward
    queue_node *temp = q->front;
    node_t *dequeued_data = temp->data;
    q->front = q->front->next;

    // If the front is now NULL, the queue is empty
    if (q->front == NULL) {
        q->back = NULL;
    }

    // Free the old front node
    free(temp);  
    return dequeued_data;
}


