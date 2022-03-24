/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * User "programs"
 */

#include "user_prog.h"
#include "vga.h"

/**
 * User test program
 * This "program" will test functionality that
 * our operating system provides
 */
void user_test(void) {
    vga_puts("Test process is running...\n");
    while(1) {
        asm("hlt");
    }
}
