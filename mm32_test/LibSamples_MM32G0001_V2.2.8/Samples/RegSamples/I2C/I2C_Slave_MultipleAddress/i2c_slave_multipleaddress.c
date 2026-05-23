/***********************************************************************************************************************
    @file    i2c_slave_multipleaddress.c
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
#define _I2C_SLAVE_MULTIPLEADDRESS_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "i2c_slave_multipleaddress.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup I2C
  * @{
  */

/**
  * @addtogroup I2C_Slave_MultipleAddress
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
void I2C_Configure(void)
{
    /* Enable I2C1 Clock */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_I2C1);

    /* I2C Module Disable */
    CLEAR_BIT(I2C1->ENR, I2C_ENR_ENABLE);

    /* 7-bit Addreesing */
    CLEAR_BIT(I2C1->CR, I2C_CR_SLAVE10);

    /* The Slave Address Of I2C */
    MODIFY_REG(I2C1->SAR, I2C_SAR_ADDR, 0x58 << I2C_SAR_ADDR_Pos);              // 0x58  -> 00 0101 8000

    /* Slave Address Mask */
    MODIFY_REG(I2C1->SLVMASK, I2C_SLVMASK_MASK, 0x3EFU << I2C_SLVMASK_MASK_Pos);// 0x3EF -> 11 1110 1111 -> 0x58/0x48

    /* Slave Enable */
    CLEAR_BIT(I2C1->CR, I2C_CR_DISSLAVE);

    /* Master Disable */
    CLEAR_BIT(I2C1->CR, I2C_CR_MASTER);

    /* Enable GPIOA Clock */
    SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOA);

    /* Config PA0 AF3 */
    MODIFY_REG(GPIOA->AFRL, GPIO_AFRL_AFR0, 0x03U << GPIO_AFRL_AFR0_Pos);

    /* Config PA0 Alternate Function Output Open-Draw */
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE0, 0x01U << GPIO_CRL_MODE0_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF0, 0x03U << GPIO_CRL_CNF0_Pos);

    /* Config PA4 AF3 */
    MODIFY_REG(GPIOA->AFRL, GPIO_AFRL_AFR4, 0x03U << GPIO_AFRL_AFR4_Pos);

    /* Config PA4 Alternate Function Output Open-Draw */
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE4, 0x01U << GPIO_CRL_MODE4_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF4, 0x03U << GPIO_CRL_CNF4_Pos);

    /* I2C Module Enable */
    SET_BIT(I2C1->ENR, I2C_ENR_ENABLE);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void I2C_Slave_MultipleAddress_Sample(void)
{
    uint8_t i = 0, Buffer[10];
    uint8_t RxLength = 0, TxLength = 0;

    printf("\r\nTest %s", __FUNCTION__);

    I2C_Configure();

    while (1)
    {
        /* Receive Buffer Full */
        if(READ_BIT(I2C1->RAWISR, I2C_RAWISR_RX_FULL))
        {
            /* Read Data */
            Buffer[RxLength++] = READ_BIT(I2C1->DR, I2C_DR_DAT) >> I2C_DR_DAT_Pos;

            if(RxLength == 10)
            {
                /* Abort Operation In Progress */
                SET_BIT(I2C1->ENR, I2C_ENR_ABORT);

                /* Abort Operation In Progress */
                while (READ_BIT(I2C1->ENR, I2C_ENR_ABORT))
                {
                }

                /* Clear The TX_ABRT */
                READ_BIT(I2C1->TX_ABRT, I2C_TX_ABRT_TX_ABRT);

                /* STOP Condition Detection */
                while (0 == READ_BIT(I2C1->RAWISR, I2C_RAWISR_STOP))
                {
                }

                TxLength = 0;

                printf("\r\nI2C Slave[0x%X] Receive : ", I2C1->SLVRCVADDR);

                for(i = 0; i < RxLength; i++)
                {
                    printf("0x%02X ", Buffer[i]);
                }
            }
        }

        /* Read Request */
        if(READ_BIT(I2C1->RAWISR, I2C_RAWISR_RD_REQ))
        {
            /* Write Data */
            MODIFY_REG(I2C1->DR, I2C_DR_DAT, Buffer[TxLength++] << I2C_DR_DAT_Pos);

            /* Reading This Rgister Clears The RD_REQ Interrupt */
            READ_BIT(I2C1->RD_REQ, I2C_RD_REQ_RD_REQ);

            if(TxLength == RxLength)
            {
                RxLength = 0;

                printf("\r\nI2C Slave Send Finish.");
            }
        }
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

