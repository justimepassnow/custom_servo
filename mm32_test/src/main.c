#include "mm32_device.h"
#include <stdint.h>
#include <stdio.h>

#define BUF_SIZE 16

#define ANGLE_MIN 45
#define ANGLE_MAX 280

uint32_t SystemCoreClock = 8000000U; // Initialized to default 8MHz

// PID target and feedback
volatile int32_t target_angle = 150; // default target to center (150 degrees)
volatile int32_t current_angle = 0;
volatile uint8_t status_reporting_enabled = 0;

// PID parameters (default tuning, float representation for serial parser only)
float Kp = 100.0f;
float Ki = 0.05f;
float Kd = 8.0f;

// PID fixed-point Q16 gains (pre-calculated for 1ms time-step)
volatile int32_t Kp_q16 = 6553600;     // 100.0 * 65536
volatile int32_t Ki_q16 = 3;           // 0.05 * 0.001 * 65536 (rounded)
volatile int32_t Kd_q16 = 524288000;   // 8.0 / 0.001 * 65536

// PID state variables (pure integer, Q8 fixed-point format)
int32_t pid_error = 0;
int32_t prev_error = 0;
int32_t pid_integral = 0;
int32_t pid_derivative = 0;

#define PID_INTERVAL_MS 1
#define INTEGRAL_LIMIT_Q8 25600 // ~100 degrees max windup limit in Q8
#define DEADBAND_Q8 384         // 1.5 degrees deadband in Q8

void print_status(void);

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

void uart_enable_tx(void) {
    USART_RxCmd(USART1, DISABLE);
    USART_TxCmd(USART1, ENABLE);
}

void uart_enable_rx(void) {
    // Wait for Transmission Complete (TC) to ensure the last byte is fully shifted out
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    USART_TxCmd(USART1, DISABLE);
    USART_RxCmd(USART1, ENABLE);
}

void uart_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;

    // 1. Enable GPIOA and USART1 peripheral clocks using official HAL APIs
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_USART1, ENABLE);

    // 2. Configure PA12 as Alternate Function Open-Drain Output (Single-Wire shared pin)
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_OD; // Open-drain mode for shared line with external pull-up
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. Configure Pin Multiplexing for PA12 to AF1 (USART1_TX)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_1);

    // 4. Configure USART1 with 115200 Baud, 8-N-1 format, initially in Rx mode
    USART_StructInit(&USART_InitStruct);
    USART_InitStruct.USART_BaudRate   = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits   = USART_StopBits_1;
    USART_InitStruct.USART_Parity     = USART_Parity_No;
    USART_InitStruct.USART_Mode       = USART_Mode_Rx; // Initial state: Rx listening
    USART_Init(USART1, &USART_InitStruct);

    // 5. Enable Half-Duplex selection (connects RX to TX pin internally and disables separate RX pin)
    USART_HalfDuplexCmd(USART1, ENABLE);

    // 6. Enable USART1
    USART_Cmd(USART1, ENABLE);
}

// Low-level helper to write a character to USART without managing half-duplex state transitions
void uart_putchar_raw(char c) {
    // Wait until Transmit Data Register is Empty (TXE flag is SET)
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    // Write character using HAL transmit API
    USART_SendData(USART1, (uint16_t)c);
}

// Public API: Write a character with automatic half-duplex state transition
void uart_putchar(char c) {
    uart_enable_tx();
    uart_putchar_raw(c);
    uart_enable_rx();
}

