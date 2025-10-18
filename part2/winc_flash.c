/**
 *
 * \file
 *
 * \brief WINC1500 SPI Flash.
 *
 * Copyright (c) 2016-2021 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include "winc_flash.h"
#define DUMMY_REGISTER	(0x1084)

#define TIMEOUT (-1) /*MS*/

//#define DISABLE_UNSED_FLASH_FUNCTIONS

#define FLASH_BLOCK_SIZE					(32UL * 1024)
/*!<Block Size in Flash Memory
 */
#define FLASH_PAGE_SZ						(256)
/*!<Page Size in Flash Memory */

#define HOST_SHARE_MEM_BASE		(0xd0000UL)
#define CORTUS_SHARE_MEM_BASE	(0x60000000UL)
#define NMI_SPI_FLASH_ADDR		(0x111c)
/***********************************************************
SPI Flash DMA
***********************************************************/
#define GET_UINT32(X,Y)			(X[0+Y] + ((uint32_t)X[1+Y]<<8) + ((uint32_t)X[2+Y]<<16) +((uint32_t)X[3+Y]<<24))
#define SPI_FLASH_BASE			(0x10200)
#define SPI_FLASH_MODE			(SPI_FLASH_BASE + 0x00)
#define SPI_FLASH_CMD_CNT		(SPI_FLASH_BASE + 0x04)
#define SPI_FLASH_DATA_CNT		(SPI_FLASH_BASE + 0x08)
#define SPI_FLASH_BUF1			(SPI_FLASH_BASE + 0x0c)
#define SPI_FLASH_BUF2			(SPI_FLASH_BASE + 0x10)
#define SPI_FLASH_BUF_DIR		(SPI_FLASH_BASE + 0x14)
#define SPI_FLASH_TR_DONE		(SPI_FLASH_BASE + 0x18)
#define SPI_FLASH_DMA_ADDR		(SPI_FLASH_BASE + 0x1c)
#define SPI_FLASH_MSB_CTL		(SPI_FLASH_BASE + 0x20)
#define SPI_FLASH_TX_CTL		(SPI_FLASH_BASE + 0x24)

#define M2M_INFO(x,...)
#define M2M_ERR(x,...)
#define M2M_SUCCESS 0
#define M2M_ERR_INIT -5
#define M2M_ERR_FAIL -12

/*********************************************/
/* STATIC FUNCTIONS							 */
/*********************************************/

/**
*	@fn			spi_flash_read_status_reg
*	@brief		Read status register
*	@param[OUT]	val
					value of status reg
*	@return		Status of execution
*	@note		Compatible with MX25L6465E
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_read_status_reg(int fd, uint8_t * val)
{
	int8_t ret = M2M_SUCCESS;
	uint8_t cmd[1];
	uint32_t reg;

	cmd[0] = 0x05;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 4) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x01) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, DUMMY_REGISTER) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&reg))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(reg != 1);

	if (ret == M2M_SUCCESS)
    {
        if (!spi_read_reg(fd, DUMMY_REGISTER, &reg))
            ret = M2M_ERR_FAIL;
    }
	*val = (uint8_t)(reg & 0xff);
	return ret;
}

#ifdef DISABLE_UNSED_FLASH_FUNCTIONS
/**
*	@fn			spi_flash_read_security_reg
*	@brief		Read security register
*	@return		Security register value
*	@note		Compatible with MX25L6465E
*	@author		M. Abdelmawla
*	@version	1.0
*/
static uint8_t spi_flash_read_security_reg(int fd)
{
	uint8_t	cmd[1];
	uint32_t	reg;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x2b;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 1) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x01) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, DUMMY_REGISTER) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&reg))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(reg != 1);
	if (ret == M2M_SUCCESS)
    {
        if (!spi_read_reg(fd, DUMMY_REGISTER, &reg))
            ret = M2M_ERR_FAIL;
    }

	return (int8_t)reg & 0xff;
}

