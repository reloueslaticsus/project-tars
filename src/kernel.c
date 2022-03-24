
/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Kernel functions
 */
#include <spede/flames.h>
#include <spede/stdarg.h>
#include <spede/stdio.h>

#include "kernel.h"
#include "interrupts.h"
#include "vga.h"
#include "scheduler.h"
#include "user_prog.h"
// Current log level
int kernel_log_level;
proc_t* current = NULL;

/**
 * Initializes any kernel internal data structures and variables
 */
void kernel_init() {
    // Set the default log level
    kernel_log_level = KERNEL_LOG_LEVEL_TRACE;
    // Display a welcome message on the host
    kernel_log_info("Welcome to TARS!");
}

/**
 * Prints a kernel log message to the host with an error log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_error(char *msg, ...) {
    if(kernel_log_level < KERNEL_LOG_LEVEL_ERROR){
        return;
    }

    va_list args;
    printf("error: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a warning log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_warn(char *msg, ...) {
    if(kernel_log_level < KERNEL_LOG_LEVEL_WARN){
        return;
    }

    va_list args;
    printf("warning: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with an info log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_info(char *msg, ...) {
    // Return if our log level is less than info
    if (kernel_log_level < KERNEL_LOG_LEVEL_INFO) {
        return;
    }

    // Obtain the list of variable arguments
    va_list args;

    // Indicate this is an 'info' type of message
    printf("info: ");

    // Pass the message variable arguments to vprintf
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a debug log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_debug(char *msg, ...) {
    if(kernel_log_level < KERNEL_LOG_LEVEL_DEBUG){
        return;
    }

    va_list args;
    printf("debug: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Prints a kernel log message to the host with a trace log level
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_log_trace(char *msg, ...) {
    if(kernel_log_level < KERNEL_LOG_LEVEL_TRACE){
        return;
    }

    va_list args;
    printf("trace: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
}

/**
 * Triggers a kernel panic that does the following:
 *   - Displays a panic message on the host console
 *   - Triggers a breakpiont (if running through GDB)
 *   - aborts/exits the operating system program
 *
 * @param msg - string format for the message to be displayed
 * @param ... - variable arguments to pass in to the string format
 */
void kernel_panic(char *msg, ...) {

    va_list args;
    printf("panic: ");

    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);

    printf("\n");
    breakpoint();
    exit(1);
}

/**
 * Special function to trigger kernel functionality
 * @param cmd - single character/byte for the associated
 * command
 */
void kernel_debug_command(unsigned char cmd) {
    int pid;
    int error;
    switch(cmd) {
        case 'b':
            kernel_log_trace("breakpoint set");
            breakpoint();
            break;
        case 'p':
            pid = kproc_create(&user_test, "Test", PROC_TYPE_USER);
            if(pid != -1) {
                kernel_log_trace("process %d created", pid);
            }
            break;
        case 'x':
            error = kproc_destroy(current);
            if(error != -1) {
                kernel_log_trace("process destroyed");
            }
            break;
        case '-':
            kernel_log_level--;
            switch(kernel_log_level) {
                case 0:
                    //kernel log none
                    break;
                case 1:
                    //kernel log error
                    kernel_log_error("LOG LEVEL SET TO ERROR!");
                    break;
                case 2:
                    //kernel log warn
                    kernel_log_warn("LOG LEVEL SET TO WARN!");
                    break;
                case 3:
                    //kernel log info
                    kernel_log_info("LOG LEVEL SET TO INFO!");
                    break;
                case 4:
                    //kernel log debug
                    kernel_log_debug("LOG LEVEL SET TO DEBUG!");
                    break;
                case 5:
                    //kernel log trace
                    kernel_log_trace("LOG LEVEL SET TO TRACE!");
                    break;
                default:
                    //anything less than 0 will be handled below
                    break;

            }
            if(kernel_log_level < KERNEL_LOG_LEVEL_NONE){
                kernel_log_level = KERNEL_LOG_LEVEL_NONE;
            }
            break;
        case '=':
            kernel_log_level++;
            switch(kernel_log_level) {
                case 1:
                    //kernel log error
                    kernel_log_error("LOG LEVEL SET TO ERROR!");
                    break;
                case 2:
                    //kernel log warn
                    kernel_log_warn("LOG LEVEL SET TO WARN!");
                    break;
                case 3:
                    //kernel log info
                    kernel_log_info("LOG LEVEL SET TO INFO!");
                    break;
                case 4:
                    //kernel log debug
                    kernel_log_debug("LOG LEVEL SET TO DEBUG!");
                    break;
                case 5:
                    //kernel log trace
                    kernel_log_trace("LOG LEVEL SET TO TRACE!");
                    break;
                case 6:
                    //kernel log all
                    kernel_log_trace("LOG LEVEL SET TO ALL!");
                    break;
                default:
                    //anything greater than 6 will be handled below
                    kernel_log_trace("LOG LEVEL SET TO ALL!");
                    break;

            }
            if(kernel_log_level > KERNEL_LOG_LEVEL_ALL){
                kernel_log_level = KERNEL_LOG_LEVEL_ALL;
            }
            break;
        case 'q':
            kernel_log_trace("exiting kernel");
            exit(0);
            break;

        default:
            kernel_log_trace("invalid kernel debug command: ctrl+%c", cmd);
            break;
    }
}

/**
 * Kernel Idle
 * Runs infinitely, ensures interrupts are enabled and the CPU is halted
 */
void kernel_idle(void) {
    kernel_log_trace("kernel idle task");
    while(1) {
        interrupts_enable();
        asm("hlt");
    }
}

/**
 * Kernel entrypoint
 *
 * This is the primary entry point for the kernel. It is only
 * entered when an interrupt occurs. When it is entered, the
 * process context must be saved. Any kernel processing (such as
 * interrupt handling) is performed before finally exiting the
 * kernel context to restore the proces state.
 */
void kernel_context_enter(trapframe_t *trapframe) {
    current->trapframe = trapframe;
    interrupts_irq_handler(trapframe->interrupt);
    scheduler_run();
    kernel_context_exit(current->trapframe);
}
