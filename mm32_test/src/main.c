#include <stdint.h>

// 1. Force the HSI clock ON before any peripheral init
void __wrap_SystemInit(void) {
    // RCC_CR is at offset 0x00
    // Enable HSI (Bit 0)
    *(volatile uint32_t *)(0x40021000U + 0x00U) |= (1U << 0U); 
    // Wait for HSI ready (Bit 1)
    while(!(*(volatile uint32_t *)(0x40021000U + 0x00U) & (1U << 1U)));
}

// RCC Registers
#define RCC_AHBENR         (*(volatile uint32_t *)(0x40021000U + 0x14U))
#define RCC_APB1ENR        (*(volatile uint32_t *)(0x40021000U + 0x1CU))

// GPIOA Registers
#define GPIOA_CRL          (*(volatile uint32_t *)(0x48000000U + 0x00U))
#define GPIOA_CRH          (*(volatile uint32_t *)(0x48000000U + 0x04U))
#define GPIOA_BSRR         (*(volatile uint32_t *)(0x48000000U + 0x10U))
#define GPIOA_AFRH         (*(volatile uint32_t *)(0x48000000U + 0x24U))

// USART1 Registers
#define USART1_SR          (*(volatile uint32_t *)(0x40013800U + 0x00U))
#define USART1_DR          (*(volatile uint32_t *)(0x40013800U + 0x04U))
#define USART1_BRR         (*(volatile uint32_t *)(0x40013800U + 0x08U))
#define USART1_CR1         (*(volatile uint32_t *)(0x40013800U + 0x0CU))

void delay(void) {
    for(volatile int i=0; i<500000; i++);
}

void uart_init(void) {
    // 1. Enable GPIOA Clock (Bit 17) in RCC_AHBENR
    RCC_AHBENR |= (1U << 17U);

    // 2. Enable USART1 Clock (Bit 16) in RCC_APB1ENR
    RCC_APB1ENR |= (1U << 16U);

    // 3. Configure PA12 (TX) in GPIOA_CRH (Bits 16-19)
    // Clear CNF12 and MODE12
    GPIOA_CRH &= ~(0xFU << 16U);
    // Set to Alternate Function Output Push-Pull (value 0x9: CNF=2 (AF push-pull), MODE=1 (10MHz speed))
    GPIOA_CRH |= (0x9U << 16U);

    // 4. Configure AFR12 in GPIOA_AFRH (Bits 16-19) to AF1 (value 0x1)
    GPIOA_AFRH &= ~(0xFU << 16U);
    GPIOA_AFRH |= (0x1U << 16U);

    // 5. Configure Baud Rate to 9600 assuming 8MHz peripheral clock
    // USARTDIV = 8000000 / (16 * 9600) = 52.0833
    // Mantissa = 52 = 0x34, Fraction = 0.0833 * 16 = 1.33 \approx 1
    // Write (Mantissa << 4) | (Fraction & 0xF)
    USART1_BRR = (52U << 4U) | 1U;

    // 6. Enable USART (UE, Bit 13) and Transmitter (TE, Bit 3) in CR1
    USART1_CR1 = (1U << 13U) | (1U << 3U);
}

void uart_putchar(char c) {
    // Wait until TXE (Transmit data register empty, Bit 7) is set
    while (!(USART1_SR & (1U << 7U)));
    // Write character
    USART1_DR = (uint32_t)c;
}

void uart_print(const char *str) {
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
    GPIOA_CRL &= ~(0xFU << 24U);
    GPIOA_CRL |= (0x1U << 24U);

    uart_print("MM32G0001 Boot Successful!\n");

    while (1) {
        // LED ON
        GPIOA_BSRR = (1U << 6U);
        uart_print("LED ON\n");
        delay();

        // LED OFF
        GPIOA_BSRR = (1U << 22U);
        uart_print("LED OFF\n");
        delay();
    }
}
