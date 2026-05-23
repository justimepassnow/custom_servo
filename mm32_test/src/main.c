#include "mm32_device.h"
#include <stdint.h>
#include <stdio.h>

#define BUF_SIZE 16

uint32_t SystemCoreClock = 8000000U; // Initialized to default 8MHz
char rx_buf[BUF_SIZE];
uint8_t rx_idx = 0;

// Configure system clock to run CPU at full 48MHz (peripherals at safe 24MHz limit)
void SystemInit(void) {
    // 1. Enable HSI
    RCC->CR |= RCC_CR_HSION;
    
    // 2. Wait until HSI is stable and ready
    while (!(RCC->CR & RCC_CR_HSIRDY)) {
    }
    
    // 3. Configure Flash Latency to 1 wait state (0x2) and enable Prefetch
    FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;
    FLASH->ACR |= FLASH_ACR_LATENCY_1;  // Set 1 Wait State
    FLASH->ACR |= FLASH_ACR_PRFTBE;     // Enable prefetch buffer
    
    // 4. Set AHB prescaler to DIV1 (HCLK = SYSCLK = 48MHz CPU clock)
    RCC->CFGR &= ~RCC_CFGR_HPRE_Msk;
    RCC->CFGR |= (0x00U << RCC_CFGR_HPRE_Pos); // HPRE = DIV1
    
    // 5. Set APB1 prescaler to DIV2 (PCLK1 = HCLK / 2 = 24MHz peripheral clock)
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

// Calibrated delay loop for ms at 48MHz CPU clock
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 6000; i++) {
        __ASM("nop");
    }
}

void uart_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

    // 1. Enable GPIOA and USART1 peripheral clocks using official HAL APIs
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_USART1, ENABLE);

    // 2. Configure PA12 as Alternate Function Push-Pull Output (TX)
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. Configure Pin Multiplexing for PA12 to AF1 (USART1_TX)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_1);

    // 4. Configure PA3 as Input Pull-Up (USART1_RX)
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 5. Configure Pin Multiplexing for PA3 to AF1 (USART1_RX)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

    // 6. Configure USART1 with 115200 Baud, 8-N-1 format, Rx and Tx enabled
    USART_StructInit(&USART_InitStruct);
    USART_InitStruct.USART_BaudRate   = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits   = USART_StopBits_1;
    USART_InitStruct.USART_Parity     = USART_Parity_No;
    USART_InitStruct.USART_Mode       = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStruct);

    // 7. Enable USART1
    USART_Cmd(USART1, ENABLE);
}

void uart_putchar(char c) {
    // Wait until Transmit Data Register is Empty (TXE flag is SET)
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    // Write character using HAL transmit API
    USART_SendData(USART1, (uint16_t)c);
}

void uart_print(const char *str) {
    while (*str) {
        if (*str == '\n') {
            uart_putchar('\r');
        }
        uart_putchar(*str++);
    }
}

// Convert an integer to a string (simple custom implementation to avoid big standard library bloat)
void uart_print_int(uint32_t val) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) {
        uart_print("0");
        return;
    }
    while (val > 0 && i >= 0) {
        buf[i--] = (val % 10) + '0';
        val /= 10;
    }
    uart_print(&buf[i + 1]);
}

