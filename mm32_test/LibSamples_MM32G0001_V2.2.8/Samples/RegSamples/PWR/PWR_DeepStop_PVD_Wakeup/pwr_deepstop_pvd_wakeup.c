/***********************************************************************************************************************
    @file    pwr_deepstop_pvd_wakeup.c
    @author  FAE Team
    @date    18-Apr-2023
    @brief   THIS FILE PROVIDES ALL THE SYSTEM FUNCTIONS.
  **********************************************************************************************************************
    @attention

    <h2><center>&copy; Copyright(c) <2023> <MindMotion></center></h2>

      Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
    following conditions are met:
    1. Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
       the following disclaimer in the documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
       promote products derived from this software without specific prior written permission.

      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *********************************************************************************************************************/

/* Define to prevent recursive inclusion */
#define _PWR_DEEPSTOP_PVD_WAKEUP_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "pwr_deepstop_pvd_wakeup.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup PWR
  * @{
  */

/**
  * @addtogroup PWR_DeepStop_PVD_Wakeup
  * @{
  */

/* Private typedef ****************************************************************************************************/

/* Private define *****************************************************************************************************/

/* Private macro ******************************************************************************************************/

/* Private variables **************************************************************************************************/

/* Private functions **************************************************************************************************/

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void PWR_GPIO_Configure(void)
{
    /* Enable GPIOA Clock */
    SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOA);

    /* Enable GPIOB Clock */
    SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOB);

    /* Config GPIOA Analog Input Mode */
    WRITE_REG(GPIOA->CRL, 0x00000000);
    WRITE_REG(GPIOA->CRH, 0x04400000);

    /* Config GPIOB Analog Input Mode */
    WRITE_REG(GPIOB->CRL, 0x00000000);
    WRITE_REG(GPIOB->CRH, 0x00000000);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void PWR_PVD_Configure(void)
{
    /* Enable PWR Clock */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWR);

    /* PVD Level Selection : 3.3V */
    MODIFY_REG(PWR->CR, PWR_CR_PLS, 0x05U << PWR_CR_PLS_Pos);

    /* Enable EXTI Line16 Interrupt */
    SET_BIT(EXTI->IMR, EXTI_IMR_IMR16);
    /* EXTI Line16 Rising Trigger */
    SET_BIT(EXTI->RTSR, EXTI_RTSR_TR16);
    /* EXTI Line16 Falling Trigger */
    SET_BIT(EXTI->FTSR, EXTI_FTSR_TR16);

    NVIC_SetPriority(PVD_IRQn, 1);
    NVIC_EnableIRQ(PVD_IRQn);

    /* Enable PVD */
    SET_BIT(PWR->CR, PWR_CR_PVDE);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void PWR_Pretreatment(void)
{
    printf("\r\n-->Entry Deep Stop Mode");

    PLATFORM_DelayMS(10);

    PWR_GPIO_Configure();

    PWR_PVD_Configure();
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void PWR_Reinitialize(void)
{
    SystemInit();

    PLATFORM_Init();

    PLATFORM_DelayMS(100);

    printf("\r\n-->Wakeup By PVD");
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void PWR_DeepStopConfig(void)
{
    uint32_t systickreg = 0;
    uint32_t nvicReg = 0;
    uint32_t wrIcerReg = 0;
    uint32_t exti16Reg = 0;
    
    systickreg = SysTick->CTRL & 0x01;
    
    /* Enable PWR Clock */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWR);

    /* DeepStop Mode */
    SET_BIT(PWR->CR, PWR_CR_LPDS);

    /* Set SLEEPDEEP bit of Cortex System Control Register */
    SET_BIT(SCB->SCR,   SCB_SCR_SLEEPDEEP_Msk);

    /* By default, clear the enablement of all interrupts except EXTI0 - EXTI15. The interrupt lines with enablement above EXTI16 will maintain their original enabled states */
    nvicReg = NVIC->ISER[0];
    exti16Reg = EXTI->IMR;
    wrIcerReg = nvicReg;
    wrIcerReg &= ~((1 << EXTI0_1_IRQn)|(1 << EXTI2_3_IRQn)|(1 << EXTI4_15_IRQn));
    
    if(exti16Reg & (1 << 16))
    {
        wrIcerReg &= ~(1 << PVD_IRQn);
    }
    if(exti16Reg & (1 << 17))
    {
        wrIcerReg &= ~(1 << IWDG_IRQn);
    }
    NVIC->ICER[0] = wrIcerReg;
    
    
    /* Enable HSI */
    RCC->CR |= 0x1;
    while((RCC->CR & 0x2) == 0);
	
    /* To ensure that the instruction is deterministic after awakening, turn off the prefetch function before stop. */
    RCC->CFGR  &= ~(RCC_CFGR_HPRE_Msk | RCC_CFGR_SW_Msk);
    while((RCC->CFGR & RCC_CFGR_SWS_Msk) != 0);
	
    /* Before turning off prefetch, you need to clear the latency configuration */
    FLASH->ACR &= ~FLASH_ACR_LATENCY_Msk;
    FLASH->ACR &= ~FLASH_ACR_PRFTBE_Msk;
    while ((FLASH->ACR & 0x00000010) != 0)
    {
    }
    
    SysTick->CTRL &= 0xFFFFFFFE;

    /* RCC_HSIDLY register */
    *(__IO uint32_t *)(RCC_BASE + 0x44) = 0xFF;	
    
    /* Request Wait For Interrupt */
    __WFI();

    NVIC->ISER[0] = nvicReg;
    SysTick->CTRL |= systickreg ;
    
    /* Reset SLEEPDEEP bit of Cortex System Control Register */
    CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void PWR_DeepStop_PVD_Wakeup_Sample(void)
{
    uint8_t i = 0;

    printf("\r\nTest %s", __FUNCTION__);

    while (1)
    {
        printf("\r\n-->Running...");

        for (i = 0; i < 20; i++)
        {
            PLATFORM_LED_Toggle(LED1);
            PLATFORM_DelayMS(100);
        }

        PWR_Pretreatment();

        PWR_DeepStopConfig();

        PWR_Reinitialize();
    }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/********************************************** (C) Copyright MindMotion **********************************************/

