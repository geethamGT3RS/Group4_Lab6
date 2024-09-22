#include <stdint.h>
#include "tm4c123gh6pm.h"

#define STCTRL *((volatile long *)0xE000E010)
#define STRELOAD *((volatile long *)0xE000E014)
#define STCURRENT *((volatile long *)0xE000E018)

#define COUNT_FLAG (1 << 16)
#define ENABLE (1 << 0)
#define CLKINT (1 << 2)
#define CLOCK_MHZ 16

volatile int dutyCycle = 50;

void delayMicroseconds(int us)
{
    STCURRENT = 0;
    STRELOAD = us * (CLOCK_MHZ);
    STCTRL |= (CLKINT | ENABLE);
    while ((STCTRL & COUNT_FLAG) == 0)
        ;
    STCTRL = 0;
}

void updateDutyCycle(int change)
{
    dutyCycle += change;
    if (dutyCycle > 100)
        dutyCycle = 100;
    if (dutyCycle < 0)
        dutyCycle = 0;
}

void GPIOF_Handler(void)
{
    if (GPIO_PORTF_RIS_R & 0x10)
    {
        GPIO_PORTF_ICR_R = 0x10;
        updateDutyCycle(5);
    }
    if (GPIO_PORTF_RIS_R & 0x01)
    {
        GPIO_PORTF_ICR_R = 0x01;
        updateDutyCycle(-5);
    }
}

void initGPIO(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20;
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R = 0x1F;
    GPIO_PORTF_DIR_R = 0x0E;
    GPIO_PORTF_DEN_R = 0x1F;
    GPIO_PORTF_PUR_R = 0x11;
}

void initInterrupts(void)
{
    GPIO_PORTF_IS_R &= ~0x11;
    GPIO_PORTF_IBE_R &= ~0x11;
    GPIO_PORTF_IEV_R &= ~0x11;
    GPIO_PORTF_ICR_R = 0x11;
    GPIO_PORTF_IM_R |= 0x11;
    NVIC_EN0_R = (1 << 30);
}

int main(void)
{
    initGPIO();
    initInterrupts();

    const int pwmFrequency = 100000;
    const int period_us = 1000000 / pwmFrequency;

    while (1)
    {
        int onTime = (period_us * dutyCycle) / 100;
        int offTime = period_us - onTime;

        if (onTime > 0)
        {
            GPIO_PORTF_DATA_R |= 0x02;
            delayMicroseconds(onTime);
        }
        if (offTime > 0)
        {
            GPIO_PORTF_DATA_R &= ~0x02;
            delayMicroseconds(offTime);
        }
    }
}
