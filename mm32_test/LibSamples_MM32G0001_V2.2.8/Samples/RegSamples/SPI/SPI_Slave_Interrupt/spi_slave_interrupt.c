/***********************************************************************************************************************
    @file    spi_slave_interrupt.c
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
#define _SPI_SLAVE_INTERRUPT_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "spi_slave_interrupt.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup SPI
  * @{
  */

/**
  * @addtogroup SPI_Slave_Interrupt
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
void SPI_Configure(void)
{
    /* Enable SPI1 Clock */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI1);

    /* Only Valid In Low 8 Bit */
    CLEAR_BIT(SPI1->GCTL, SPI_GCTL_DW8_32);

    /* SPI Data Width Bit : 8 Bit Data */
    SET_BIT(SPI1->CCTL, SPI_CCTL_SPILEN);

    /* Slave Mode */
    CLEAR_BIT(SPI1->GCTL, SPI_GCTL_MODE);

    /* Data Transmission Or Receipt Highest Bit In Front : MSB */
    CLEAR_BIT(SPI1->CCTL, SPI_CCTL_LSBFE);

    /* Clock In Low Level In Idle Status */
    CLEAR_BIT(SPI1->CCTL, SPI_CCTL_CPOL);

    /* The First Data Bit Sampling Begins From The First Clock Edge */
    SET_BIT(SPI1->CCTL, SPI_CCTL_CPHA);

    /* Receipt Enable */
    SET_BIT(SPI1->GCTL, SPI_GCTL_RXEN);

    /* Transmission Enable */
    SET_BIT(SPI1->GCTL, SPI_GCTL_TXEN);

    /* Enable GPIOA Clock */
    SET_BIT(RCC->AHBENR, RCC_AHBENR_GPIOA);

    /* Config PA2 AF3 */
    MODIFY_REG(GPIOA->AFRL, GPIO_AFRL_AFR2, 0x03U << GPIO_AFRL_AFR2_Pos);   /* SPI1_MISO */

    /* Config PA2 Alternate Function Output Push-Pull */
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE2, 0x01U << GPIO_CRL_MODE2_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF2,  0x02U << GPIO_CRL_CNF2_Pos);

    /* Config PA8 AF0 */
    MODIFY_REG(GPIOA->AFRH, GPIO_AFRH_AFR8, 0x00U << GPIO_AFRH_AFR8_Pos);   /* SPI1_SCK */

    /* Config PA5 Input Floating Mode */
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_MODE8, 0x00U << GPIO_CRH_MODE8_Pos);
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF8,  0x01U << GPIO_CRH_CNF8_Pos);

    /* Config PA9 AF0 */
    MODIFY_REG(GPIOA->AFRH, GPIO_AFRH_AFR9, 0x00U << GPIO_AFRH_AFR9_Pos);   /* SPI1_MOSI */

    /* Config PA6 Input Pull-Up/Pull-Down Mode */
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_MODE9, 0x00U << GPIO_CRH_MODE9_Pos);
    MODIFY_REG(GPIOA->CRH, GPIO_CRH_CNF9,  0x02U << GPIO_CRH_CNF9_Pos);
    /* Pull-Up */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS9);

    /* Enable SYSCFG Clock */
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SYSCFG);

    /* Config PA0 Input Pull-Up/Pull-Down Mode */                           /* SPI1_NSS */
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE0, 0x00U << GPIO_CRL_MODE0_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF0,  0x02U << GPIO_CRL_CNF0_Pos);
    /* Pull-Up */
    WRITE_REG(GPIOA->BSRR, GPIO_BSRR_BS0);

    /* EXTI Line0 Bound To PortA */
    MODIFY_REG(SYSCFG->EXTICR1, SYSCFG_EXTICR1_EXTI0, 0x00U << SYSCFG_EXTICR1_EXTI0_Pos);
    /* Enable EXTI Line0 Interrupt */
    SET_BIT(EXTI->IMR, EXTI_IMR_IMR0);
    /* EXTI Line0 Rising Trigger */
    SET_BIT(EXTI->RTSR, EXTI_RTSR_TR0);
    /* EXTI Line0 Falling Trigger */
    SET_BIT(EXTI->FTSR, EXTI_FTSR_TR0);

    NVIC_SetPriority(EXTI0_1_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_1_IRQn);

    NVIC_SetPriority(SPI1_IRQn, 1);
    NVIC_EnableIRQ(SPI1_IRQn);

    /* SPI Enable */
    SET_BIT(SPI1->GCTL, SPI_GCTL_SPIEN);

    WRITE_REG(SPI1->TXREG, 0xFF);
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void SPI_RxTxData_Interrupt(uint8_t *rBuffer, uint8_t *wBuffer, uint8_t Length)
{
    uint8_t i = 0;

    for (i = 0; i < Length; i++)
    {
        SPI_RxStruct.Buffer[i] = 0;
        SPI_TxStruct.Buffer[i] = wBuffer[i];
    }

    SPI_RxStruct.Length = Length;
    SPI_RxStruct.CurrentCount = 0;
    SPI_RxStruct.CompleteFlag = 0;

    SPI_TxStruct.Length = Length;
    SPI_TxStruct.CurrentCount = 0;
    SPI_TxStruct.CompleteFlag = 0;

    /* Enable SPI Interrupt */
    SET_BIT(SPI1->GCTL, SPI_GCTL_INTEN);

    /* Receipt Data Interrupt Enable */
    SET_BIT(SPI1->INTEN, SPI_INTEN_RX_IEN);

    /* Transmission Buffer Enpty Interrupt Enable */
    SET_BIT(SPI1->INTEN, SPI_INTEN_TX_IEN);

    while (0 == SPI_TxStruct.CompleteFlag)
    {
    }

    while (0 == SPI_RxStruct.CompleteFlag)
    {
    }

    for (i = 0; i < Length; i++)
    {
        rBuffer[i] = SPI_RxStruct.Buffer[i];
    }
}

/***********************************************************************************************************************
  * @brief
  * @note   none
  * @param  none
  * @retval none
  *********************************************************************************************************************/
void SPI_Slave_Interrupt_Sample(void)
{
    uint8_t i = 0;
    uint8_t rBuffer[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t wBuffer[10] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA};

    printf("\r\nTest %s", __FUNCTION__);

    SPI_Configure();

    while (1)
    {
        printf("\r\nWait SPI master R/W...");

        SPI_RxTxData_Interrupt(rBuffer, wBuffer, 10);

        printf("\r\nRx Data : ");

        for(i = 0; i < 10; i++)
        {
            printf("0x%02X ", rBuffer[i]);
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