// Public API: Write a string with optimized single-duplex transition for the entire block
void uart_print(const char *str) {
    uart_enable_tx();
    while (*str) {
        if (*str == '\n') {
            uart_putchar_raw('\r');
        }
        uart_putchar_raw(*str++);
    }
    uart_enable_rx();
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

// 1ms SysTick timekeeping variables
volatile uint32_t ms_ticks = 0;
void SysTick_Handler(void) {
    ms_ticks++;
}

uint32_t millis(void) {
    return ms_ticks;
}

// Initialize ADC1 for analog reading on PA7 (potentiometer) and PA2 (current sensor)
void adc_init(void) {
    ADC_InitTypeDef  ADC_InitStruct;
    GPIO_InitTypeDef GPIO_InitStruct;

    // 1. Enable ADC1 peripheral clock
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_ADC1, ENABLE);

    // 2. Initialize ADC1
    ADC_StructInit(&ADC_InitStruct);
    ADC_InitStruct.ADC_Resolution       = ADC_Resolution_12b;
    ADC_InitStruct.ADC_Prescaler        = ADC_Prescaler_16;
    ADC_InitStruct.ADC_Mode             = ADC_Mode_Imm;
    ADC_InitStruct.ADC_DataAlign        = ADC_DataAlign_Right;
    ADC_Init(ADC1, &ADC_InitStruct);

    // 3. Configure sampling time for optimal precision
    ADC_SampleTimeConfig(ADC1, ADC_SampleTime_240_5);

    // 4. Configure PA7 and PA2 as Analog Inputs (AIN)
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7 | GPIO_Pin_2;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 5. Enable ADC1
    ADC_Cmd(ADC1, ENABLE);
}

// Read an analog value on the specified ADC channel using standard AnyChannel single-conversion polling
uint16_t adc_read(uint8_t channel) {
    // Configure AnyChannel mode for a single-channel scan
    ADC_AnyChannelNumCfg(ADC1, 0);
    ADC_AnyChannelSelect(ADC1, ADC_AnyChannel_0, channel);
    ADC_AnyChannelCmd(ADC1, ENABLE);

    // Start conversion
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    // Wait until conversion completes (EOC flag is set)
    while (RESET == ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)) {
        __ASM("nop");
    }

    // Clear flag and read result
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    uint16_t value = ADC_GetConversionValue(ADC1);

    // Disable AnyChannel mode
    ADC_AnyChannelCmd(ADC1, DISABLE);

    return value;
}

// Read current draw in Milliamperes (mA) from INA180 current sensor on PA2 (ADC_Channel_5)
// Shunt = 0.01 Ohms, Gain = 20
// I (mA) = raw_adc * 16500 / 4095
int32_t read_current_ma(void) {
    uint16_t raw_adc = adc_read(ADC_Channel_5);
    int32_t current_ma = ((int32_t)raw_adc * 16500) / 4095;
    return current_ma;
}

// Absolute value function for floats
float float_abs(float val) {
    return (val >= 0.0f) ? val : -val;
}

// Parse a float from a string (integer part + fractional part)
float parse_float(const char *str) {
    float res = 0.0f;
    float sign = 1.0f;
    int i = 0;
    if (str[i] == '-') {
        sign = -1.0f;
        i++;
    }
    // Integer part
    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10.0f + (str[i] - '0');
        i++;
    }
    // Fractional part
    if (str[i] == '.') {
        i++;
        float factor = 0.1f;
        while (str[i] >= '0' && str[i] <= '9') {
            res += (str[i] - '0') * factor;
            factor *= 0.1f;
            i++;
        }
    }
    return res * sign;
}

// Custom float print utility for serial debug (without float printf bloat)
void uart_print_float(float val) {
    if (val < 0.0f) {
        uart_print("-");
        val = -val;
    }
    uint32_t integer_part = (uint32_t)val;
    uint32_t fractional_part = (uint32_t)((val - (float)integer_part) * 100.0f + 0.5f); // 2 decimal places, rounded
    uart_print_int(integer_part);
    uart_print(".");
    if (fractional_part < 10) {
        uart_print("0");
    }
    uart_print_int(fractional_part);
}

