/***********************************************************************************************************************
    @file    flash_optionbyte_erase_program.c
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
#define _FLASH_OPTIONBYTE_ERASE_PROGRAM_C_

/* Files include */
#include <stdio.h>
#include "platform.h"
#include "flash_optionbyte_erase_program.h"

/**
  * @addtogroup MM32G0001_RegSamples
  * @{
  */

/**
  * @addtogroup FLASH
  * @{
  */

/**
  * @addtogroup FLASH_OptionByte_Erase_Program
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
void FLASH_OptionByte_Erase_Program_Sample(void)
{
    printf("\r\nTest %s", __FUNCTION__);

    /* Unlock The Flash Memory */
    if (READ_BIT(FLASH->CR, FLASH_CR_LOCK))
    {
        WRITE_REG(FLASH->KEYR, 0x45670123);
        WRITE_REG(FLASH->KEYR, 0xCDEF89AB);
    }

    /* Unlock The Option Byte Memory */
    WRITE_REG(FLASH->OPTKEYR, 0x45670123);
    WRITE_REG(FLASH->OPTKEYR, 0xCDEF89AB);

    /* Enable Option Byte Erase */
    SET_BIT(FLASH->CR, FLASH_CR_OPTER);
    /* Write Flash Address Register */
    WRITE_REG(FLASH->AR, 0x1FFFF800U);
    /* Start Operation */
    SET_BIT(FLASH->CR, FLASH_CR_STRT);
    /* Wait Operation Complete */
    while(READ_BIT(FLASH->SR, FLASH_SR_BSY));
    /* Disable Option Byte Erase */
    CLEAR_BIT(FLASH->CR, FLASH_CR_OPTER);
    /* Clear Flash Status Register */
    WRITE_REG(FLASH->SR, FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR);

    /* Enable Option Byte Programming */
    SET_BIT(FLASH->CR, FLASH_CR_OPTPG);
    *(volatile uint16_t *)(0x1FFFF804) = 0x12;
    /* Wait Operation Complete */
    while(READ_BIT(FLASH->SR, FLASH_SR_BSY));
    /* Disable Option Byte Programming */
    CLEAR_BIT(FLASH->CR, FLASH_CR_OPTPG);
    /* Clear Flash Status Register */
    WRITE_REG(FLASH->SR, FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR);

    /* Enable Option Byte Programming */
    SET_BIT(FLASH->CR, FLASH_CR_OPTPG);
    *(volatile uint16_t *)(0x1FFFF806) = 0x34;
    /* Wait Operation Complete */
    while(READ_BIT(FLASH->SR, FLASH_SR_BSY));
    /* Disable Option Byte Programming */
    CLEAR_BIT(FLASH->CR, FLASH_CR_OPTPG);
    /* Clear Flash Status Register */
    WRITE_REG(FLASH->SR, FLASH_SR_EOP | FLASH_SR_WRPRTERR | FLASH_SR_PGERR);

    /* Lock The Flash Memory */
    SET_BIT(FLASH->CR, FLASH_CR_LOCK);

    printf("\r\nRead  OptionByte Data0 : 0x%02x", *((volatile uint16_t *)(0x1FFFF804)));
    printf("\r\nRead  OptionByte Data1 : 0x%02x", *((volatile uint16_t *)(0x1FFFF806)));

    while (1)
    {
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

