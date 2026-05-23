/***********************************************************************************************************************
    @file    gpio_key_input.c
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
#define _GPIO_KEY_INPUT_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "gpio_key_input.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup GPIO
  * @{
  */

/**
  * @addtogroup GPIO_KEY_Input
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

    /* Config KEY1(PA2) Input Pull-Up/Pull-Down Mode */
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE2, 0x00U << GPIO_CRL_MODE2_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF2,  0x02U << GPIO_CRL_CNF2_Pos);
    /* Pull-Down */
    WRITE_REG(GPIOA->BRR, GPIO_BSRR_BR2);

    /* Config KEY2(PA9) Input Pull-Up/Pull-Down Mode */
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_MODE9, 0x00U << GPIO_CRH_MODE9_Pos);
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF9,  0x02U << GPIO_CRH_CNF9_Pos);
    /* Pull-Up */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS9);

    /* Config KEY3(PA8) Input Pull-Up/Pull-Down Mode */
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_MODE8, 0x00U << GPIO_CRH_MODE8_Pos);
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF8,  0x02U << GPIO_CRH_CNF8_Pos);
    /* Pull-Up */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS8);

    /* Config KEY4(PA3) Input Pull-Up/Pull-Down Mode */
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE3, 0x00U << GPIO_CRL_MODE3_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF3,  0x02U << GPIO_CRL_CNF3_Pos);
    /* Pull-Up */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS3);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void KEY_FSM_Handler(uint8_t *State, uint8_t *Count, uint8_t InputLevel, uint8_t ActiveLevel, char *Name)
{
    if (0 == *State)
    {
        if (InputLevel == ActiveLevel)
        {
            *Count += 1;

            if (*Count >= 5)
            {
                *State = 1;
                *Count = 0;
                printf("\r\n%s Pressed", Name);
            }
        }
        else
        {
            *Count = 0;
        }
    }
    else
    {
        if (InputLevel != ActiveLevel)
        {
            *Count += 1;

            if (*Count >= 5)
            {
                *State = 0;
                *Count = 0;
                printf("\r\n%s Release", Name);
            }
        }
        else
        {
            *Count = 0;
        }
    }
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void GPIO_KEY_Input_Sample(void)
{
    static uint8_t KeyState[4] =
    {
        0, 0, 0, 0
    };
    static uint8_t KeyCount[4] =
    {
        0, 0, 0, 0
    };

    printf("\r\nTest %s", __FUNCTION__);

    GPIO_Configure();

    printf("\r\nPress K1-K4...");

    while (1)
    {
        KEY_FSM_Handler(&KeyState[0], &KeyCount[0], READ_BIT(GPIOA->IDR, GPIO_IDR_IDR2) >> GPIO_IDR_IDR2_Pos, 1, "K1");
        KEY_FSM_Handler(&KeyState[1], &KeyCount[1], READ_BIT(GPIOA->IDR, GPIO_IDR_IDR9) >> GPIO_IDR_IDR9_Pos, 0, "K2");
        KEY_FSM_Handler(&KeyState[2], &KeyCount[2], READ_BIT(GPIOA->IDR, GPIO_IDR_IDR8) >> GPIO_IDR_IDR8_Pos, 0, "K3");
        KEY_FSM_Handler(&KeyState[3], &KeyCount[3], READ_BIT(GPIOA->IDR, GPIO_IDR_IDR3) >> GPIO_IDR_IDR3_Pos, 0, "K4");

        PLATFORM_LED_Enable(LED1, (FunctionalState)(READ_BIT(GPIOA->IDR, GPIO_IDR_IDR2) >> GPIO_IDR_IDR2_Pos));
        PLATFORM_LED_Enable(LED3, (FunctionalState)(READ_BIT(GPIOA->IDR, GPIO_IDR_IDR8) >> GPIO_IDR_IDR8_Pos));
        PLATFORM_LED_Enable(LED4, (FunctionalState)(READ_BIT(GPIOA->IDR, GPIO_IDR_IDR3) >> GPIO_IDR_IDR3_Pos));

        PLATFORM_DelayMS(10);
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