// Non-blocking serial parsing to update the target angle and PID gains
void process_serial_input(void) {
    static char rx_buf[BUF_SIZE];
    static uint8_t rx_idx = 0;

    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
        char c = (char)USART_ReceiveData(USART1);
        
        // Echo back the character to the user
        if (c == '\r') {
            uart_print("\r\n");
        } else if (c != '\n') {
            uart_putchar(c);
        }

        if (c == '\r' || c == '\n') {
            if (rx_idx > 0) {
                rx_buf[rx_idx] = '\0';
                
                char cmd = rx_buf[0];
                
                // Skip leading spaces after the command letter, if any
                int start_idx = 1;
                while (rx_buf[start_idx] == ' ' || rx_buf[start_idx] == '\t') {
                    start_idx++;
                }
                
                if (cmd == 'p' || cmd == 'P') {
                    Kp = parse_float(&rx_buf[start_idx]);
                    Kp_q16 = (int32_t)(Kp * 65536.0f);
                    uart_print("[PID] Kp set to ");
                    uart_print_float(Kp);
                    uart_print("\r\n");
                } else if (cmd == 'i' || cmd == 'I') {
                    Ki = parse_float(&rx_buf[start_idx]);
                    Ki_q16 = (int32_t)(Ki * 0.001f * 65536.0f); // Pre-calculate for 1ms interval
                    uart_print("[PID] Ki set to ");
                    uart_print_float(Ki);
                    uart_print("\r\n");
                } else if (cmd == 'd' || cmd == 'D') {
                    Kd = parse_float(&rx_buf[start_idx]);
                    Kd_q16 = (int32_t)((Kd / 0.001f) * 65536.0f); // Pre-calculate for 1ms interval
                    uart_print("[PID] Kd set to ");
                    uart_print_float(Kd);
                    uart_print("\r\n");
                } else if (cmd >= '0' && cmd <= '9') {
                    // It's a target angle
                    int32_t val = 0;
                    int i = 0;
                    while (rx_buf[i] >= '0' && rx_buf[i] <= '9') {
                        val = val * 10 + (rx_buf[i] - '0');
                        i++;
                    }
                    if (val >= ANGLE_MIN && val <= ANGLE_MAX) {
                        target_angle = val;
                        status_reporting_enabled = 1; // Start reporting status
                        uart_print("[Target] Set to ");
                        uart_print_int(target_angle);
                        uart_print(" degrees. Tracking...\r\n");
                    } else {
                        uart_print("[Error] Target ");
                        uart_print_int(val);
                        uart_print(" out of range (");
                        uart_print_int(ANGLE_MIN);
                        uart_print("-");
                        uart_print_int(ANGLE_MAX);
                        uart_print(")\r\n");
                    }
                } else {
                    uart_print("[Error] Unknown command. Use P/I/D values or target angle digits.\r\n");
                }
                
                rx_idx = 0;
            }
        } else if (c != '\b' && rx_idx < BUF_SIZE - 1) {
            // Store character
            rx_buf[rx_idx++] = c;
        } else if (c == '\b' && rx_idx > 0) {
            // Handle backspace
            rx_idx--;
        }
    }
}

