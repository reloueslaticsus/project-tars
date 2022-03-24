/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Kernel Process Handling
 */

#include <spede/string.h>
#include <spede/stdio.h>
#include <spede/time.h>
#include <spede/machine/proc_reg.h>

#include "kernel.h"
#include "kproc.h"
#include "scheduler.h"
#include "timer.h"

#include "queue.h"

queue_t run_queue;

/**
 * Update the current process' run time and CPU time
 */
void scheduler_timer() {
    current->run_time++;
    current->cpu_time++;
}

/**
 * Initialize the scheduler
 */
void scheduler_init() {
    /* Initialize the run queue */
    kernel_log_info("Initializing Scheduler");
    if(queue_init(&run_queue) == -1){
        kernel_log_error("Unable to initialize scheduler");
        return;
    }

    /* Register the timer callback */
    if(timer_callback_register(&scheduler_timer, 1, -1) == -1) {
        kernel_log_error("Unable to register scheduler timer!");
    }
}

/**
 * Executes the scheduler
 */
void scheduler_run() {

    if(current){
        //if we haven't expired our timeslice, return
        if(current->cpu_time < SCHEDULER_TIMESLICE) {
            return;
        }

        //if the current pid isn't the idle task, requeue
        if(current->pid != 0){
            queue_in(&run_queue, current->pid);
        }
        //set cpu time to 0 for good measure and set task to idle
        current->cpu_time = 0;
        current->state = IDLE;
    }

    //queue out the next process, and set it as our current task
    int pid;
    if(queue_out(&run_queue, &pid) == -1){
        current = pid_to_proc(0);
    } else {
        current = pid_to_proc(pid);
    }

    //check what we got back is valid
    if(current == NULL) {
        kernel_panic("Current process could not be set in scheduler!");
    }

    //process is now running
    current->state = RUNNING;
}

/**
 * Adds a process to the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_add(proc_t *proc) {
    if(!proc) {
        kernel_log_error("Unable to add invalid process to run queue!");
        return;
    }

    //set process state to idle and add to queue if it isn't the kernel idle task
    proc->state = IDLE;
    if(proc->pid != 0) {
        queue_in(&run_queue, proc->pid);
    }
}

/**
 * Removes a process from the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_remove(proc_t *proc) {
    if(!proc) {
        kernel_log_error("Unable to remove invalid process from run queue!");
        return;
    }

    /*
     * If the process we are trying to remove is the currently running one,
     * make sure it isn't the kernel idle task and then set its state to idle.
     * We don't add it back to the run queue and the cleanup should be done
     * elsewhere
     */
    if(proc == current) {
        if(proc->pid == 0) {
            return;
        }
        current->state = IDLE;
        current = NULL;
        return;
    }

    /*
     * Otherwise, cycle through all of the processes (dequeueing and requeueing)
     * until we find the process we are looking for or we have cycled through
     * them all. Once the correct process is found, we simply keep it dequeued.
     */
    int pid;
    for(int i = 0; i < run_queue.size; i++) {
        if(queue_out(&run_queue, &pid) == -1) {
            kernel_log_error("Attempted removal from empty run queue!");
            return;
        }

        if(pid == proc->pid) {
            proc_t* proc = pid_to_proc(pid);
            proc->state = NONE;
            return;
        }

        if(queue_in(&run_queue, pid) == -1) {
            kernel_log_error("Attempted requeue into full run queue!");
            return;
        }
    }
}

