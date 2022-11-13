/*
 * W25Qxx.h
 *
 *  Created on: June 14 2020
 *      Author: Alex
 *
 *  W25QXX SPI flash IC driver. Tested on W25Q16 device at 42 Mbit/s SPI bus.
 *  Read can be performed from any available address,
 *  Write operation should be aligned to 256-byte page
 *
 *  Driver is working with FatFS library.
 *  As soon as data on the flash drive can be erased by sector (4k)
 *  block size of FAT file system should be setup as 4096 bytes
 *  Read and Write operations will work with 4K aligned blocks
 *
 *  Before used as disk device, the flash should be formated like this:
 *
 *  bool formatFlashDrive(void) {
 *	MKFS_PARM p;
 *	p.fmt		= FM_FAT | FM_SFD;
 *	p.au_size	= 4096;						// W25Q16 sector size
 *	p.align		= 0;						// 8
 *	p.n_fat		= 1;
 *	p.n_root	= 128;						// Number of files in root directory. Can be 256 or 512. 32 bytes per entry, 4096 bytes, 1 sector!
 *
 *	uint8_t *wrk = malloc(4096);
 *	if (wrk) {
 *		if (FR_OK == f_mkfs ("0:/", &p, wrk, 4096)) {
 *			free(wrk);
 *			return true;
 *		}
 *	}
 *	return false;
 *}
 *
 */

#ifndef _W25Qxx_H_
#define _W25Qxx_H_

#include "main.h"

#define FLASH_SPI_PORT		hspi2
extern SPI_HandleTypeDef 	FLASH_SPI_PORT;

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef enum {
	W25Qxx_RET_OK		= 0,
	W25Qxx_RET_ALIGN,
	W25Qxx_RET_SIZE,
	W25Qxx_RET_ADDR,
	W25Qxx_RET_ERASE,
	W25Qxx_RET_WRITE,
	W25Qxx_RET_READ,
	W25Qxx_RES_RO,
	W25Qxx_RES_BUSY
} W25Qxx_RET;

#ifdef __cplusplus
extern "C" {
#endif

bool		W25Qxx_Init(void);
uint16_t	W25Qxx_SectorCount(void);
W25Qxx_RET	W25Qxx_Read(uint32_t addr, uint8_t buff[], uint16_t size);
W25Qxx_RET	W25Qxx_Write(uint32_t addr, const uint8_t buff[], uint16_t size);
W25Qxx_RET	W25Qxx_Erase(uint16_t start_sector, uint16_t n_sectors);

#ifdef __cplusplus
}
#endif

#endif
