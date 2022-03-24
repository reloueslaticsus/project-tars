/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Kernel Process Handling
 */

#include <spede/stdio.h>
#include <spede/string.h>
#include <spede/machine/proc_reg.h>

#include "kernel.h"
#include "trapframe.h"
#include "kproc.h"
#include "scheduler.h"
#include "timer.h"
#include "queue.h"
#include "vga.h"

#define LINE_WIDTH 55

// Next available process id to be assigned
int next_pid;

// Process table allocator
queue_t proc_allocator;

// Process table
proc_t proc_table[PROC_MAX];

// Process stacks
unsigned char proc_stack[PROC_MAX][PROC_STACK_SIZE];

/**
 * Looks up a process in the process table via the process id
 * @param pid - process id
 * @return pointer to the process entry, NULL or error or if not found
 */
proc_t *pid_to_proc(int pid) {
    for(int i = 0; i < PROC_MAX; i++) {
        if(proc_table[i].pid == pid) {
            return &proc_table[i];
        }
    }
    return NULL;
}

void displayProcs() {
    char buff[(LINE_WIDTH - 1) * 11 + 1] = {0};
    char line[LINE_WIDTH] = {0};

    //clear from previous disply
    int x = 0;
    int y = 0;
    for(int i = 0; i < (LINE_WIDTH - 1) * 11 + 1; i++) {
        if(i % (LINE_WIDTH - 1) == 0) {
            y++;
            x = 0;
        } else {
            vga_put(x, y, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY, 0x00);
            x++;
        }
    }

    //buffer all running and idle process info
    snprintf(buff, sizeof(line) - 1, "%s%8s%10s%15s%15s\n", "ENTRY", "PID", "STATE", "TIME", "NAME");
    for(int i = 0; i < PROC_MAX; i++) {
        if(proc_table[i].state == IDLE || proc_table[i].state == RUNNING) {
            snprintf(line, sizeof(line) - 1, "%5d%8d%10c%15d%15s\n",
                     i, proc_table[i].pid, (proc_table[i].state == IDLE ? 'I' : 'R'),
                     proc_table[i].run_time, proc_table[i].name);
            strcat(buff, line);

        }
    }

    //manually print everything since clearing the screen will interfere with the spinner and uptime
    x = 0;
    y = 0;
    for(int i = 0; buff[i] != '\0'; i++) {
        if(buff[i] == '\n') {
            y++;
            x = 0;
        } else {
            vga_put(x, y, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY, buff[i]);
            x++;
        }
    }

}

/**
 * Initializes all process related data structures
 * Creates the first process (kernel_idle)
 * Registers the callback to display the process table/status
 */
void kproc_init(void) {
    kernel_log_info("Initializing process table");
    next_pid = 0;
    // Initialize the process allocator
    queue_init(&proc_allocator);
    for(int i = 0; i < PROC_MAX; i++) {
        if(queue_in(&proc_allocator, i) == -1) {
            kernel_log_error("Couldn't queue another pid!");
        }
    }
    // Initialize the process table
    memset(proc_table, 0, sizeof(proc_t)*PROC_MAX);
    // Initialize the process stacks
    memset(proc_stack, 0, PROC_MAX * PROC_STACK_SIZE);
    // Create the idle task as a kernel process
    kernel_log_info("Launching the idle task");
    kproc_create(kernel_idle, "idle", PROC_TYPE_KERNEL);

    // Add a timer callback that displays the status of all processes that have been created
    timer_callback_register(&displayProcs, 1, -1);
}

/**
 * Creates a new process
 * @param proc_ptr - address of process to execute
 * @param proc_name - "friendly" process name
 * @param proc_type - process type (kernel or user)
 * @return process id of the created process, -1 on error
 */
int kproc_create(void *proc_ptr, char *proc_name, proc_type_t proc_type) {
    // Allocate the PCB entry for the process from the process allocator
    //kernel_log_trace("kproc_create()");
    int entryId;
    if(queue_out(&proc_allocator, &entryId) == -1){
        kernel_log_warn("Process creation failed: at limit!");
        return -1;
    }
    // Initialize the PCB entry for the process
    memset(&proc_table[entryId], 0, sizeof(proc_t));
    // Set the stack pointer for the process within proc_stack
    proc_table[entryId].stack = proc_stack[entryId];
    // Initialize the stack
    memset(proc_table[entryId].stack, 0, PROC_STACK_SIZE);
    // Set the pid to a unique value (next_pid)
    proc_table[entryId].pid = next_pid;
    next_pid++;
    // Initialize process control block variables to default values
    proc_table[entryId].state = NONE;
    proc_table[entryId].type = proc_type;
    proc_table[entryId].start_time = timer_get_system_time();
    proc_table[entryId].run_time = 0;
    proc_table[entryId].cpu_time = 0;
    // Copy the process name to the PCB
    strncpy(proc_table[entryId].name, proc_name, PROC_NAME_LEN - 1);
    // Allocate the trapframe pointer at the top of the stack (initially empty, so the bottom, effectively)
    // proc->trapframe = (trapframe_t *)(&proc->stack[PROC_STACK_SIZE - sizeof(trapframe_t)]);
    proc_table[entryId].trapframe = (trapframe_t*)(&proc_table[entryId].stack[PROC_STACK_SIZE - sizeof(trapframe_t)]);
    // Allocate the trapframe data:
    //   eip     = proc_ptr
    //   eflags  = EF_DEFAULT_VALUE | EF_INTR
    //   cs      = get_cs()
    //   ds      = get_ds()
    //   es      = get_es()
    //   fs      = get_fs()
    //   gs      = get_gs()
    proc_table[entryId].trapframe->eip = (unsigned int)proc_ptr;
    proc_table[entryId].trapframe->eflags = EF_DEFAULT_VALUE | EF_INTR;
    proc_table[entryId].trapframe->cs = get_cs();
    proc_table[entryId].trapframe->ds = get_ds();
    proc_table[entryId].trapframe->es = get_es();
    proc_table[entryId].trapframe->fs = get_fs();
    proc_table[entryId].trapframe->gs = get_gs();

    // Add the process to the scheduler
    scheduler_add(&proc_table[entryId]);

    //kernel_log_info("Created process %s (%d) entry=%d", proc_name, proc_table[entryId].pid, entryId);

    // Return the process id for the newly created process
    return proc_table[entryId].pid;
}

/**
 * Destroys a process
 * If the process is currently scheduled it must be unscheduled
 * @param proc - process control block
 * @return 0 on success, -1 on error
 */
int kproc_destroy(proc_t *proc) {
    if(proc->pid == 0) {
        kernel_log_error("Cannot destroy idle task!");
        return -1;
    }
    // Remove the process from the scheduler
    scheduler_remove(proc);

    // Clear all data structures associated with the process (proc_stack, proc_table)
    memset(proc->stack, 0, sizeof(PROC_STACK_SIZE));
    memset(proc, 0, sizeof(proc_t));

    // Add the proc table entry back to the process queue (to be recycled)
    if(queue_in(&proc_allocator, proc - proc_table) == -1) {
        kernel_log_error("Unable to deallocate process with pid %d", proc->pid);
    }
    return 0;
}

