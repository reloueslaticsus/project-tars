/*
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Simple circular queue implementation
 */

#include "queue.h"
#include "kernel.h"
#include <spede/string.h>
/**
 * Initializes the specified queue data structure
 * @param queue - pointer to the queue
 * @return 0 on success, -1 on failure
 */
int queue_init(queue_t *queue) {
    if(!queue){
        kernel_log_error("Unable to initialize queue!");
        return -1;
    }
    //queue = items[QUEUE_SIZE];
    memset(queue,0,sizeof(queue_t));
    return 0;
}

/**
 * Adds an item to the tail of the queue
 * @param queue - pointer to the queue
 * @param item - item to add to the tail of the queue
 * @return 0 on success, -1 on failure
 */
int queue_in(queue_t *queue, int item) {
	//queue is at max capacity
    if(queue_is_full(queue)) {
        kernel_log_error("Queue full!");
        return -1;
    }

	//add item to queues tail
    queue->items[queue->tail] = item;
    queue->size++;
    queue->tail = (queue->tail+1) % QUEUE_SIZE;
    return 0;

}

/**
 * Removes an item from the head of the queue
 * @param queue - pointer to the queue
 * @param item - pointer to where the item will be saved
 * @return 0 on success, -1 on failure
 */
 int queue_out(queue_t *queue, int *item) {
    //queue is empty
    if(queue_is_empty(queue) || !item) {
        //kernel_log_error("Queue empty or item is invalid!");
        return -1;
    }
    //take item at the head, and pop it from queue into the item address
    *item = queue->items[queue->head];
    queue->size--;
    queue->head = (queue->head+1) % QUEUE_SIZE;
    return 0;
}
