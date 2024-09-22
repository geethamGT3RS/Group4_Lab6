#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define STCTRL *((volatile long *)0xE000E010)    // control and status
#define STRELOAD *((volatile long *)0xE000E014)  // reload value
#define STCURRENT *((volatile long *)0xE000E018) // current value

#define COUNT_FLAG (1 << 16) // bit 16 of CSR automatically set to 1
#define ENABLE (1 << 0)      // bit 0 of CSR to enable the timer
#define CLKINT (1 << 2)      // bit 2 of CSR to specify CPU clock
#define INTERRUPT (1 << 1)   // bit 2 of CSR to specify CPU clock
#define CLOCK_MHZ 16

volatile int f = 100000;
volatile int percent = 5;
volatile int x = (8 * 1000000) /100000;  // Fix exponentiation issue
volatile int interrupt_count = 0;

void GPIO_F0_INT_HANDLER(void) {
    x = x + (160000 * percent) / f;  // Fixed exponentiation issue
    x = ((16000000 / f) < x) ? (16000000 / f) : x;
}

void GPIO_F4_INT_HANDLER(void) {
    x = x - (160000 * percent) / f;  // Fixed exponentiation issue
    x = (x < 0) ? 0 : x;
}

void GPIO_INIT(void) {
    SYSCTL_RCGC2_R |= 0x00000020;       /* enable clock to GPIOF */
    GPIO_PORTF_LOCK_R = 0x4C4F434B;     /* unlock commit register */
    GPIO_PORTF_CR_R = 0x1F;             /* make PORTF0 configurable */
    GPIO_PORTF_DEN_R = 0x1F;            /* set PORTF pins 4 pin */
    GPIO_PORTF_DIR_R = 0x0E;            /* set PORTF4 pin as input user switch pin */
    GPIO_PORTF_PUR_R = 0x11;            /* PORTF4 is pulled up */
}

void GPIO_INT_HANDLER(void) {
    int sw1, sw2;
    interrupt_count = interrupt_count + 1;
    int temp_x = 500;
    while (temp_x > 0) {
        temp_x = temp_x - 1;
    }
    sw1 = GPIO_PORTF_DATA_R & 0x10;
    sw2 = GPIO_PORTF_DATA_R & 0x01;
    if (sw1 == 0 && sw2 != 0) {
        GPIO_F4_INT_HANDLER(); // Call GPIO_F4 handler
    }
    if (sw1 != 0 && sw2 == 0) {
        GPIO_F0_INT_HANDLER(); // Call GPIO_F0 handler
    }

    GPIO_PORTF_ICR_R = 0x11; // Clear interrupt flags
}

void Delay(int us) {
    STCURRENT = 0;
    STRELOAD = us;                // Reload value for 'us' microseconds
    STCTRL |= (CLKINT | ENABLE);  // Enable the timer
    while ((STCTRL & COUNT_FLAG) == 0) // Wait until the count flag is set
    {
        ;
    }
    STCTRL = 0; // Disable timer
    return;
}

int main(void) {
    GPIO_INIT();                      // Initialize GPIO
    GPIO_PORTF_IS_R = 0x00;           // Edge-sensitive interrupts
    GPIO_PORTF_IBE_R = 0x00;          // Interrupt on single edge
    GPIO_PORTF_IEV_R = 0x00;          // Falling edge triggers interrupt
    GPIO_PORTF_IM_R = 0x11;           // Unmask interrupts for PF4 and PF0
    NVIC_EN0_R = 0x40000000;          // Enable IRQ30 (GPIO Port F interrupt)

    while (1) {
        if(x!=0){
        GPIO_PORTF_DATA_R |= 0x02;    // Turn on LED connected to PF1
        Delay(x);                     // Delay depending on the value of x
        }
        if(x!=(16000000 / f)){
        GPIO_PORTF_DATA_R &= ~0x02;   // Turn off LED connected to PF1
        Delay((16000000 / f) - x);    // Fixed exponentiation issue
        }
    }
}