/**
*	@fn			spi_flash_gang_unblock
*	@brief		Unblock all flash area
*	@note		Compatible with MX25L6465E
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_gang_unblock(int fd)
{
	uint8_t	cmd[1];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x98;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x01) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}

/**
*	@fn			spi_flash_clear_security_flags
*	@brief		Clear all security flags
*	@note		Compatible with MX25L6465E
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_clear_security_flags(int fd)
{
	uint8_t cmd[1];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x30;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x01) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}
#endif

/**
*	@fn			spi_flash_load_to_cortus_mem
*	@brief		Load data from SPI flash into cortus memory
*	@param[IN]	u32MemAdr
*					Cortus load address. It must be set to its AHB access address
*	@param[IN]	u32FlashAdr
*					Address to read from at the SPI flash
*	@param[IN]	u32Sz
*					Data size
*	@return		Status of execution
*	@note		Compatible with MX25L6465E and should be working with other types
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_load_to_cortus_mem(int fd, uint32_t u32MemAdr, uint32_t u32FlashAdr, uint32_t u32Sz)
{
	uint8_t cmd[5];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x0b;
	cmd[1] = (uint8_t)(u32FlashAdr >> 16);
	cmd[2] = (uint8_t)(u32FlashAdr >> 8);
	cmd[3] = (uint8_t)(u32FlashAdr);
	cmd[4] = 0xA5;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, u32Sz) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]|(((uint32_t)cmd[1])<<8)|(((uint32_t)cmd[2])<<16)|(((uint32_t)cmd[3])<<24)) ||
        !spi_write_reg(fd, SPI_FLASH_BUF2, cmd[4]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x1f) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, u32MemAdr) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 5 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}

/**
*	@fn			spi_flash_sector_erase
*	@brief		Erase sector (4KB)
*	@param[IN]	u32FlashAdr
*					Any memory address within the sector
*	@return		Status of execution
*	@note		Compatible with MX25L6465E and should be working with other types
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_sector_erase(int fd, uint32_t u32FlashAdr)
{
	uint8_t cmd[4];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x20;
	cmd[1] = (uint8_t)(u32FlashAdr >> 16);
	cmd[2] = (uint8_t)(u32FlashAdr >> 8);
	cmd[3] = (uint8_t)(u32FlashAdr);

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]|(((uint32_t)cmd[1])<<8)|(((uint32_t)cmd[2])<<16)|(((uint32_t)cmd[3])<<24)) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x0f) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 4 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}

/**
*	@fn			spi_flash_write_enable
*	@brief		Send write enable command to SPI flash
*	@return		Status of execution
*	@note		Compatible with MX25L6465E and should be working with other types
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_write_enable(int fd)
{
	uint8_t cmd[1];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x06;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x01) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}

/**
*	@fn			spi_flash_write_disable
*	@brief		Send write disable command to SPI flash
*	@note		Compatible with MX25L6465E and should be working with other types
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_write_disable(int fd)
{
	uint8_t cmd[1];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;
	cmd[0] = 0x04;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x01) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}

/**
*	@fn			spi_flash_page_program
*	@brief		Write data (less than page size) from cortus memory to SPI flash
*	@param[IN]	u32MemAdr
*					Cortus data address. It must be set to its AHB access address
*	@param[IN]	u32FlashAdr
*					Address to write to at the SPI flash
*	@param[IN]	u32Sz
*					Data size
*	@note		Compatible with MX25L6465E and should be working with other types
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_page_program(int fd, uint32_t u32MemAdr, uint32_t u32FlashAdr, uint32_t u32Sz)
{
	uint8_t cmd[4];
	uint32_t	val	= 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x02;
	cmd[1] = (uint8_t)(u32FlashAdr >> 16);
	cmd[2] = (uint8_t)(u32FlashAdr >> 8);
	cmd[3] = (uint8_t)(u32FlashAdr);

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]|(((uint32_t)cmd[1])<<8)|(((uint32_t)cmd[2])<<16)|(((uint32_t)cmd[3])<<24)) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x0f) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, u32MemAdr) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 4 | (1<<7) | ((u32Sz & 0xfffff) << 8)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&val))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
	}
	while(val != 1);

	return ret;
}

/**
*	@fn			spi_flash_read_internal
*	@brief		Read from data from SPI flash
*	@param[OUT]	pu8Buf
*					Pointer to data buffer
*	@param[IN]	u32Addr
*					Address to read from at the SPI flash
*	@param[IN]	u32Sz
*					Data size
*	@note		Data size must be < 64KB (limitation imposed by the bus wrapper)
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_read_internal(int fd, uint8_t *pu8Buf, uint32_t u32Addr, uint32_t u32Sz)
{
	int8_t ret = M2M_SUCCESS;
	/* read size must be < 64KB */
	if (spi_flash_load_to_cortus_mem(fd, HOST_SHARE_MEM_BASE, u32Addr, u32Sz) != M2M_SUCCESS)
    {
        ret = M2M_ERR_FAIL;
        goto ERR;
    }
	if (!spi_read_data(fd, HOST_SHARE_MEM_BASE, pu8Buf, u32Sz))
    {
        ret = M2M_ERR_FAIL;
    }
