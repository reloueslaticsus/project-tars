/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Timer Implementation
 */
#include <spede/machine/io.h>
#include <spede/string.h>

#include "interrupts.h"
#include "queue.h"

#include "timer.h"
#include "vga.h"
#include "kernel.h"

/**
 * Forward Declarations
 */
void timer_irq_handler(void);

/**
 * Data structures
 */
// Timer data structure
typedef struct timer_t {
    void (*callback)(); // Function to call when the interval occurs
    int interval;       // Interval in which the timer will be called
    int repeat;         // Indicate how many intervals to repeat (-1 should repeat forever)
} timer_t;


/**
 * Variables
 */
// Number of timer ticks that have occured
int timer_ticks;

// Timers table; each item in the array is a timer_t struct
timer_t timers[TIMERS_MAX];

// Timer allocator; used to allocate indexes into the timers table
queue_t timer_allocator;


/**
 * Initializes timer related data structures and variables
 */
void timer_init(void) {
    kernel_log_info("Initializing Timer");
    timer_ticks = 0;
    memset(timers, 0, sizeof(timer_t));
    queue_init(&timer_allocator);
    for(int i = 0; i < TIMERS_MAX; i++){
        if(queue_in(&timer_allocator, i) == -1){
            kernel_log_error("Error on timer queue allocation");
            break;
        }
    }

    interrupts_irq_register(IRQ_TIMER, isr_entry_timer, timer_irq_handler);
}

/**
 * Registers a new callback to be called at the specified interval
 * @param func_ptr - function pointer to be called
 * @param interval - number of ticks before the callback is performed
 * @param repeat   - Indicate how many intervals to repeat (-1 should repeat forever)
 *
 * @return the allocated timer id or -1 for errors
 */
int timer_callback_register(void (*func_ptr)(), int interval, int repeat) {
    int timer_id = -1;
    if(queue_out(&timer_allocator, &timer_id) == -1){
        kernel_log_error("Unable to unregister timer callback!");
        return -1;
    }
    timers[timer_id].callback = func_ptr;
    timers[timer_id].interval = interval;
    timers[timer_id].repeat = repeat;
    return timer_id;
}

/**
 * Unregisters the specified callback
 * @param id
 *
 * @return 0 on success, -1 on error
 */
int timer_callback_unregister(int id) {
    if(id < 0 || id >= TIMERS_MAX){
        kernel_log_error("Timer ID out of bounds!");
        return -1;
    }
    timers[id].callback = NULL;
    timers[id].interval = 0;
    timers[id].repeat = 0;
    if(queue_in(&timer_allocator, id) == -1){
        kernel_log_error("Unable to queue timer id!");
        return -1;
    }
    return 0;
}

/**
 * Returns the current system time (in ticks)
 *
 * @return timer_ticks
 */
int timer_get_system_time() {
    return timer_ticks;
}

/**
 * Timer IRQ Handler
 *
 * Should perform the following:
 *   - Increment the timer ticks every time the timer occurs
 *   - Handle each registered timer
 *     - If the interval is hit, run the callback function
 *     - Handle timer repeats
 */
void timer_irq_handler(void) {
    timer_ticks++;
    for(int i = 0; i < TIMERS_MAX; i++){
        if(timers[i].callback != NULL && timer_ticks % timers[i].interval == 0){
            timers[i].callback();
            if(timers[i].repeat > 0){
                timers[i].repeat--;
            }else if(timers[i].repeat == 0){
                timer_callback_unregister(i);
            }
        }
    }
}
