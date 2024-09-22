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
volatile uint32_t timerValue = 0;
volatile int timerRunning = 0;           // Flag to check if the timer is running
volatile int f = 100000;                 // Frequency in Hz
volatile int percent = 5;                // Step size for duty cycle adjustment
volatile int x = (8 * 1000000) / 100000; // Initial duty cycle
volatile int interrupt_count = 0;

void GPIO_F0_INT_HANDLER(void)
{
    x = x + (160000 * percent) / f; // Increase x
    // Ensure x doesn't exceed the maximum value for a valid duty cycle
    x = ((16000000 / f) <= x) ? (16000000 / f) : x;
}

void GPIO_F4_INT_HANDLER(void)
{
    x = x - (160000 * percent) / f; // Decrease x
    // Ensure x doesn't go below 1
    x = (x <= 0) ? 0 : x;
}

void GPIO_INIT(void)
{
    SYSCTL_RCGC2_R |= 0x00000020;   /* enable clock to GPIOF */
    GPIO_PORTF_LOCK_R = 0x4C4F434B; /* unlock commit register */
    GPIO_PORTF_CR_R = 0x1F;         /* make PORTF0 configurable */
    GPIO_PORTF_DEN_R = 0x1F;        /* set PORTF pins 4 pin */
    GPIO_PORTF_DIR_R = 0x0E;        /* set PORTF4 pin as input user switch pin */
    GPIO_PORTF_PUR_R = 0x11;        /* PORTF4 is pulled up */
}

void Timer0A_Init(void)
{
    SYSCTL_RCGCTIMER_R |= 0x01;  // Enable clock for Timer0
    TIMER0_CTL_R &= ~0x01;       // Disable Timer0A during configuration
    TIMER0_CFG_R = 0x00;         // Configure for 32-bit timer mode (concatenated)
    TIMER0_TAMR_R = 0x12;        // TAMR: Up-counting, periodic mode
    TIMER0_TAILR_R = 0xFFFFFFFF; // Set interval load to max (for long counting)
    TIMER0_ICR_R = 0x01;         // Clear the timeout flag
    TIMER0_IMR_R &= ~0x01;       // Disable timer interrupts (not needed for this case)
}

void GPIO_INTERRUPT_HANDLER(void)
{
    GPIO_PORTF_ICR_R = 0x10; // Clear interrupt flags
    interrupt_count = interrupt_count + 1;
    int temp_x = 5;
    while (temp_x > 0)
    {
        temp_x = temp_x - 1;
    }
    if (timerRunning == 0)
    {
        // Start Timer
        TIMER0_CTL_R |= 0x01; // Enable Timer0A
        timerRunning = 1;
    }
    else
    {
        // Stop Timer
        TIMER0_CTL_R &= ~0x01;     // Disable Timer0A
        timerValue = TIMER0_TAR_R; // Read current timer value
        TIMER0_TAV_R = 0;          // Reset Timer0A counter to 0
        timerRunning = 0;
        if (timerValue < 6500000)
        {
            GPIO_F0_INT_HANDLER(); // Call GPIO_F0 handler
        }
        else
        {
            GPIO_F4_INT_HANDLER(); // Call GPIO_F4 handler
        }
    }
}

void Delay(int us)
{
    STCURRENT = 0;
    STRELOAD = us;                     // Reload value for 'us' microseconds
    STCTRL |= (CLKINT | ENABLE);       // Enable the timer
    while ((STCTRL & COUNT_FLAG) == 0) // Wait until the count flag is set
    {
        ;
    }
    STCTRL = 0; // Disable timer
    return;
}

int main(void)
{
    GPIO_INIT(); // Initialize GPIO
    Timer0A_Init();
    GPIO_PORTF_IS_R = 0x00;  // Edge-sensitive interrupts
    GPIO_PORTF_IBE_R = 0x10; // Interrupt on single edge
    GPIO_PORTF_IM_R = 0x10;  // Unmask interrupts for PF4 and PF0
    NVIC_EN0_R = 0x40000000; // Enable IRQ30 (GPIO Port F interrupt)

    while (1)
    {

        if (x != 0)
        {
            GPIO_PORTF_DATA_R |= 0x02; // Turn on LED connected to PF1
            Delay(x);                  // Delay depending on the value of x
        }
        if (x != (16000000 / f))
        {
            GPIO_PORTF_DATA_R &= ~0x02; // Turn off LED connected to PF1
            Delay((16000000 / f) - x);  // Delay for the remaining time
        }
    }
}