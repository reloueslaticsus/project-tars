/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Simple circular queue implementation
 */
#ifndef QUEUE_H
#define QUEUE_H

// Queue size
#ifndef QUEUE_SIZE
#define QUEUE_SIZE 64
#endif

// Queue data structure
typedef struct {
    int items[QUEUE_SIZE];  // queue items
    int head;               // queue head
    int tail;               // queue tail
    int size;               // queue size
} queue_t;

/**
 * Function declarations
 */

/**
 * Initializes the specified queue data structure
 * @param queue - pointer to the queue
 * @return 0 on success, -1 on failure
 */
int queue_init(queue_t *queue);

/**
 * Adds an item to the queue
 * @param queue - pointer to the queue
 * @param item - item to add to the tail of the queue
 * @return 0 on success, -1 on failure
 */
int queue_in(queue_t *queue, int item);

/**
 * Removes an item from the head o fthe queue
 * @param queue - pointer to the queue
 * @param item - pointer to the buffer where the item will be saved
 * @return 0 on success, -1 on failure
 */
int queue_out(queue_t *queue, int *item);

/**
 * Determines if a queue is empty
 * @return 1 if true, 0 if false
 */
#define queue_is_empty(queue) ((queue) && (queue)->size == 0)

/**
 * Determines if a queue is full
 * @return 1 if true, 0 if false
 */
#define queue_is_full(queue) ((queue) && (queue)->size == QUEUE_SIZE)

#endif
