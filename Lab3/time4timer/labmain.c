/* labmain.c
   Lab 3 – Assignment 2 (Timer, polling, 1 Hz updates)
   - Uses the Intel Interval Timer mapped at 0x04000020..0x0400003F
   - No delay() calls; we poll the timer TO flag
   - Exact function names preserved: set_leds, set_displays, get_sw, get_btn, labinit, handle_interrupt, main
*/

#include <stdint.h>

/* ===== externs from support files (unchanged) ===== */
extern void print(const char*);
extern void print_dec(unsigned int);
extern void display_string(int, char*);
extern void time2string(char*, int);
extern void tick(int*);
extern int nextprime(int);

/* ===== MMIO: LEDs / Switches / Button / HEX =====
   (addresses match your existing project) */
#define LEDS_ADDR    ((volatile unsigned int*)0x04000000u)
#define SWITCH_ADDR  ((volatile unsigned int*)0x04000010u)   /* SW[9:0] */
#define BTN2_ADDR    ((volatile unsigned int*)0x040000D0u)   /* bit0: BTN2 */

#define HEX_BASE     0x04000050u
#define HEX_STRIDE   0x10u

/* ===== TIMER: Intel Interval Timer (32-bit), 6x16-bit regs, 32-bit spaced =====
   base +0x00: STATUS   (bit0 TO=timeout R/W1C, bit1 RUN RO)
   base +0x04: CONTROL  (bit0 ITO, bit1 CONT, bit2 START (W1), bit3 STOP (W1))
   base +0x08: PERIODL  (low 16 of (period-1))
   base +0x0C: PERIODH  (high 16 of (period-1))
   base +0x10: SNAPL    (not used here)
   base +0x14: SNAPH    (not used here)
*/
#define TIMER_BASE   0x04000020u
#define TMR_STATUS   (*(volatile unsigned int*)(TIMER_BASE + 0x00))
#define TMR_CONTROL  (*(volatile unsigned int*)(TIMER_BASE + 0x04))
#define TMR_PERIODL  (*(volatile unsigned int*)(TIMER_BASE + 0x08))
#define TMR_PERIODH  (*(volatile unsigned int*)(TIMER_BASE + 0x0C))
#define TMR_SNAPL    (*(volatile unsigned int*)(TIMER_BASE + 0x10))
#define TMR_SNAPH    (*(volatile unsigned int*)(TIMER_BASE + 0x14))

/* CONTROL bits */
#define CTRL_ITO     (1u << 0)   /* interrupt enable (unused: we poll) */
#define CTRL_CONT    (1u << 1)   /* continuous mode */
#define CTRL_START   (1u << 2)   /* write-1 pulse to start (self-clears) */
#define CTRL_STOP    (1u << 3)   /* write-1 pulse to stop  (self-clears) */

/* STATUS bits */
#define ST_TO        (1u << 0)   /* timeout flag (R/W1C) */
#define ST_RUN       (1u << 1)   /* running (RO) */

/* ===== globals from template ===== */
int  mytime       = 0x5957;
char textstring[] = "text, more text, and even more text!";
volatile unsigned int timeoutcount = 0;

/* Active-low seven-seg LUT for 0..9 (DP = bit7) */
const int LED_NBR[] = {
    64,  /* 0 */
    121, /* 1 */
    36,  /* 2 */
    48,  /* 3 */
    25,  /* 4 */
    18,  /* 5 */
    2,   /* 6 */
    120, /* 7 */
    0,   /* 8 */
    16   /* 9 */
};

/* ===== (c) Assignment 1 function: LEDs ===== */
void set_leds(int led_mask) {
    *LEDS_ADDR = ((unsigned int)led_mask) & 0x3FFu;     /* 10 LEDs (LSBs) */
}

/* ===== (e) Assignment 1 function: one HEX digit to one display ===== */
void set_displays(int display_number, int value) {
    if (display_number < 0 || display_number > 5) return;  /* HEX0..HEX5 */

    volatile unsigned int *disp =
        (volatile unsigned int *)(HEX_BASE + (unsigned)display_number * HEX_STRIDE);

    unsigned int patt = ((unsigned int)LED_NBR[value & 0xF]) & 0xFFu;
    patt |= 0x80u; /* DP off (active-low) */
    *disp = patt;
}