ERR:
	return ret;
}

/**
*	@fn			spi_flash_pp
*	@brief		Program data of size less than a page (256 bytes) at the SPI flash
*	@param[IN]	u32Offset
*					Address to write to at the SPI flash
*	@param[IN]	pu8Buf
*					Pointer to data buffer
*	@param[IN]	u16Sz
*					Data size
*	@return		Status of execution
*	@author		M. Abdelmawla
*	@version	1.0
*/
static int8_t spi_flash_pp(int fd, uint32_t u32Offset, uint8_t *pu8Buf, uint16_t u16Sz)
{
	int8_t ret = M2M_SUCCESS;
	uint8_t tmp;

	if (spi_flash_write_enable(fd) != M2M_SUCCESS) {
		ret = M2M_ERR_FAIL;
		goto ERR;
	}

	/* use shared packet memory as temp mem */
	if (!spi_write_data(fd, HOST_SHARE_MEM_BASE, pu8Buf, u16Sz)) {
		ret = M2M_ERR_FAIL;
		goto ERR;
	}

	if (spi_flash_page_program(fd, HOST_SHARE_MEM_BASE, u32Offset, u16Sz) != M2M_SUCCESS) {
		ret = M2M_ERR_FAIL;
		goto ERR;
	}

	if (spi_flash_read_status_reg(fd, &tmp) != M2M_SUCCESS) {
		ret = M2M_ERR_FAIL;
		goto ERR;
	}

	do {
		if (spi_flash_read_status_reg(fd, &tmp) != M2M_SUCCESS) {
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
	} while(tmp & 0x01);

	if (spi_flash_write_disable(fd) != M2M_SUCCESS) {
		ret = M2M_ERR_FAIL;
	}

ERR:
	return ret;
}

/**
*	@fn			spi_flash_rdid
*	@brief		Read SPI Flash ID
*	@return		SPI FLash ID
*	@author		M.S.M
*	@version	1.0
*/
static uint32_t spi_flash_rdid(int fd)
{
	unsigned char cmd[1];
	uint32_t reg = 0;
	uint32_t cnt = 0;
	int8_t	ret = M2M_SUCCESS;

	cmd[0] = 0x9f;

	if (!spi_write_reg(fd, SPI_FLASH_DATA_CNT, 4) ||
        !spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]) ||
        !spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x1) ||
        !spi_write_reg(fd, SPI_FLASH_DMA_ADDR, DUMMY_REGISTER) ||
        !spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1<<7)))
    {
        ret = M2M_ERR_FAIL;
    }
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, (uint32_t *)&reg))
        {
            ret = M2M_ERR_FAIL;
            break;
        }
		if(++cnt > 500)
		{
			ret = M2M_ERR_INIT;
			break;
		}
	}
	while(reg != 1);
	if (ret == M2M_SUCCESS)
    {
        if (!spi_read_reg(fd, DUMMY_REGISTER, &reg))
            ret = M2M_ERR_FAIL;
    }
	M2M_PRINT("Flash ID %x \n",(unsigned int)reg);
	return reg;
}

