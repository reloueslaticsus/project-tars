/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Interrupt handling functions
 */
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <spede/machine/asmacros.h>

// Maximum number of ISR handlers
#ifndef IRQ_MAX
#define IRQ_MAX      0xf0
#endif

// ISR definitions
#define IRQ_TIMER    0x20      // PIC IRQ 0 (Timer)
#define IRQ_KEYBOARD 0x21      // PIC IRQ 1 (Keyboard)


#ifndef ASSEMBLER

// PIC Definitions
#define PIC1_BASE   0x20            // base address for PIC primary controller
#define PIC2_BASE   0xa0            // base address for PIC secondary controller
#define PIC1_CMD    PIC1_BASE       // address for issuing commands to PIC1
#define PIC1_DATA   (PIC1_BASE+1)   // address for setting data for PIC1
#define PIC2_CMD    PIC2_BASE       // address for issuing commands to PIC2
#define PIC2_DATA   (PIC2_BASE+1)   // address for setting data for PIC2

#define PIC_EOI     0x20            // PIC End-of-Interrupt command

typedef void (*irq_handler_t)(void);

/**
 * General interrupt enablement
 */
void interrupts_init(void);

/**
 * Enable interrupts with the CPU
 */
void interrupts_enable(void);

/**
 * Disable interrupts with the CPU
 */
void interrupts_disable(void);

/**
 * Registers an ISR in the IDT and IRQ handler for processing interrupts
 * @param irq - IRQ number
 * @param entry - function pointer to be registered in the IDT
 * @param handler - function pointer to be called when the specified IRQ occurs
 */
void interrupts_irq_register(int irq, irq_handler_t entry, irq_handler_t handler);

/**
 * Interrupt service routine handler
 * @param irq - IRQ number
 */
void interrupts_irq_handler(int irq);

/**
 * Enables the specified IRQ in the PIC
 * @param irq - IRQ number
 */
void pic_irq_enable(int irq);

/**
 * Disables the specified IRQ in the PIC
 * @param irq - IRQ number
 */
void pic_irq_disable(int irq);

/**
 * Queries if the given IRQ is enabled in the PIC
 * @param irq - IRQ number
 * @return - 1 if enabled, 0 if disabled
 */
int pic_irq_enabled(int irq);

/**
 * Dismisses the specified IRQ in the PIC
 * @param irq - IRQ number
 */
void pic_irq_dismiss(int irq);


__BEGIN_DECLS
/**
 * ISR Function Declarations
 *
 * Source should exist in context.S
 */

/**
 * ISR for Timer IRQ
 * Should be added to IDT to be called when the timer interrupt request
 * (IRQ 0) occurs.
 */
extern void isr_entry_timer();

/**
 * ISR for Keyboard IRQ
 * Should be added to IDT to be called when the keyboard interrupt request
 * (IRQ 1) occurs.
 */
extern void isr_entry_keyboard();

__END_DECLS
#endif
#endif
