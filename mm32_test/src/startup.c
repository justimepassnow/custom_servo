#include <stdint.h>

extern uint32_t _sidata; /* Start of data in FLASH */
extern uint32_t _sdata;  /* Start of data in RAM */
extern uint32_t _edata;  /* End of data in RAM */
extern uint32_t _sbss;   /* Start of bss in RAM */
extern uint32_t _ebss;   /* End of bss in RAM */
extern uint32_t _estack; /* Top of Stack */

extern int main(void);
extern void SystemInit(void);

void Reset_Handler(void) {
    // 1. Copy initialized data segment from FLASH to RAM
    uint32_t *pSource = &_sidata;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSource++;
    }

    // 2. Zero-fill the uninitialized bss segment
    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    // 3. Call the system clock initialization
    SystemInit();

    // 4. Jump to the main function
    main();

    // 5. Catch-all loop
    while (1) {
    }
}

void Default_Handler(void) {
    while (1) {
    }
}

// Weak symbol mapping for interrupts
void NMI_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)          __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)      __attribute__((weak, alias("Default_Handler")));

// Vector Table (aligned and placed in .isr_vector section)
__attribute__((section(".isr_vector"), used))
void (* const g_pFnVectors[])(void) = {
    (void (*)(void))&_estack,  // Initial Stack Pointer
    Reset_Handler,             // Reset Handler
    NMI_Handler,               // NMI Handler
    HardFault_Handler,         // Hard Fault Handler
    0, 0, 0, 0, 0, 0, 0,       // Reserved
    SVC_Handler,               // SVCall Handler
    0, 0,                      // Reserved
    PendSV_Handler,            // PendSV Handler
    SysTick_Handler,           // SysTick Handler
};
