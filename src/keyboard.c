#include <spede/stdio.h>
#include <spede/machine/io.h>

#include "kernel.h"
#include "keyboard.h"
#include "interrupts.h"
#include "vga.h"

// Keyboard data port
#define KBD_PORT_DATA           0x60

// Keyboard status port
#define KBD_PORT_STAT           0x64

// Keyboard status bits (CTRL, ALT, SHIFT, CAPS, NUMLOCK)
#define KEY_STATUS_CTRL         0x01
#define KEY_STATUS_ALT          0x02
#define KEY_STATUS_SHIFT        0x04
#define KEY_STATUS_CAPS         0x08
#define KEY_STATUS_NUMLOCK      0x10

// Keyboard scancode definitions
#define KEY_CTRL_L              0x1D
#define KEY_CTRL_R              0xE01D

#define KEY_ALT_L               0x38
#define KEY_ALT_R               0xE038

#define KEY_SHIFT_L             0x2A
#define KEY_SHIFT_R             0x36

#define KEY_CAPS                0x3A
#define KEY_NUMLOCK             0x45

// Macros for handling keyboard presses or releases
#define KEY_PRESSED(c)          ((c & 0x80) == 0)
#define KEY_RELEASED(c)         ((c & 0x80) != 0)

// Macros for testing key status combinations
#define KEY_STATUS_ALL(stat, test) ((stat & test) == test)
#define KEY_STATUS_ANY(stat, test) ((stat & test) != 0)

// Bit-map to keep track of CTRL, ALT, SHIFT, CAPS, NUMLOCK
//
// When any of these keys are pressed, the appropriate bit
// should be set. When released, the bit should be cleared.
//   CTRL, ALT, SHIFT
//
// When any of these keys are pressed and then released, the
// appropriate bits should be toggled:
//   CAPS, NUMLOCK
static unsigned int kbd_status = 0x0;

// Primary keymap
// 126 values, some added to pad so we can index correctly
// others added in case we need to implement them later
static const char keyboard_map_primary[] = {
      0,    0,   '1',   '2',   '3',   '4',   '5',   '6',   '7',
    '8',  '9',   '0',   '-',   '=',  '\b',  '\t',   'q',   'w',
    'e',  'r',   't',   'y',   'u',   'i',   'o',   'p',   '[',
    ']', '\n',     0,   'a',   's',   'd',   'f',   'g',   'h',
    'j',  'k',   'l',   ';',  '\'',   '`',     0,  '\\',   'z',
    'x',  'c',   'v',   'b',   'n',   'm',   ',',   '.',   '/',
      0,  '*',     0,   ' ',     0,     0,     0,     0,     0
};

// Secondary keymap (when CAPS ^ SHIFT is enabled)
static const char keyboard_map_secondary[] = {
      0,    0,   '!',   '@',   '#',   '$',   '%',   '^',   '&',
    '*',  '(',   ')',   '_',   '+',  '\b',  '\t',   'Q',   'W',
    'E',  'R',   'T',   'Y',   'U',   'I',   'O',   'P',   '{',
    '}', '\n',     0,   'A',   'S',   'D',   'F',   'G',   'H',
    'J',  'K',   'L',   ':',   '"',   '~',     0,   '|',   'Z',
    'X',  'C',   'V',   'B',   'N',   'M',   '<',   '>',   '?',
      0,  '*',     0,   ' ',     0,     0,     0,     0,     0
};

/**
 * keyboard interrupt request handler that will poll the hardware for data
 * and send it to the screen.
 */
void keyboard_irq_handler() {
    unsigned int c = keyboard_poll();
    if(c != KEY_NULL) {
        vga_putc(c);
    }
}

/**
 * Initializes keyboard data structures and variables
 */
void keyboard_init() {
    kernel_log_info("Initializing keyboard");
    interrupts_irq_register(IRQ_KEYBOARD, isr_entry_keyboard, keyboard_irq_handler);
}

/**
 * Scans for keyboard input and returns the raw character data
 * @return raw character data from the keyboard
 */
unsigned int keyboard_scan(void) {
    return inportb(KBD_PORT_DATA);
}

/**
 * Polls for a keyboard character to be entered.
 *
 * If a keyboard character data is present, will scan and return
 * the decoded keyboard output.
 *
 * @return decoded character or KEY_NULL (0) for any character
 *         that cannot be decoded
 */