/* ===== (f) Assignment 1 function: read 10 switches ===== */
int get_sw(void) {
    return (int)((*SWITCH_ADDR) & 0x3FFu);
}

/* ===== (g) Assignment 1 function: read BTN2 (bit0) ===== */
int get_btn(void) {
    return (int)((*BTN2_ADDR) & 0x1u);
}

/* Helper: HH:MM:SS → HEX5..HEX0 (HEX0 is rightmost) */
static void show_time(int h, int m, int s) {
    set_displays(0, s % 10);
    set_displays(1, (s / 10) % 10);
    set_displays(2, m % 10);
    set_displays(3, (m / 10) % 10);
    set_displays(4, h % 10);
    set_displays(5, (h / 10) % 10);
}

/* ===== Assignment 2 (a/b): timer init (100 ms @ 30 MHz), poll TO =====
   IMPORTANT: write CONTROL **once** with (CONT|START); START is a write-1 pulse.
   Also clear any pending TO before starting. */
void labinit(void) {
    const uint32_t period_100ms = 3000000u - 1u; /* 30 MHz * 0.1 s - 1 */

    /* Load (period - 1) split low/high (16-bit each) */
    TMR_PERIODL = (uint16_t)(period_100ms & 0xFFFFu);
    TMR_PERIODH = (uint16_t)((period_100ms >> 16) & 0xFFFFu);

    /* Clear any pending timeout BEFORE starting (TO is R/W1C) */
    TMR_STATUS = ST_TO;

    /* Single write: continuous + start (don’t overwrite CONT with a second write) */
    TMR_CONTROL = (CTRL_CONT | CTRL_START);
}

/* Assignment 2 uses polling; no interrupts handled here */
void handle_interrupt(unsigned cause) { (void)cause; }

/* ===== Assignment 2 (b/c): main loop, polling TO; 10 timeouts → 1 second =====
   - Read inputs every loop (not tied to TO)
   - On TO: ACK, heartbeat LED, timeoutcount++
   - When timeoutcount == 10 (i.e., 1 s), update ASCII time + HH:MM:SS on HEX */
int main(void) {
    labinit();

    int hr = 0, min = 0, sec = 0;
    int heartbeat = 0;

    /* Show something deterministic at power-up on HEX */
    show_time(hr, min, sec);
    set_leds(0);

    while (1) {
        /* Always sample inputs (don’t depend on TO for reads) */
        int sw  = get_sw();
        int btn = get_btn();

        /* Button-driven set (same mapping as A1h):
           SW[9:8] = 01 => seconds, 10 => minutes, 11 => hours
           SW[5:0] new value (0..63), clamped */
        if (btn) {
            int sel = (sw >> 8) & 0x3;
            int val = sw & 0x3F;
            if      (sel == 1) sec = val % 60;
            else if (sel == 2) min = val % 60;
            else if (sel == 3) hr  = val % 100;
            while (get_btn()) { /* crude debounce: wait for release */ }
            show_time(hr, min, sec);
        }

        /* ---- Timer polling ---- */
        if (TMR_STATUS & ST_TO) {        /* timeout occurred? */
            TMR_STATUS = ST_TO;          /* ACK (write-1-to-clear)....
             When the time-out event-flag is a “1”, how does your code reset it to “0”?
By writing a ‘1’ back to the TO bit in the STATUS register*/

            /* 10 Hz heartbeat on LED1 to prove timer is running */
            heartbeat ^= 1;
            set_leds((heartbeat & 1) ? 0x001 : 0x000);

            /* (c) 10 timeouts = 1 second */
            if (++timeoutcount >= 10) {
                timeoutcount = 0;

                /* Terminal text (kept from template) */
                time2string(textstring, mytime);
                display_string(3, textstring);
                tick(&mytime);

                /* HH:MM:SS (software clock) */
                if (++sec >= 60) {
                    sec = 0;
                    if (++min >= 60) {
                        min = 0;
                        if (++hr >= 100) hr = 0;  /* two digits for hours */
                    }
                }
                show_time(hr, min, sec);
            }
        }
    }
}