int main(void) {
    GPIO_InitTypeDef        GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_OCInitTypeDef       TIM_OCInitStruct;

    // 1. Initialize USART1 at 115200 Baud for debug logging and input
    uart_init();
    uart_print("\n--- MM32G0001 2kHz Serial PWM Controller ---\n");

    // 2. Enable Peripheral Clocks for TIM1 and GPIOA
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_TIM1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);

    // 3. Calculate Timer Period for 2kHz frequency
    uint32_t TimerClock = TIM_GetTIMxClock(TIM1);
    uint32_t TargetFrequency = 2000; // 2kHz
    uint32_t TimerPeriod = (TimerClock / TargetFrequency) - 1;

    uart_print("Timer Peripheral Clock: ");
    uart_print_int(TimerClock);
    uart_print(" Hz\n");

    uart_print("Calculated Timer Period (ARR): ");
    uart_print_int(TimerPeriod);
    uart_print("\n");

    // 4. Initialize TIM1 Time Base
    TIM_TimeBaseStructInit(&TIM_TimeBaseStruct);
    TIM_TimeBaseStruct.TIM_Prescaler         = 0;
    TIM_TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period            = TimerPeriod;
    TIM_TimeBaseStruct.TIM_ClockDivision     = TIM_CKD_Div1;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStruct);

    // 5. Initialize TIM1 Channel 3 Output Compare (PWM1 mode)
    TIM_OCStructInit(&TIM_OCInitStruct);
    TIM_OCInitStruct.TIM_OCMode       = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState  = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse        = (50 * TimerPeriod) / 100; // Start at 50% duty cycle
    TIM_OCInitStruct.TIM_OCPolarity   = TIM_OCPolarity_High;
    TIM_OCInitStruct.TIM_OCIdleState  = TIM_OCIdleState_Set;
    TIM_OC3Init(TIM1, &TIM_OCInitStruct);

    // 6. Configure PA6 AF4 pin mapping to TIM1_CH3 (Heartbeat LED on MM32 MiniBoard)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_4);

    // 7. Initialize PA6 as Alternate Function Push-Pull
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 7b. Initialize PA5 as Input Pull-Up for physical Software Reset Button
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU; // Input Pull-Up
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 8. Enable TIM1 and its Main PWM Outputs (required for advanced timers)
    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    uart_print("TIM1 Channel 3 PWM Initialization Complete on PA6.\n");
    uart_print("Software Reset Button active on PA5 (ground to reset).\n");
    uart_print("Ready! Enter a duty cycle percentage (0-100) and press Enter:\n\n");
    uart_print("> ");

    while (1) {
        // A. Check if the physical reset button on PA5 is pressed (reads LOW/RESET)
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == Bit_RESET) {
            uart_print("\n[RESET] Physical Button Pressed! Triggering Software System Reset...\n");
            delay_ms(50); // Debounce delay
            NVIC_SystemReset(); // Call CMSIS Standard System Reset
        }

        // B. Poll for serial RX data
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
            char c = (char)USART_ReceiveData(USART1);
            
            // Handle carriage return or newline to trigger command processing
            if (c == '\r' || c == '\n') {
                if (rx_idx > 0) {
                    rx_buf[rx_idx] = '\0';
                    
                    // Parse integer from text
                    int32_t val = 0;
                    for (int i = 0; rx_buf[i] != '\0'; i++) {
                        if (rx_buf[i] >= '0' && rx_buf[i] <= '9') {
                            val = val * 10 + (rx_buf[i] - '0');
                        }
                    }
                    
                    // Enforce range: 0% to 100%
                    if (val < 0) val = 0;
                    if (val > 100) val = 100;
                    
                    // Update pulse width (CCR3)
                    uint32_t pulse = (val * TimerPeriod) / 100;
                    TIM_SetCompare3(TIM1, pulse);
                    
                    uart_print("\nSet LED Duty Cycle to: ");
                    uart_print_int(val);
                    uart_print("% (Pulse: ");
                    uart_print_int(pulse);
                    uart_print(")\n\n> ");
                    
                    rx_idx = 0;
                }
            } 
            // Handle Backspace (ASCII 8 or 127)
            else if (c == 8 || c == 127) {
                if (rx_idx > 0) {
                    rx_idx--;
                    uart_print("\b \b"); // Clear char on terminal screen
                }
            } 
            // Save normal characters to buffer and echo back
            else {
                if (rx_idx < BUF_SIZE - 1) {
                    rx_buf[rx_idx++] = c;
                    uart_putchar(c); // Echo character
                }
            }
        }
    }
}
