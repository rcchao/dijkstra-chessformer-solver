// Implementation of a simple queue using a linked list 

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include "node.h"

/****************** STRUCTURES ******************/

// Definition for the queue node
typedef struct queue_node {
    node_t *data;  
    struct queue_node *next;  
} queue_node;

// Definition for the queue structure
typedef struct queue {
    queue_node *front;  
    queue_node *back;  
} queue;

/****************** INTERFACE ******************/

queue* create_queue();
bool is_queue_empty(queue *q);
void enqueue(queue *q, node_t *new_data);
node_t* dequeue(queue *q);

#endif
