#include "ch32fun.h"
#include <stdio.h>

// On the SOP8 package, PD4 is Pin 3 on the physical chip.
#define BLINK_PIN 4 

int main(void)
{
    // 1. Initialize the system clocks (fun sets this to 48MHz internal by default)
    SystemInit();

    // 2. Enable the clock for Port D so we can use its pins
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;

    // 3. Configure PD4 as a 10MHz Output Push-Pull
    // Each pin takes up 4 bits in the CFGLR register. 
    // First, clear the 4 bits for Pin 4...
    GPIOD->CFGLR &= ~(0xf << (BLINK_PIN * 4));
    // ...then set them to 0x01 (which means Output, Max Speed 10MHz, Push-Pull)
    GPIOD->CFGLR |= (0x01 << (BLINK_PIN * 4));

    while(1)
    {
        // Turn Pin High (Write to Bit Status Set/Reset register)
        GPIOD->BSHR = (1 << BLINK_PIN);
        
        // Precise millisecond delay provided natively by ch32v003fun
        Delay_Ms(500); 

        // Turn Pin Low (Write to Bit Clear register)
        GPIOD->BCR = (1 << BLINK_PIN);
        
        Delay_Ms(500);
    }
}