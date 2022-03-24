#include <spede/machine/io.h>
#include <spede/stdarg.h>
#include <spede/stdio.h>

#include "kernel.h"
#include "vga.h"

// Current x position (column)
int pos_x = 0;

// Current y position (row)
int pos_y = 0;

// Current background color
int color_bg = VGA_COLOR_BLACK;

// Current foreground color
int color_fg = VGA_COLOR_LIGHT_GREY;

/**
 * Initializes the VGA driver and configuration
 *  - Defaults variables
 *  - Clears the screen
 */
void vga_init(void) {
    kernel_log_info("vga: Initializing VGA");
    color_bg = VGA_COLOR_BLACK;
    color_fg = VGA_COLOR_LIGHT_GREY;
    vga_clear();
}

/**
 * Clears the VGA output
 */
void vga_clear(void) {
    unsigned short* vga_buf = VGA_BASE;
    //kernel_log_trace("vga: Clearing screen");
    for(int i = 0; i <= VGA_WIDTH * VGA_HEIGHT; i++){
        vga_buf[i] = VGA_CHAR(color_bg, color_fg, 0x00);
    }

    pos_x = 0;
    pos_y = 0;
}

/**
 * Sets the current X/Y (column/row) position
 *
 * @param x - x position (0 to VGA_WIDTH-1)
 * @param y - y position (0 to VGA_HEIGHT-1)
 * @notes If the input parameters exceed the valid range, the position
 *        will be set to the range boundary (min or max)
 */
void vga_set_xy(int x, int y) {
    pos_x = (x < 0 ? 0 : (x >= VGA_WIDTH ? VGA_WIDTH - 1 : x));
    pos_y = (y < 0 ? 0 : (y >= VGA_HEIGHT ? VGA_HEIGHT - 1 : y));
}

/**
 * Sets the background color.
 *
 * Does not modify any existing background colors, only sets it for
 * new operations.
 *
 * @param bg - background color
 */
void vga_set_bg(int bg) {
    //kernel_log_trace("vga: Setting background color to 0x%02x", bg);
    if(bg >= 0 && bg <= 0xF) {
        color_bg = bg;
    }
}

/**
 * Sets the foreground/text color.
 *
 * Does not modify any existing foreground colors, only sets it for
 * new operations.
 *
 * @param color - background color
 */
void vga_set_fg(int fg) {
    //kernel_log_trace("vga: Setting foreground color to 0x%02x", fg);
    if(fg >= 0 && fg <= 0xF) {
        color_fg = fg;
    }
}

/**
 * Prints the character on the screen.
 *
 * Does not change the x/y position, simply sets the character
 * at the current x/y position using existing background and foreground
 * colors.
 *
 * @param c - Character to print
 */
void vga_set_c(char c) {
    //kernel_log_trace("vga: Setting character to '%c'", c);
    unsigned short* vga_buf = VGA_BASE;
    int offset = (pos_y * VGA_WIDTH) + pos_x;
    vga_buf[offset] = VGA_CHAR(color_bg, color_fg, c);
}

/**
 * Prints a character on the screen at the specified x/y position and
 * with the specified background/foreground colors
 *
 * @param x - x position (0 to VGA_WIDTH-1)
 * @param y - y position (0 to VGA_HEIGHT-1)
 * @param bg - background color
 * @param fg - foreground color
 * @param c - character to print
 */
void vga_put(int x, int y, int bg, int fg, char c) {
    //kernel_log_trace("vga: Printing '%c' at (%d, %d) with bg=0x%02x, fg=0x%02x", c, x, y, bg, fg);
    unsigned short* vga_buf = VGA_BASE;
    x = (x < 0 ? 0 : (x >= VGA_WIDTH ? VGA_WIDTH - 1 : x));
    y = (y < 0 ? 0 : (y >= VGA_HEIGHT ? VGA_HEIGHT - 1 : y));
    bg = bg & 0x7;
    fg = fg & 0xF;
    int offset = (y * VGA_WIDTH) + x;
    vga_buf[offset] = VGA_CHAR(bg, fg, c);
}

void scroll() {
    //reset to column zero and check if we were on the last line
    unsigned short* vga_buf = VGA_BASE;
    pos_x = 0;
    if(pos_y + 1 == VGA_HEIGHT){
        int last_row_start_pos = VGA_WIDTH * (VGA_HEIGHT - 1);

        //move screen up and keep pos_y the same
        //swap in whatever is 1 run length away until we hit the last row
        for(int i = 0; i < last_row_start_pos; i++){
            vga_buf[i] = vga_buf[VGA_WIDTH + i];
        }

        //delete last line (could be done in the first loop but we put it here to make things neater)
        for(int i = 0; i < VGA_WIDTH; i++){
            vga_buf[last_row_start_pos + i] = VGA_CHAR(color_bg, color_fg, 0x00);
        }

    }else{
        //safe to move down one line
        pos_y++;
    }
}

/**
 * Prints a character on the screen.
 *
 * When a character is printed, will do the following:
 *  - Update the x and y positions
 *  - If needed, will wrap from the end of the current line to the
 *    start of the next line
 *  - If the last line is reached, will ensure that all text is
 *    scrolled up
 *  - Special characters are handled as such:
 *    - tab character (\t) prints 'tab_stop' spaces
 *    - backspace (\b) character moves the character back one position,
 *      prints a space, and then moves back one position again
 *
 * @param c - character to print
 */
void vga_putc(char c) {

    if(c == '\b'){
        //check bounds and decrement x/y position if possible
        if(pos_x == 0){
            pos_x = VGA_WIDTH - 1;
            if(pos_y != 0){
                pos_y--;
            }
        }else{
            pos_x--;
        }

        //remove char (we could linearly shift everything back that is ahead of this char in the future)
        vga_put(pos_x, pos_y, color_bg, color_fg, 0x00);

    }else if(c == '\n'){
        //jump to next line or scroll the text
        scroll();
    }else if(c == '\t'){
        //insert 4 spaces
        for(int i = 0; i < 4; i++){
            vga_put(pos_x, pos_y, color_bg, color_fg, 0x00);

            if(pos_x + 1 == VGA_WIDTH){
                scroll();
            }else{
                pos_x++;
            }
        }
    }else if(c == '\r'){
        //nothing to do but set cursor to start of line
        pos_x = 0;
    }else{
        //utilize existing function and scroll if we need to
        //kernel_log_trace("printing %c", c);
        vga_put(pos_x, pos_y, color_bg, color_fg, c);
        if(pos_x + 1 == VGA_WIDTH){
            scroll();
        }else{
            pos_x++;
        }
    }
}

/**
 * Prints a string on the screen.
 *
 * @param s - string to print
 */
void vga_puts(char *s) {
    if(!s) {
        return;
    }

    for(int i = 0; s[i] != '\0'; i++){
        vga_putc(s[i]);
    }
}

