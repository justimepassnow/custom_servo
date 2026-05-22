#include "ch32fun.h"
#include <stdio.h>

// On the SOP8 package, PD6 maps directly to Physical Leg 1
#define BLINK_PIN 6 

int main(void)
{
    // 1. Initialize the system clocks (48MHz internal)
    SystemInit();

    // 2. Enable the clock for Port D
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;

    // 3. Configure PD6 as a 10MHz Output Push-Pull
    GPIOD->CFGLR &= ~(0xf << (BLINK_PIN * 4)); // Clear the 4 bits for Pin 6
    GPIOD->CFGLR |= (0x01 << (BLINK_PIN * 4)); // Set to Output, Max Speed 10MHz, Push-Pull

    while(1)
    {
        // Turn Pin High
        GPIOD->BSHR = (1 << BLINK_PIN);
        
        Delay_Ms(500); 

        // Turn Pin Low
        GPIOD->BCR = (1 << BLINK_PIN);
        
        Delay_Ms(500);
    }
}