// PID controller update function (called every 1ms, Q16/Q8 integer control math)
void pid_update(uint32_t arr_period) {
    // Low-pass filter state in Q8 (256 scale factor)
    static int32_t filtered_adc_q8 = -1;

    // 1. Read potentiometer ADC and apply exponential moving average filter in Q8
    uint16_t raw_adc = adc_read(ADC_Channel_7);
    if (filtered_adc_q8 < 0) {
        filtered_adc_q8 = ((int32_t)raw_adc) << 8; // initialize
    } else {
        // alpha = 0.10 -> Q8 equivalent is 26/256
        // Pre-scale raw_adc to Q8, multiply, and shift right back to Q8 scale
        filtered_adc_q8 = (26 * (((int32_t)raw_adc) << 8) + 230 * filtered_adc_q8) >> 8;
    }

    // 2. Map filtered ADC to float physical angle (the ONLY float variable allowed)
    float current_angle_f = ((float)(filtered_adc_q8 >> 8) * 300.0f) / 4095.0f;
    
    // Update global integer angle for printing/display (convert float back to integer)
    current_angle = (int32_t)(current_angle_f + 0.5f);

    // 3. Compute error as Q8 integer (scaling float error by 256)
    pid_error = (int32_t)(((float)target_angle - current_angle_f) * 256.0f);

    // 4. Check deadband in Q8
    int32_t abs_error = (pid_error >= 0) ? pid_error : -pid_error;
    if (abs_error < DEADBAND_Q8) {
        // Within deadband: hold motor completely still
        TIM_SetCompare3(TIM1, 0); // PA6 low
        TIM_SetCompare2(TIM1, 0); // PA8 low
        pid_integral = 0;         // Reset integral term
        prev_error = 0;
        if (status_reporting_enabled) {
            status_reporting_enabled = 0;
            print_status(); // Print final target reached state
            uart_print("[STATUS] Target reached! Motor is now idle.\r\n");
        }
        return;
    }

    // 5. Calculate P, I, D terms in Q24 format using 64-bit integers to prevent 32-bit overflow
    int64_t p_term = (int64_t)Kp_q16 * (int64_t)pid_error;

    // Accumulate integral in Q8 (Ki_q16 is pre-scaled by dt = 0.001s)
    pid_integral += pid_error;
    if (pid_integral > INTEGRAL_LIMIT_Q8) pid_integral = INTEGRAL_LIMIT_Q8;
    else if (pid_integral < -INTEGRAL_LIMIT_Q8) pid_integral = -INTEGRAL_LIMIT_Q8;
    
    int64_t i_term = (int64_t)Ki_q16 * (int64_t)pid_integral;

    // Calculate derivative term (Kd_q16 is pre-scaled by 1/dt = 1000)
    pid_derivative = pid_error - prev_error;
    prev_error = pid_error;
    
    int64_t d_term = (int64_t)Kd_q16 * (int64_t)pid_derivative;

    // Sum all terms in Q24 format using 64-bit integer
    int64_t pid_out_q24 = p_term + i_term + d_term;

    // Scale output back to standard integer timer range (divide Q24 by 2^24)
    int32_t pid_out = (int32_t)(pid_out_q24 >> 24);

    // 6. Output to PWM channels based on direction
    int32_t pulse = (pid_out >= 0) ? pid_out : -pid_out;
    if (pulse > (int32_t)arr_period) {
        pulse = (int32_t)arr_period; // clamp to 100% duty cycle
    }

    if (pid_out > 0) {
        // Direction A (Forward): PWM on Channel 3, LOW on Channel 2
        TIM_SetCompare3(TIM1, pulse);
        TIM_SetCompare2(TIM1, 0);
    } else {
        // Direction B (Backward): LOW on Channel 3, PWM on Channel 2
        TIM_SetCompare3(TIM1, 0);
        TIM_SetCompare2(TIM1, pulse);
    }
}

// Prints the measured and target values (without float printf bloat)
void print_status(void) {
    uart_print("[STATUS] Target: ");
    uart_print_int(target_angle);
    uart_print(" deg | Current: ");
    uart_print_int(current_angle);
    uart_print(" deg | Error: ");
    
    // Support printing negative numbers for error
    int32_t err = target_angle - current_angle;
    if (err < 0) {
        uart_print("-");
        uart_print_int(-err);
    } else {
        uart_print_int(err);
    }
    
    uart_print(" deg | Current Draw: ");
    uart_print_int(read_current_ma());
    uart_print(" mA | Duty: ");
    // Compute current duty cycle percent of the active output channel
    uint32_t pulse3 = TIM1->CCR3;
    uint32_t pulse2 = TIM1->CCR2;
    uint32_t arr = TIM1->ARR;
    if (arr == 0) arr = 1; // avoid division by 0
    
    if (pulse3 > 0) {
        uart_print_int((pulse3 * 100) / arr);
        uart_print("% (PA6 Forward)\r\n");
    } else if (pulse2 > 0) {
        uart_print_int((pulse2 * 100) / arr);
        uart_print("% (PA8 Backward)\r\n");
    } else {
        uart_print("0% (Idle/Deadband)\r\n");
    }
}