/**
*	@fn			spi_flash_unlock
*	@brief		Unlock SPI Flash
*	@author		M.S.M
*	@version	1.0
*/
#if 0
static void spi_flash_unlock(int fd)
{
	uint8_t tmp;
	tmp = spi_flash_read_security_reg(fd);
	spi_flash_clear_security_flags(fd);
	if(tmp & 0x80)
	{
		spi_flash_write_enable(fd);
		spi_flash_gang_unblock(fd);
	}
}
#endif
static void spi_flash_enter_low_power_mode(int fd) {
	volatile unsigned long tmp;
	unsigned char* cmd = (unsigned char*) &tmp;
    uint32_t reg;
	cmd[0] = 0xb9;

	spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0);
	spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]);
	spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x1);
	spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0);
	spi_write_reg(fd, SPI_FLASH_CMD_CNT, 1 | (1 << 7));
	do
	{
		if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, &reg))
            break;
	} while (reg != 1);
}


static void spi_flash_leave_low_power_mode(int fd) {
	volatile unsigned long tmp;
	unsigned char* cmd = (unsigned char*) &tmp;
    uint32_t reg;
	cmd[0] = 0xab;

	spi_write_reg(fd, SPI_FLASH_DATA_CNT, 0);
	spi_write_reg(fd, SPI_FLASH_BUF1, cmd[0]);
	spi_write_reg(fd, SPI_FLASH_BUF_DIR, 0x1);
	spi_write_reg(fd, SPI_FLASH_DMA_ADDR, 0);
	spi_write_reg(fd, SPI_FLASH_CMD_CNT,  1 | (1 << 7));
    do
    {
        if (!spi_read_reg(fd, SPI_FLASH_TR_DONE, &reg))
            break;
    } while (reg != 1);
}
/*********************************************/
/* GLOBAL FUNCTIONS							 */
/*********************************************/
/**
 *	@fn		spi_flash_enable
 *	@brief	Enable spi flash operations
 *	@author	M. Abdelmawla
 *	@version	1.0
 */
#define REV_3A0 0x3a0

int8_t spi_flash_enable(int fd, uint8_t enable)
{
	int8_t s8Ret = M2M_SUCCESS;
	if(REV(chip_get_id(fd)) >= REV_3A0) {
		uint32_t u32Val;

		/* Enable pinmux to SPI flash. */
		if (!spi_read_reg(fd, 0x1410, &u32Val)) {
			s8Ret = M2M_ERR_FAIL;
			goto ERR1;
		}
		/* GPIO15/16/17/18 */
		u32Val &= ~((0x7777ul) << 12);
		if(enable) {
			u32Val |= ((0x1111ul) << 12);
			spi_write_reg(fd, 0x1410, u32Val);
			spi_flash_leave_low_power_mode(fd);
		} else {
			spi_flash_enter_low_power_mode(fd);
			/* Disable pinmux to SPI flash to minimize leakage. */
			u32Val |= ((0x0010ul) << 12);
			spi_write_reg(fd, 0x1410, u32Val);
		}
	}
ERR1:
	return s8Ret;
}
/**
*	@fn			spi_flash_read
*	@brief		Read from data from SPI flash
*	@param[OUT]	pu8Buf
*					Pointer to data buffer
*	@param[IN]	u32offset
*					Address to read from at the SPI flash
*	@param[IN]	u32Sz
*					Data size
*	@return		Status of execution
*	@note		Data size is limited by the SPI flash size only
*	@author		M. Abdelmawla
*	@version	1.0
*/
int8_t spi_flash_read(int fd, uint8_t *pu8Buf, uint32_t u32offset, uint32_t u32Sz)
{
	int8_t ret = M2M_SUCCESS;
	if(u32Sz > FLASH_BLOCK_SIZE)
	{
		do
		{
			ret = spi_flash_read_internal(fd, pu8Buf, u32offset, FLASH_BLOCK_SIZE);
			if(M2M_SUCCESS != ret) goto ERR;
			u32Sz -= FLASH_BLOCK_SIZE;
			u32offset += FLASH_BLOCK_SIZE;
			pu8Buf += FLASH_BLOCK_SIZE;
		} while(u32Sz > FLASH_BLOCK_SIZE);
	}

	ret = spi_flash_read_internal(fd, pu8Buf, u32offset, u32Sz);

ERR:
	return ret;
}

