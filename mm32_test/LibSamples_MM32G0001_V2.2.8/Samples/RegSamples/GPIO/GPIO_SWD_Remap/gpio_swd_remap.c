/***********************************************************************************************************************
    @file    gpio_swd_remap.c
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
#define _GPIO_SWD_REMAP_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "gpio_swd_remap.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup GPIO
  * @{
  */

/**
  * @addtogroup GPIO_SWD_Remap
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
void GPIO_Configure(void)
{
    /* Enable GPIOA Clock */
    SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOA);

    /* Config PA13 AF7 */
    MODIFY_REG(GPIOA->AFRH, GPIO_AFRH_AFR13, 0x07U << GPIO_AFRH_AFR13_Pos);

    /* Config PA14 AF7 */
    MODIFY_REG(GPIOA->AFRH, GPIO_AFRH_AFR14, 0x07U << GPIO_AFRH_AFR14_Pos);

    /* Config PA13 General Purpose Output Push-Pull */
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_MODE13, 0x01U << GPIO_CRH_MODE13_Pos);
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF13,  0x00U << GPIO_CRH_CNF13_Pos);

    /* Config PA14 General Purpose Output Push-Pull */
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_MODE14, 0x01U << GPIO_CRH_MODE14_Pos);
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF14,  0x00U << GPIO_CRH_CNF14_Pos);

    /* Set PA13 */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS13);

    /* Set PA14 */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS14);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void GPIO_SWD_Remap_Sample(void)
{
    uint8_t i = 0;

    printf("\r\nTest %s", __FUNCTION__);

    printf("\r\nDownloadable programm...");

    for (i = 0; i < 10; i++)
    {
        PLATFORM_LED_Toggle(LED1);
        printf(".");

        PLATFORM_DelayMS(500);
    }

    GPIO_Configure();

    printf("\r\nCan't download program unless reset manually!");

    while (1)
    {
        /* PA13 Toggle */
        if (READ_BIT(GPIOA->ODR, GPIO_ODR_ODR13))
        {
            WRITE_REG(GPIOA->BRR, GPIO_BRR_BR13);
        }
        else
        {
            WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS13);
        }

        /* PA14 Toggle */
        if (READ_BIT(GPIOA->ODR, GPIO_ODR_ODR14))
        {
            WRITE_REG(GPIOA->BRR, GPIO_BRR_BR14);
        }
        else
        {
            WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS14);
        }

        PLATFORM_LED_Toggle(LED1);
        PLATFORM_DelayMS(100);
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