unsigned int keyboard_poll(void) {
    //bit 0 indicates data or no data
    unsigned int c = KEY_NULL;
    if(inportb(KBD_PORT_STAT) & 1){
        c = keyboard_decode(keyboard_scan());
    }
    return c;
}

/**
 * Blocks until a keyboard character has been entered
 * @return decoded character entered by the keyboard or KEY_NULL
 *         for any character that cannot be decoded
 */
unsigned int keyboard_getc(void) {
    unsigned int c = KEY_NULL;
    while(c == KEY_NULL){
        c = keyboard_poll();
    }

    return c;
}

/**
 * Processes raw keyboard input and decodes it.
 *
 * Should keep track of the keyboard status for the following keys:
 *   SHIFT, CTRL, ALT, CAPS, NUMLOCK
 *
 * For all other characters, they should be decoded/mapped to ASCII
 * or ASCII-friendly characters.
 *
 * For any character that cannot be mapped, KEY_NULL should be returned.
 */
unsigned int keyboard_decode(unsigned int c) {
    //prefix for right ctrl, right alt, and others
    //we don't care about it
    if(c == 0xe0){
        return KEY_NULL;
    }

    //extract actual key without the bit indicating that it was pressed
    int key = c & 0x7f;

    //switch on the key. Press/release toggle applies to ctrl, alt, and shift.
    //Press toggle applies to numlock and capslock
    switch(key){

        case 0x01:
            return KEY_ESCAPE;

        case KEY_ALT_L:
        case KEY_ALT_R:
            kernel_log_trace("Alt %s", (KEY_PRESSED(c) ? "pressed" : "released"));
            kbd_status = kbd_status ^ KEY_STATUS_ALT;
            return KEY_NULL;

        case KEY_CTRL_L:
        case KEY_CTRL_R:
            kernel_log_trace("Ctrl %s", (KEY_PRESSED(c) ? "pressed" : "released"));
            kbd_status = kbd_status ^ KEY_STATUS_CTRL;
            return KEY_NULL;

        case KEY_SHIFT_L:
        case KEY_SHIFT_R:
            kernel_log_trace("Shift %s", (KEY_PRESSED(c) ? "pressed" : "released"));
            kbd_status = kbd_status ^ KEY_STATUS_SHIFT;
            return KEY_NULL;

        case KEY_CAPS:
            if(KEY_PRESSED(c)){
                if(kbd_status & KEY_STATUS_CAPS){
                    kernel_log_trace("CAPS LOCK OFF");
                    kbd_status = kbd_status ^ KEY_STATUS_CAPS;
                }else{
                    kernel_log_trace("CAPS LOCK ON");
                    kbd_status = kbd_status | KEY_STATUS_CAPS;
                }
            }
            return KEY_NULL;

        case KEY_NUMLOCK:
            if(KEY_PRESSED(c)){
                if(kbd_status & KEY_STATUS_NUMLOCK){
                    kernel_log_trace("NUM LOCK OFF");
                    kbd_status = kbd_status ^ KEY_STATUS_NUMLOCK;
                }else{
                    kernel_log_trace("NUM LOCK ON");
                    kbd_status = kbd_status | KEY_STATUS_NUMLOCK;
                }
            }
            return KEY_NULL;

        default:
            break;
    }

    if(KEY_PRESSED(c)){
        //check if we are shifted OR the caps lock is on
        //and a letter was hit. We can just use the table
        //to quickly check if the scancode corresponds to a-z.
        //The idea is that we don't want to capitalize 1 to a !
        //via caps lock (only shift).
        if(KEY_STATUS_ALL(kbd_status, KEY_STATUS_SHIFT) ^
          (KEY_STATUS_ALL(kbd_status, KEY_STATUS_CAPS) &&
           keyboard_map_primary[c] >= 'a' &&
           keyboard_map_primary[c] <= 'z')){
            //kernel_log_trace("returning 0x%02x char %c", c, c);
            return keyboard_map_secondary[c];
        }else if(kbd_status & KEY_STATUS_CTRL) {
            kernel_debug_command(keyboard_map_primary[c]);
            return KEY_NULL;
        }
        //kernel_log_trace("returning 0x%02x char %c", c, c);
        return keyboard_map_primary[c];
    }
    return KEY_NULL;
}
