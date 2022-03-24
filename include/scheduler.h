/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Kernel Process Scheduler
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kproc.h"

#ifndef SCHEDULER_TIMESLICE
#define SCHEDULER_TIMESLICE 250
#endif


/**
 * Initializes the scheduler
 */
void scheduler_init();

/**
 * Executes the scheduler
 */
void scheduler_run();

/**
 * Adds a process to the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_add(proc_t *proc);

/**
 * Removes a process from the scheduler
 * @param proc - pointer to the process entry
 */
void scheduler_remove(proc_t *proc);

#endif
