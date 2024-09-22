#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define STCTRL *((volatile long *)0xE000E010)
#define STRELOAD *((volatile long *)0xE000E014)
#define STCURRENT *((volatile long *)0xE000E018)

#define COUNT_FLAG (1 << 16)
#define ENABLE (1 << 0)
#define CLKINT (1 << 2)
#define CLOCK_MHZ 16

volatile uint32_t timerValue = 0;
volatile int timerRunning = 0;
volatile int f = 100000;
volatile int percent = 5;
volatile int x = (8 * 1000000) / 100000;
volatile int interrupt_count = 0;

void GPIO_F0_INT_HANDLER(void)
{
    x += (160000 * percent) / f;
    x = ((16000000 / f) <= x) ? (16000000 / f) : x;
}

void GPIO_F4_INT_HANDLER(void)
{
    x -= (160000 * percent) / f;
    x = (x <= 0) ? 0 : x;
}

void GPIO_INIT(void)
{
    SYSCTL_RCGC2_R |= 0x20;
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_DEN_R = 0x1F;
    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTF_PUR_R = 0x11;
}

void Timer0A_Init(void)
{
    SYSCTL_RCGCTIMER_R |= 0x01;
    TIMER0_CTL_R &= ~0x01;
    TIMER0_CFG_R = 0x00;
    TIMER0_TAMR_R = 0x12;
    TIMER0_TAILR_R = 0xFFFFFFFF;
    TIMER0_ICR_R = 0x01;
    TIMER0_IMR_R &= ~0x01;
}

void GPIO_INTERRUPT_HANDLER(void)
{
    GPIO_PORTF_ICR_R = 0x10;
    interrupt_count++;
    int temp_x = 5;
    while (temp_x-- > 0)
        ;
    if (timerRunning == 0)
    {
        TIMER0_CTL_R |= 0x01;
        timerRunning = 1;
    }
    else
    {
        TIMER0_CTL_R &= ~0x01;
        timerValue = TIMER0_TAR_R;
        TIMER0_TAV_R = 0;
        timerRunning = 0;
        if (timerValue < 6500000)
            GPIO_F0_INT_HANDLER();
        else
            GPIO_F4_INT_HANDLER();
    }
}

void Delay(int us)
{
    STCURRENT = 0;
    STRELOAD = us;
    STCTRL |= (CLKINT | ENABLE);
    while (!(STCTRL & COUNT_FLAG))
        ;
    STCTRL = 0;
}

int main(void)
{
    GPIO_INIT();
    Timer0A_Init();
    GPIO_PORTF_IS_R = 0x00;
    GPIO_PORTF_IBE_R = 0x10;
    GPIO_PORTF_IM_R = 0x10;
    NVIC_EN0_R = 0x40000000;

    while (1)
    {
        if (x != 0)
        {
            GPIO_PORTF_DATA_R |= 0x02;
            Delay(x);
        }
        if (x != (16000000 / f))
        {
            GPIO_PORTF_DATA_R &= ~0x02;
            Delay((16000000 / f) - x);
        }
    }
}
