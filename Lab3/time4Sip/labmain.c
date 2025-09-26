/* labmain.c  — primes + timer interrupt updates to HEX display */

#include <stdint.h>

/* ===== externs (provided) ===== */
extern void print(const char*);
extern void print_dec(unsigned int);
extern void display_string(int, char*);
extern void time2string(char*, int);
extern void tick(int*);
extern int nextprime(int);
extern void enable_interrupt(void);
extern void tick(int* t);
extern int  mytime;
extern volatile int timeoutcount;



/* ===== MMIO (same addresses you used) ===== */
#define LEDS_ADDR    ((volatile unsigned int*)0x04000000u)
#define SWITCH_ADDR  ((volatile unsigned int*)0x04000010u)   /* SW[9:0] */
#define BTN2_ADDR    ((volatile unsigned int*)0x040000D0u)   /* bit0: BTN2 */
#define HEX_BASE     0x04000050u
#define HEX_STRIDE   0x10u

#define SW_BASE   0x04000010u
#define SW_DATA   (*(volatile unsigned int*)(SW_BASE + 0x00))
#define SW_DIR    (*(volatile unsigned int*)(SW_BASE + 0x04))
#define SW_IMASK  (*(volatile unsigned int*)(SW_BASE + 0x08))
#define SW_ECAP   (*(volatile unsigned int*)(SW_BASE + 0x0C))


/* ===== TIMER (Intel Interval Timer) ===== */
#define TIMER_BASE   0x04000020u
#define TMR_STATUS   (*(volatile unsigned int*)(TIMER_BASE + 0x00))
#define TMR_CONTROL  (*(volatile unsigned int*)(TIMER_BASE + 0x04))
#define TMR_PERIODL  (*(volatile unsigned int*)(TIMER_BASE + 0x08))
#define TMR_PERIODH  (*(volatile unsigned int*)(TIMER_BASE + 0x0C))
#define CTRL_ITO     (1u << 0)
#define CTRL_CONT    (1u << 1)
#define CTRL_START   (1u << 2)
#define ST_TO        (1u << 0)
#define TIMER_CLK_HZ 30000000u        // dtekv interval timer clock (adjust if needed)
#define IRQ_RATE_HZ  10u  

/* ===== globals from template ===== */
int  mytime       = 0x0000;                     /* MM:SS in BCD-like nibbles */
char textstring[] = "text, more text, and even more text!";
/* count timer interrupts; do work every 10th */
volatile int timeoutcount = 0;
volatile unsigned sw3_is_high = 0;   // 1 while SW3 is ON

/* (b) add prime */
int prime = 1234567;

/* 7-segment digit patterns (active-low) for 0..9 */
static const unsigned char LED_NBR[10] =
  { 64,121,36,48,25,18,2,120,0,16 };

/* one HEX digit → one display (HEX0..HEX5) */
static inline void set_displays(int display_number, int value) {
    if (display_number < 0 || display_number > 5) return;
    volatile unsigned int *disp =
        (volatile unsigned int *)(HEX_BASE + (unsigned)display_number * HEX_STRIDE);
    unsigned int patt = (unsigned)LED_NBR[value & 0xF] | 0x80u;  /* DP off */
    *disp = patt;
}
static inline void clear_display(int display_number){
    if (display_number < 0 || display_number > 5) return;
    volatile unsigned int *disp =
        (volatile unsigned int *)(HEX_BASE + (unsigned)display_number * HEX_STRIDE);
    *disp = 0xFFu; /* all segments off (active-low) */
}

/* init timer for 1 Hz periodic interrupts @ 30 MHz clock */
void labinit(void) {
    /* --- Switch block: inputs + clear edges + enable IRQ for SW[3] --- */
    SW_DIR   = 0x00000000u;        // inputs
    SW_ECAP  = 0xFFFFFFFFu;        // clear any latched edges
    SW_IMASK = (1u << 3);          // ONLY Switch #3 raises interrupts

    /* --- Timer setup (unchanged) --- */
    const uint32_t period = (TIMER_CLK_HZ / IRQ_RATE_HZ) - 1u;
    TMR_CONTROL = 0;
    TMR_STATUS  = 0;
    TMR_PERIODL = (uint16_t)(period & 0xFFFFu);
    TMR_PERIODH = (uint16_t)(period >> 16);
    TMR_CONTROL = (CTRL_ITO | CTRL_CONT | CTRL_START);

    /* --- finally enable global/external interrupts --- */
    enable_interrupt();
}
static inline void show_time_on_hex(void) {
    /* mytime format: [15:12]=M10, [11:8]=M1, [7:4]=S10, [3:0]=S1 */
    int t   = mytime;
    int m10 = (t >> 12) & 0xF;
    int m1  = (t >> 8)  & 0xF;
    int s10 = (t >> 4)  & 0xF;
    int s1  =  t        & 0xF;

    /* HEX index 0 = rightmost digit on board */
    set_displays(0, s1);
    set_displays(1, s10);
    set_displays(2, m1);
    set_displays(3, m10);
    clear_display(4);
    clear_display(5);
}




void handle_interrupt(unsigned cause) {

    if (cause == 16u) {
        if (TMR_STATUS & ST_TO) {
            TMR_STATUS = 0;  // ack

            // your 10→1 Hz divider for the clock (optional)
            if (++timeoutcount >= 10) {
                timeoutcount = 0;
                tick(&mytime);
                show_time_on_hex();
            }

            // print based on switch-held state
            if (sw3_is_high) {
                print_dec(17);
            } else {
                print_dec(16);
            }
            print("\n");
        }
    }


    if (cause == 17u) {
        unsigned edge = SW_ECAP;
        if (edge & (1u << 3)) {
            SW_ECAP = (1u << 3);                 // ack edge
            sw3_is_high = (SW_DATA >> 3) & 1u;   // read current level

            if (sw3_is_high) { tick(&mytime); tick(&mytime); show_time_on_hex(); }
        }
    }
}

/* (c) new main: print primes forever */
int main(void) {
    labinit();

    while (1) {
        print("Prime: ");
        prime = nextprime(prime);
        print_dec((unsigned)prime);
        print("\n");
    }
}