/**
*	@fn			spi_flash_write
*	@brief		Program SPI flash
*	@param[IN]	pu8Buf
*					Pointer to data buffer
*	@param[IN]	u32Offset
*					Address to write to at the SPI flash
*	@param[IN]	u32Sz
*					Data size
*	@return		Status of execution
*	@author		M. Abdelmawla
*	@version	1.0
*/
int8_t spi_flash_write(int fd, uint8_t* pu8Buf, uint32_t u32Offset, uint32_t u32Sz)
{
	int8_t ret = M2M_SUCCESS;
	uint32_t u32wsz;
	uint32_t u32off;
	uint32_t u32Blksz;
	u32Blksz = FLASH_PAGE_SZ;
	u32off = u32Offset % u32Blksz;
	if(u32Sz<=0)
	{
		M2M_ERR("Data size = %d",(int)u32Sz);
		ret = M2M_ERR_FAIL;
		goto ERR;
	}

	if (u32off)/*first part of data in the address page*/
	{
		u32wsz = u32Blksz - u32off;
		if(spi_flash_pp(fd, u32Offset, pu8Buf, (uint16_t)BSP_MIN(u32Sz, u32wsz))!=M2M_SUCCESS)
		{
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		if (u32Sz < u32wsz) goto EXIT;
		pu8Buf += u32wsz;
		u32Offset += u32wsz;
		u32Sz -= u32wsz;
	}
	while (u32Sz > 0)
	{
		u32wsz = BSP_MIN(u32Sz, u32Blksz);

		/*write complete page or the remaining data*/
		if(spi_flash_pp(fd, u32Offset, pu8Buf, (uint16_t)u32wsz)!=M2M_SUCCESS)
		{
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		pu8Buf += u32wsz;
		u32Offset += u32wsz;
		u32Sz -= u32wsz;
	}
EXIT:
ERR:
	return ret;
}

/**
*	@fn			spi_flash_erase
*	@brief		Erase from data from SPI flash
*	@param[IN]	u32Offset
*					Address to write to at the SPI flash
*	@param[IN]	u32Sz
*					Data size
*	@return		Status of execution
*	@note		Data size is limited by the SPI flash size only
*	@author		M. Abdelmawla
*	@version	1.0
*/
int8_t spi_flash_erase(int fd, uint32_t u32Offset, uint32_t u32Sz)
{
	uint32_t i = 0;
	int8_t ret = M2M_SUCCESS;
	uint8_t  tmp = 0;
	M2M_PRINT("\r\n>Start erasing...\r\n");
	for(i = u32Offset; i < (u32Sz +u32Offset); i += (16*FLASH_PAGE_SZ))
	{
		if (spi_flash_write_enable(fd) != M2M_SUCCESS) {
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		if (spi_flash_read_status_reg(fd, &tmp) != M2M_SUCCESS) {
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		if (spi_flash_sector_erase(fd, i + 10) != M2M_SUCCESS) {
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		if (spi_flash_read_status_reg(fd, &tmp) != M2M_SUCCESS) {
			ret = M2M_ERR_FAIL;
			goto ERR;
		}
		do
		{
			if (spi_flash_read_status_reg(fd, &tmp) != M2M_SUCCESS) {
				ret = M2M_ERR_FAIL;
				goto ERR;
			}
		}while(tmp & 0x01);

	}
	M2M_PRINT("Done\r\n");
ERR:
	return ret;
}

/**
*	@fn			spi_flash_get_size
*	@brief		Get size of SPI Flash
*	@return		Size of Flash
*	@author		M.S.M
*	@version	1.0
*/
uint32_t spi_flash_get_size(int fd)
{
	uint32_t u32FlashId = 0, u32FlashPwr = 0;
	static uint32_t gu32InternalFlashSize= 0;

	if(!gu32InternalFlashSize)
	{
		u32FlashId = spi_flash_rdid(fd);
		if((u32FlashId != 0xffffffff) && (u32FlashId !=0))
		{
			/*flash size is the third byte from the FLASH RDID*/
			u32FlashPwr = ((u32FlashId>>16)&0xff) - 0x11; /*2MBIT is the min*/
			/*That number power 2 to get the flash size*/
			gu32InternalFlashSize = 1<<u32FlashPwr;
			M2M_INFO("Flash Size %lu Mb\n",gu32InternalFlashSize);
		}
		else
		{
			M2M_ERR("Can't detect Flash size\n");
		}
	}

	return gu32InternalFlashSize;
}
