/* labmain.c
   Lab 3 – Assignment 1 (c–h)
*/

/* External functions from other files */
extern void print(const char*);
extern void print_dec(unsigned int);
extern void display_string(int, char*);
extern void time2string(char*, int);
extern void tick(int*);
extern void delay(int);
extern int nextprime(int);

#include <stdint.h>

/* --------------------------------------------------
   HEX display memory-mapped base and stride
   HEX0 base = 0x04000050, each next display +0x10 (16 bytes away)
-------------------------------------------------- */
#define HEX_BASE   0x04000050u
#define HEX_STRIDE 0x10u

/* --------------------------------------------------
   Globals
-------------------------------------------------- */
int mytime = 0x5957; /*just a starting value for a text clock (used by time2string/tick).*/
int led_val = 0;
char textstring[] = "text, more text, and even more text!";

/* Active-low 7-seg patterns for digits 0..9 (DP is bit7) */
const int LED_NBR[] = {
    64,   /* 0 */
    121,  /* 1 */
    36,   /* 2 */
    48,   /* 3 */
    25,   /* 4 */
    18,   /* 5 */
    2,    /* 6 */
    120,  /* 7 */
    0,    /* 8 */
    16    /* 9 */
};

/* --------------------------------------------------
   Assignment 1 I/O functions
-------------------------------------------------- */

/* (c) Write to the 10 LEDs (active-high), LSB = LED1 */
void set_leds(int led_mask) {
    volatile unsigned int *leds = (volatile unsigned int *)0x04000000u;
    *leds = (unsigned int)led_mask & 0x3FFu;   /* only 10 LEDs */
}
/*makes a pointer to led register, and mask for 10 bits cuz only lowest 10 bits 
control LEDs others stay 0*/
/* (e) Write one hex digit to one 7-seg display (HEX0..HEX5).
   DP is OFF by default (active-low -> bit7=1). */
void set_displays(int display_number, int value) {
    if (display_number < 0 || display_number > 5) return; /* HEX0..HEX5 */

    volatile unsigned int *disp =
        (volatile unsigned int *)(HEX_BASE + (unsigned)display_number * HEX_STRIDE);
/*Computes the address of HEX*/
    unsigned int patt = ((unsigned int)LED_NBR[value & 0xF]) & 0xFFu;
    /*Looks up the active-low pattern for the decimal digit value & 0xF (0..9).*/
    patt |= 0x80u; /* ensure decimal point is OFF */

    *disp = patt;
}
/*Writes the 8-bit pattern to that display.*/

/* (f) Read 10 toggle switches from 0x04000010 (LSBs SW1..SW10) */
int get_sw(void) {
    volatile unsigned int *sw = (volatile unsigned int *)0x04000010u;
    return (int)(*sw & 0x3FFu); /* only 10 LSBs */
}

/* (g) Read second push-button from 0x040000D0 (return in bit0 only --
whether BTN2 is pressed) */
int get_btn(void) {
    volatile unsigned int *btn = (volatile unsigned int *)0x040000D0u;
    return (int)(*btn & 0x1u);
}

/* Interrupt handler (unused in Assignment 1) */
void handle_interrupt(unsigned cause) {}

/* Lab init (unused in Assignment 1) */
void labinit(void) {}

/* Helper: show HH:MM:SS across HEX5..HEX0
   HEX5 HEX4   HEX3 HEX2   HEX1 HEX0
     H   H       M   M       S   S   (HEX0 is rightmost) */
static void show_time(int hours, int minutes, int seconds) {
    /* seconds */
    set_displays(0, seconds % 10);
    set_displays(1, (seconds / 10) % 10);
    /* minutes */
    set_displays(2, minutes % 10);
    set_displays(3, (minutes / 10) % 10);
    /* hours */
    set_displays(4, hours % 10);
    set_displays(5, (hours / 10) % 10);
}

/* --------------------------------------------------
   Main program – Assignment 1 (d & h)
-------------------------------------------------- */
int main(void) {
    labinit();

    /* ---- (d) Start sequence on LEDs ----
       Increment the value shown on the first 4 LEDs once per "second"
       and stop this start phase when all 4 are ON (value == 0xF). */
    set_leds(0);               /* start at 0 */
    for (int n = 0; n < 16; ++n) {
        set_leds(n);           /* show n on the lowest 4 LEDs */
        delay(600);            /* ~1s coarse delay; tune if needed */
    }

    /* ---- (h) Clock loop with button/switch updates ---- */
    int sec = 0, min = 0, hr = 0;

    while (1) {
        delay(600); /* ~1 second */

        /* Update terminal string (optional) */
        time2string(textstring, mytime);
        display_string(3, textstring);
        tick(&mytime);

        /* Update software clock */
        if (++sec >= 60) {
            sec = 0;
            if (++min >= 60) {
                min = 0;
                if (++hr >= 100) hr = 0; /* 2-digit hour wrap */
            }
        }

        /* Show HH:MM:SS on HEX5..HEX0 */
        show_time(hr, min, sec);

        /* If button pressed, read switches and update fields */
        if (get_btn()) {
            int sw = get_sw();

            /* Use SW7 (bit6) as "exit" */
            if ((sw >> 6) & 1) {
                break;
            }

            /* Top two switches (bits 9..8) pick the field */
            int sel = (sw >> 8) & 0x3;
            int val = sw & 0x3F; /* 6-bit new value */

            if (sel == 1) {           /* 01 -> seconds */
                sec = val % 60;
            } else if (sel == 2) {    /* 10 -> minutes */
                min = val % 60;
            } else if (sel == 3) {    /* 11 -> hours */
                hr = val % 100;
            }
            show_time(hr, min, sec);

            /* crude debounce: wait for release */
            while (get_btn()) { /* spin */ }
        }
    }

    return 0;
}
