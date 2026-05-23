#include "mm32_device.h"
#include <stdint.h>

uint32_t SystemCoreClock = 8000000U; // Initialized to default 8MHz

// Configure system clock to run CPU at full 48MHz (peripherals at safe 24MHz limit)
void SystemInit(void) {
    // 1. Enable HSI
    RCC->CR |= RCC_CR_HSION;
    
    // 2. Wait until HSI is stable and ready
    while (!(RCC->CR & RCC_CR_HSIRDY)) {
    }
    
    // 3. Configure Flash Latency to 1 wait state (0x2) and enable Prefetch
    // This is mandatory when running the CPU core at 48MHz
    FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;
    FLASH->ACR |= FLASH_ACR_LATENCY_1;  // Set 1 Wait State
    FLASH->ACR |= FLASH_ACR_PRFTBE;     // Enable prefetch buffer
    
    // 4. Set AHB prescaler to DIV1 (HCLK = SYSCLK = 48MHz CPU clock)
    RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
    RCC->CFGR |= (0x00U << RCC_CFGR_HPRE_Pos); // HPRE = DIV1
    
    // 5. Set APB1 prescaler to DIV2 (PCLK1 = HCLK / 2 = 24MHz peripheral clock)
    // Runs the USART peripheral safely within its 24MHz maximum hardware limit
    RCC->CFGR &= ~RCC_CFGR_PPRE1_Msk;
    RCC->CFGR |= (0x04U << RCC_CFGR_PPRE1_Pos); // PPRE1 = DIV2
    
    // 6. Switch System Clock Source to HSI (48MHz direct clock)
    RCC->CFGR &= ~RCC_CFGR_SW_Msk;
    RCC->CFGR |= (0x02U << RCC_CFGR_SW_Pos); // Select HSI
    
    // 7. Wait until HSI is successfully selected as the system clock source
    while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != (0x02U << RCC_CFGR_SWS_Pos)) {
        __ASM("nop");
    }
    
    // 8. Update global clock variable to 48MHz
    SystemCoreClock = 48000000U;
}

void delay(void) {
    // Calibrated delay loop for 48MHz CPU clock
    for(volatile int i=0; i<3000000; i++);
}

void uart_init(void) {
    // 1. Enable GPIOA Clock in RCC_AHBENR
    RCC->AHBENR |= RCC_AHBENR_GPIOA;

    // 2. Enable USART1 Clock in RCC_APB1ENR
    RCC->APB1ENR |= RCC_APB1ENR_USART1;

    // 3. Configure PA12 (TX) in GPIOA_CRH (Bits 16-19)
    // Clear CNF12 and MODE12
    GPIOA->CRH &= ~(0xFU << 16U);
    // Set to Alternate Function Output Push-Pull (value 0x9: CNF=2 (AF push-pull), MODE=1 (10MHz speed))
    GPIOA->CRH |= (0x9U << 16U);

    // 4. Configure AFR12 in GPIOA_AFRH (Bits 16-19) to AF1 (value 0x1)
    GPIOA->AFRH &= ~(0xFU << 16U);
    GPIOA->AFRH |= (0x1U << 16U);

    // 5. Configure Baud Rate to 115200 assuming 24MHz peripheral clock (APB1 divided by 2)
    // USARTDIV = 24000000 / (16 * 115200) = 13.0208
    // Mantissa = 13 = 0x0D, Fraction = 0.0208 * 16 = 0.33 \approx 0
    // Write (Mantissa << 4) | (Fraction & 0xF)
    USART1->BRR = (13U << 4U) | 0U;

    // 6. Enable USART (UE) and Transmitter (TE) in USART1_CR1
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}

void uart_putchar(char c) {
    // Wait until TXE (Transmit data register empty) is set
    while (!(USART1->SR & USART_SR_TXE));
    // Write character to USART data register
    USART1->DR = (uint32_t)c;
}

void uart_print(const char *str) {
    // Print each character in the string
    while (*str) {
        if (*str == '\n') {
            uart_putchar('\r');
        }
        uart_putchar(*str++);
    }
}

int main(void) {
    // Initialize USART1
    uart_init();

    // Set PA6 (bits 24-27) as Output for blinking LED
    GPIOA->CRL &= ~(0xFU << 24U);
    GPIOA->CRL |= (0x1U << 24U);

    uart_print("MM32G0001 Native Boot Successful at 48MHz CPU (115200 Baud)!\n");

    while (1) {
        // LED ON (using bit 6 of Port A set register)
        GPIOA->BSRR = (1U << 6U);
        uart_print("LED ON\n");
        delay();

        // LED OFF (using bit 22 of Port A reset register, which corresponds to bit 6)
        GPIOA->BSRR = (1U << 22U);
        uart_print("LED OFF\n");
        delay();
    }
}