int main(void) {
    GPIO_InitTypeDef        GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_OCInitTypeDef       TIM_OCInitStruct;

    // 1. Configure SysTick for 1ms tick interrupts (48MHz CPU clock)
    SysTick_Config(48000000U / 1000);

    // 2. Initialize USART1 at 115200 Baud for debug logging and input
    uart_init();
    uart_print("\n--- MM32G0001 Analog-Controlled PWM Dimmer ---\n");

    // 3. Enable Peripheral Clocks for TIM1 and GPIOA
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_TIM1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPERIPH_GPIOA, ENABLE);

    // 4. Calculate Timer Period for 20kHz frequency
    uint32_t TimerClock = TIM_GetTIMxClock(TIM1);
    uint32_t TargetFrequency = 20000; // 20kHz
    uint32_t TimerPeriod = (TimerClock / TargetFrequency) - 1;

    uart_print("Timer Peripheral Clock: ");
    uart_print_int(TimerClock);
    uart_print(" Hz\n");

    uart_print("Calculated Timer Period (ARR): ");
    uart_print_int(TimerPeriod);
    uart_print("\n");

    // 5. Initialize TIM1 Time Base
    TIM_TimeBaseStructInit(&TIM_TimeBaseStruct);
    TIM_TimeBaseStruct.TIM_Prescaler         = 0;
    TIM_TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period            = TimerPeriod;
    TIM_TimeBaseStruct.TIM_ClockDivision     = TIM_CKD_Div1;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStruct);

    // 6. Initialize TIM1 Channel 3 and Channel 2 Output Compare (PWM1 mode)
    TIM_OCStructInit(&TIM_OCInitStruct);
    TIM_OCInitStruct.TIM_OCMode       = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState  = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse        = 0; // Start at 0% duty cycle
    TIM_OCInitStruct.TIM_OCPolarity   = TIM_OCPolarity_High;
    TIM_OCInitStruct.TIM_OCIdleState  = TIM_OCIdleState_Set;
    TIM_OC3Init(TIM1, &TIM_OCInitStruct); // Channel 3 (PA6)
    TIM_OC2Init(TIM1, &TIM_OCInitStruct); // Channel 2 (PA8)

    // 7. Configure AF pin mappings: PA6 -> TIM1_CH3 (AF4), PA8 -> TIM1_CH2 (AF1)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_4);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_1);

    // 8. Initialize PA6 and PA8 as Alternate Function Push-Pull outputs
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 9. Initialize PA5 as Input Pull-Up for physical Software Reset Button
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_High;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU; // Input Pull-Up
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 10. Initialize ADC1 Channel 7 on PA7
    adc_init();

    // 11. Enable TIM1 and its Main PWM Outputs (required for advanced timers)
    TIM_Cmd(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    uart_print("TIM1 PWM Channels 2 & 3 Init Complete on PA8 & PA6.\r\n");
    uart_print("Software Reset Button active on PA5.\r\n");
    uart_print("ADC1 Channel 7 active on PA7.\r\n");
    uart_print("PID Motor Position Controller active! Target: 150 deg.\r\n\r\n");

    static uint32_t last_pid_time = 0;

    while (1) {
        // A. Check if the physical reset button on PA5 is pressed (reads LOW/RESET)
        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == Bit_RESET) {
            uart_print("\r\n[RESET] Physical Button Pressed! Triggering Software System Reset...\r\n");
            delay_ms(50); // Debounce delay
            NVIC_SystemReset(); // Call CMSIS Standard System Reset
        }

        // B. Process non-blocking USART1 serial target input
        process_serial_input();

        // C. Update PID Position Control Loop every 10ms (non-blocking)
        if (millis() - last_pid_time >= PID_INTERVAL_MS) {
            last_pid_time = millis();
            pid_update(TimerPeriod);
        }

        // D. Periodic logging: full status when moving, only current when idle
        static uint32_t last_print_time = 0;
        if (millis() - last_print_time >= 100) { // Print every 100ms
            last_print_time = millis();
            if (status_reporting_enabled) {
                print_status();
            } else {
                uart_print("[CURRENT] ");
                uart_print_int(read_current_ma());
                uart_print(" mA\r\n");
            }
        }

        // E. Tiny stabilization delay to optimize loop frequency
        delay_ms(1);
    }
}
