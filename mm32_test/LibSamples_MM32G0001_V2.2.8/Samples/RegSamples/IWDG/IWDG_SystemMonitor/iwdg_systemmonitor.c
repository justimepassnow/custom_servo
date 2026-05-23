/***********************************************************************************************************************
    @file    iwdg_systemmonitor.c
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
#define _IWDG_SYSTEMMONITOR_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "iwdg_systemmonitor.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup IWDG
  * @{
  */

/**
  * @addtogroup IWDG_SystemMonitor
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
void IWDG_Configure(uint32_t Timeout)
{
    uint16_t Reload = LSI_VALUE / 1000 * Timeout / 32;

    /* Enable LSI Clock */
    SET_BIT(RCC->CSR, RCC_CSR_LSION);

    /* Wait LSI Ready */
    while (READ_BIT(RCC->CSR, RCC_CSR_LSIRDY) == 0)
    {
    }

    /* Wait Watchdog Prescaler Value Update Completed */
    while(READ_BIT(IWDG->SR, IWDG_SR_PVU));
    /* Disable The Protection And Enable Access To Other Configuration Registers */
    WRITE_REG(IWDG->KR, 0x5555);
    /* IWDG Prescaler Divider */
    MODIFY_REG(IWDG->PR, IWDG_PR_PR, 0x03U << IWDG_PR_PR_Pos);
    /* Wait Watchdog Prescaler Value Update Completed */
    while(READ_BIT(IWDG->SR, IWDG_SR_PVU));

    /* Wait Watchdog Counter Reload Value Update Completed */
    while(READ_BIT(IWDG->SR, IWDG_SR_RVU));
    /* Disable The Protection And Enable Access To Other Configuration Registers */
    WRITE_REG(IWDG->KR, 0x5555);
    /* Watchdog Counter Reload Value */
    MODIFY_REG(IWDG->RLR, IWDG_RLR_RL, Reload << IWDG_RLR_RL_Pos);
    /* Wait Watchdog Counter Reload Value Update Completed */
    while(READ_BIT(IWDG->SR, IWDG_SR_PVU));

    /* Feed The Dog */
    WRITE_REG(IWDG->KR, 0xAAAA);

    /* Start The Watchdog */
    WRITE_REG(IWDG->KR, 0xCCCC);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void IWDG_SystemMonitor_Sample(void)
{
    printf("\r\nTest %s", __FUNCTION__);

    IWDG_Configure(500);

    while (1)
    {
        PLATFORM_LED_Toggle(LED1);
        PLATFORM_DelayMS(100);

        /* Feed The Dog */
        WRITE_REG(IWDG->KR, 0xAAAA);
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

