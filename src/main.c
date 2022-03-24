/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Main entry point
 */
#include "kernel.h"
#include "keyboard.h"
#include "vga.h"
#include "interrupts.h"
#include "timer.h"
#include "scheduler.h"
#include <spede/string.h>
#include <spede/stdio.h>

void spinner(void) {
    static int spin = 0;
    unsigned char spinner[] = "/-\\|";

    // Print a "spinner" at the top-right corner to help indicate
    // program activity;
    vga_put(VGA_WIDTH-1, 0,
            VGA_COLOR_BLACK, VGA_COLOR_GREEN,
            spinner[spin++ % (sizeof(spinner) - 1)]);
}

void uptime(void) {
    char buf[12] = {0};
    int buflen = 0;

    snprintf(buf, sizeof(buf) - 1, "%d", timer_get_system_time() / 100);
    buflen = strlen(buf);

    for(int i = 0; i < buflen; i++){
        vga_put(VGA_WIDTH - (buflen - i + 2), 0, VGA_COLOR_BLACK, VGA_COLOR_CYAN, buf[i]);
    }

}

/**
 * Operating system main function. Should never return/exit.
 */
int main(void) {
    // Initialize kernel
    kernel_init();
    // Initialize interrupts
    interrupts_init();
    // Initialize timer
    timer_init();
    // Initialize the VGA driver
    vga_init();
    // Initialize the keyboard driver
    keyboard_init();
    // Initialize scheduler
    scheduler_init();
    // Initialize process control
    kproc_init();


    timer_callback_register(&spinner, 10, -1);
    timer_callback_register(&uptime, 100, -1);

    // Test video output
    /*for (int bg = 0; bg <= 0x7; bg++) {
        for (int fg = 0; fg <= 0xf; fg++) {
            vga_put(fg+2, bg+2, bg, fg, '#');
        }
    }*/
    vga_puts("Welcome to TARS!\n");
    vga_puts("Press any key to continue...\n");

    // Wait for a key to be pressed
    keyboard_getc();
    vga_set_xy(0, 12);
    scheduler_run();
    kernel_context_exit(current->trapframe);
    // Should never get here
    return 0;
}
