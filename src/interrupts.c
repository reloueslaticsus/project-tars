/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 * Spring 2022
 *
 * Interrupt handling functions
 */

#include <spede/machine/io.h>           // inportb, outportb
#include <spede/machine/proc_reg.h>     // get_id_base,
#include <spede/machine/seg.h>          // get_cs, fill_gate
#include <spede/string.h>               // memset

#include "kernel.h"
#include "interrupts.h"

// Interrupt descriptor table
struct i386_gate *idt = NULL;

// Interrupt handler table
// Contains an array of function pointers associated with
// the various interrupts to be handled
irq_handler_t irq_handlers[IRQ_MAX];

/**
 * Interrupt initialization
 */
void interrupts_init() {
    kernel_log_info("Initializing Interrupts");

    // Obtain the IDT base address
    idt = get_idt_base();

    // Initialize the IRQ handlers table
    memset(idt, IRQ_MAX, 0);
}

/**
 * Enable interrupts with the CPU
 */
void interrupts_enable(void) {
    asm("sti");
}

/**
 * Disable interrupts with the CPU
 */
void interrupts_disable(void) {
    asm("cli");
}

/**
 * Handles the specified interrupt by dispatching to the registered function
 * @param interrupt - interrupt number
 */
void interrupts_irq_handler(int irq) {
    if(irq_handlers[irq]) {
        irq_handlers[irq]();
    } else {
        kernel_panic("No callback registered for IRQ %d", irq);
    }

    if(irq >= 0x20 && irq <= 0x2F) {
        pic_irq_dismiss(irq - 0x20);
    }
}

/*
 * Registers the appropriate IDT entry and handler function for the
 * specified interrupt.
 *
 * @param interrupt - interrupt number
 * @param entry - the function to run when the interrupt occurs
 * @param handler - the function to be called to process the the interrupt
 */
void interrupts_irq_register(int irq, irq_handler_t entry, irq_handler_t handler) {

    if(!entry) {
        kernel_panic("Invalid IDT entry sent for registration!");
    }
    if(!handler) {
        kernel_panic("Invalid IRQ callback sent for registration!");
    }

    fill_gate(&idt[irq], (int)entry, get_cs(), ACC_INTR_GATE, 0);

    irq_handlers[irq] = handler;

    if(irq >= 0x20 && irq <= 0x2F) {
        pic_irq_enable(irq - 0x20);
    } else {
        kernel_log_error("Failed to enable IRQ");
    }
}

/**
 * Enables the specified IRQ on the PIC
 *
 * @param irq - IRQ that should be enabled
 * @note IRQs > 0xf will be remapped
 */
void pic_irq_enable(int irq) {
    if(!(irq >= 0x0 && irq <= 0xF) || pic_irq_enabled(irq)) {
        kernel_log_error("Invalid irq to be enabled!");
        return;
    }

    int data;
    if(irq >= 0x0 && irq <= 0x7) {
        data = inportb(PIC1_DATA);
        data &= ~(1 << irq);
        outportb(PIC1_DATA, data);
    } else if(irq >= 0x8 && irq <= 0xF) {
        data = inportb(PIC2_DATA);
        data &= ~(1 << irq);
        outportb(PIC2_DATA, data);
    }
}

/**
 * Disables the specified IRQ via the PIC
 *
 * @param irq - IRQ that should be disabled
 */
void pic_irq_disable(int irq) {

    if(!(irq >= 0x0 && irq <= 0xF) || !pic_irq_enabled(irq)) {
        kernel_log_error("Invalid irq to be disabled");
        return;
    }

    int data;
    if(irq >= 0x0 && irq <= 0x7) {
        data = inportb(PIC1_DATA);
        data |= (1 << irq);
        outportb(PIC1_DATA, data);
    }else if(irq >= 0x8 && irq <= 0xF) {
        data = inportb(PIC2_DATA);
        data |= (1 << irq);
        outportb(PIC2_DATA, data);
    }
}

/**
 * Queries if the given IRQ is enabled on the PIC
 *
 * @param irq - IRQ to check
 * @return - 1 if enabled, 0 if disabled
 */
int pic_irq_enabled(int irq) {
    int mask = (1 << irq);

    if(irq >= 0x0 && irq <= 0x7) {
        return ((inportb(PIC1_DATA) & mask) > 0 ? 0 : 1);
    } else if(irq >= 0x8 && irq <= 0xF) {
        return ((inportb(PIC2_DATA) & mask) > 0 ? 0 : 1);
    }

    kernel_log_error("Invalid irq enable check");
    return 0;
}

/**
 * Dismisses an interrupt by sending the EOI command to the appropriate
 * PIC device(s). If the IRQ is assosciated with the secondary PIC, the
 * EOI command must be issued to both since the PICs are dasiy-chained.
 *
 * @param irq - IRQ to be dismissed
 */
void pic_irq_dismiss(int irq) {
    if(irq >= 0x0 && irq <= 0x7) {
        outportb(PIC1_CMD, PIC_EOI);
    } else if(irq >= 0x8 && irq <= 0xF) {
        outportb(PIC1_CMD, PIC_EOI);
        outportb(PIC2_CMD, PIC_EOI);
    }
}

