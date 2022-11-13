/*
 * W25Qxx.c
 *
 *  Created on: June 14 2020
 *      Author: Alex
 */

#include "W25Qxx.h"

#define W25Qxx_DUMMY_BYTE         0xA5

typedef enum {
	CMD_WR_ENABLE_06		= 0x06,
	CMD_WR_DISABLE_04		= 0x04,
	CMD_STAT_R1_05			= 0x05,
	CMD_STAT_R2_35			= 0x35,
	CMD_JEDEC_ID_9F			= 0x9F,
	CMD_WR_STATUS_01		= 0x01,
	CMD_WR_VST_ENABLE_50	= 0x50,
	CMD_RD_DATA_03			= 0x03,
	CMD_RD_FAST_0B			= 0x0B,
	CMD_WR_PAGE_02			= 0x02,
	CMD_ERASE_SECTOR_20		= 0x20,
	CMD_ERASE_BL32K_52		= 0x52,
	CMD_ERASE_BL64K_D8		= 0xD8,
	CMD_ERASE_ALL_C7		= 0xC7,
	CMD_POWER_DOWN_B9		= 0xB9
} W25Qxx_CMD;

typedef enum {
	S_BUSY	= 0x0001,										// Device BUSY
	S_WEL	= 0x0002,										// Write Enable
	S_BP0	= 0x0004,										// Block write protection
	S_BP1	= 0x0008,
	S_BP2	= 0x0010,
	S_TB	= 0x0020,										// Top/Bottom protection
	S_SEC	= 0x0040,										// Sector/Block protection
	S_SRP0	= 0x0080,										// Status Register protect
	S_SRP1	= 0x0100,
	S_QE	= 0x0200,										// Quad Enable
	S_R		= 0x0400,
	S_LB1	= 0x0800,										// Security Lock
	S_LB2	= 0x1000,
	S_LB3	= 0x2000,
	S_CMP	= 0x4000,										// Component protect
	S_SUS	= 0x8000										// Suspend status
} W25Qxx_STATUS;

static uint16_t sector_count = 0;							// Number of the 4k sectors on the device

// Static function forward declarations
static void 	W25Qxx_Select(void);
static void 	W25Qxx_Unselect(void);
static void 	W25Qxx_Command(uint8_t cmd);
static uint16_t	W25Qxx_Status(bool r1_only);
static uint32_t W25Qxx_JEDEC_ID(void);
static bool		W25Wxx_WriteEnable(void);
static bool 	W25Qxx_ProgramPage(uint32_t addr, const uint8_t buff[256]);
static bool		W25Qxx_EraseSector(uint32_t addr);
static bool		W25Qxx_Wait(uint32_t to);
static bool		W25Qxx_IsSectorEmpty(uint32_t addr);

bool W25Qxx_Init(void) {
	W25Qxx_Unselect();
	HAL_Delay(100);
	if (W25Qxx_SectorCount() > 0) {
		W25Qxx_Command(CMD_WR_ENABLE_06);
		W25Qxx_Command(CMD_WR_VST_ENABLE_50);
		HAL_Delay(100);
		W25Qxx_Status(false);
	}
	return (sector_count > 0);
}

uint16_t W25Qxx_SectorCount(void) {
	if (sector_count == 0) {
		uint32_t id = W25Qxx_JEDEC_ID();
		if (((id & 0xFF0000) == 0xEF0000) && ((id & 0xFF00) == 0x4000)) { // Manufacturer ID and Memory type
			uint8_t capacity = id & 0xFF;
			if (capacity >= 0x11 && capacity <= 0xA1) {
				uint16_t blocks = 1 << (capacity - 0x10);
				sector_count = blocks << 4;					// blocks * 16
			}
		}
	}
	return sector_count;
}

// Read data from any address and any size; Usually read by 4k sectors
W25Qxx_RET W25Qxx_Read(uint32_t addr, uint8_t buff[], uint16_t size) {
	if (sector_count < (addr >> 12))						// addr / 4096
		return W25Qxx_RET_ADDR;
	if (size == 0)
		return W25Qxx_RET_SIZE;
	if (!W25Qxx_Wait(1000))									// Wait for device ready
		return W25Qxx_RES_BUSY;

	uint8_t cmd_length = 5;
	uint8_t cmd[6] = { CMD_RD_FAST_0B, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF, 0, 0 };
	if (sector_count >= 0x2000) {							// W25Q256 and more
		cmd[1] = (addr >> 24) & 0xFF;
		cmd[2] = (addr >> 16) & 0xFF;
		cmd[3] = (addr >> 8)  & 0xFF;
		cmd[4] = addr & 0xFF;
		cmd[5] = 0;
		cmd_length = 6;
	}
	bool status = W25Qxx_RET_READ;
	W25Qxx_Select();
	if (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, (uint8_t *)cmd, cmd_length, 100)) {
		if (HAL_OK == HAL_SPI_Receive(&FLASH_SPI_PORT, buff, size, 1000))
			status = W25Qxx_RET_OK;
	}
	W25Qxx_Unselect();
	return status;
}

// Write data by 256-bytes pages; Usually write whole 4k sector
W25Qxx_RET W25Qxx_Write(uint32_t addr, const uint8_t buff[], uint16_t size) {
	if (addr & 0xFF)										// Address should be aligned to the page border, divided by 256, i.e. 0xXXXXXX00
		return W25Qxx_RET_ALIGN;
	if (size < 0x100 || (size & 0xFF))
		return W25Qxx_RET_SIZE;
	if (sector_count < (addr >> 12))						// addr / 4096
		return W25Qxx_RET_ADDR;

	if (!W25Qxx_Wait(1000))									// Wait for device ready
		return W25Qxx_RES_BUSY;

	if (!W25Wxx_WriteEnable())								// Failed to enable write operation
		return W25Qxx_RES_RO;

	// If we are trying to write to the begin of the sector, check the sector has been erased
	if (((addr & 0xFFF) == 0) && !W25Qxx_IsSectorEmpty(addr))
		if (!W25Qxx_EraseSector(addr))
			return W25Qxx_RET_ERASE;

	if (!W25Qxx_Wait(5000))									// Wait for erase process to finish
		return W25Qxx_RES_BUSY;

	// Write data by 256-bytes long pages
	for (uint16_t start = 0; start < size; start += 256) {
		if (!W25Qxx_ProgramPage(addr, &buff[start]))
			return W25Qxx_RET_WRITE;
		addr += 256;
		if (!W25Qxx_Wait(1000))								// Wait for device ready
			return W25Qxx_RES_BUSY;
	}
	return W25Qxx_RET_OK;
}

W25Qxx_RET W25Qxx_Erase(uint16_t start_sector, uint16_t n_sectors) {
	if (n_sectors == 0)
		return W25Qxx_RET_SIZE;
	if (sector_count < (start_sector + n_sectors))			// 4k sectors
		return W25Qxx_RET_ADDR;
	if (!W25Qxx_Wait(5000))									// Wait for erase process to finish
		return W25Qxx_RES_BUSY;

	for (uint16_t i = 0; i < n_sectors; ++i) {
		uint32_t addr = (start_sector+i) << 12;				// 4k sector to byte address
		if (!W25Qxx_EraseSector(addr))
			return W25Qxx_RET_ERASE;

		if (!W25Qxx_Wait(5000))								// Wait for erase process to finish
			return W25Qxx_RES_BUSY;
	}
	return W25Qxx_RET_OK;
}

// Low-level functions

static void W25Qxx_Select(void) {
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void W25Qxx_Unselect(void) {
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

// Send single command to FLASH W25Q16
static void W25Qxx_Command(uint8_t cmd) {
	W25Qxx_Select();
	HAL_SPI_Transmit(&FLASH_SPI_PORT, &cmd, 1, 10);
	W25Qxx_Unselect();
}

static uint16_t W25Qxx_Status(bool r1_only) {
	uint8_t t_data[2] = { CMD_STAT_R1_05, W25Qxx_DUMMY_BYTE};
	uint8_t r_data[2] = {0};
	W25Qxx_Select();
	uint16_t stat = 0;
	if (HAL_OK == HAL_SPI_TransmitReceive(&FLASH_SPI_PORT, t_data, r_data, 2, 100)) {
		stat = r_data[1];
		if (!r1_only) {
			t_data[0] = CMD_STAT_R2_35;
			t_data[1] = W25Qxx_DUMMY_BYTE;
			if (HAL_OK == HAL_SPI_TransmitReceive(&FLASH_SPI_PORT, t_data, r_data, 2, 100)) {
				stat |= r_data[1] << 8;
			}
		}
	}
	W25Qxx_Unselect();
	return stat;
}

static uint32_t W25Qxx_JEDEC_ID(void) {
	uint8_t cmd = CMD_JEDEC_ID_9F;
	uint8_t buff[3];
	uint32_t id = 0;
	W25Qxx_Select();
	if (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, &cmd, 1, 100)) {
		if (HAL_OK == HAL_SPI_Receive(&FLASH_SPI_PORT, buff, 3, 1000)) {
			id = (buff[0] << 16) | (buff[1] << 8) | buff[2];
		}
	}
	W25Qxx_Unselect();
	return id;
}

static bool W25Wxx_WriteEnable(void)  {
	uint16_t stat = W25Qxx_Status(true);
	if ((stat & S_WEL) == 0) {								// Read only
		W25Qxx_Command(CMD_WR_ENABLE_06);					// Enable write access
		stat = W25Qxx_Status(true);
	}
	return (stat & S_WEL);
}

// Page size is 256 bytes
static bool W25Qxx_ProgramPage(uint32_t addr, const uint8_t buff[256]) {
	addr &= 0xFFFFFF00;										// Align to the page boundary. Page size is 256 bytes
	bool res = false;

	if (!W25Wxx_WriteEnable())
		return false;

	// Start address of page must be divided by 256, so the last byte of address is 0
	uint8_t cmd[5] = { CMD_WR_PAGE_02, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, 0, 0 };
	uint8_t cmd_length = 4;
	if (sector_count >= 0x2000) {							// W25Q256 and more
		cmd[1] = (addr >> 24) & 0xFF;
		cmd[2] = (addr >> 16) & 0xFF;
		cmd[3] = (addr >> 8)  & 0xFF;
		cmd[4] = 0;											// last byte of address is always zero
		cmd_length = 5;
	}
	W25Qxx_Select();
	if (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, (uint8_t *)cmd, cmd_length, 100)) {
		if (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, (uint8_t *)buff, 256, 1000)) {
			W25Qxx_Unselect();
			res = W25Qxx_Wait(1000);
		}
	}
	W25Qxx_Unselect();
	return res;
}

static bool W25Qxx_EraseSector(uint32_t addr) {
	if (!W25Wxx_WriteEnable())
		return false;

	// Align to the sector border, divided by 4096, i.e. 0xXXXXY000
	uint8_t cmd[4] = { CMD_ERASE_SECTOR_20, (addr >> 16) & 0xFF, (addr >> 8) & 0xF0, 0 };
	W25Qxx_Select();
	if (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, (uint8_t *)cmd, 4, 100)) {
		W25Qxx_Unselect();
		if (W25Qxx_Wait(10000)) {
			return true;
		}
	}
	W25Qxx_Unselect();
	return false;
}

static bool W25Qxx_Wait(uint32_t to) {
	uint32_t end = HAL_GetTick() + to;
	uint8_t t_data[2] = { CMD_STAT_R1_05, 0 };
	uint8_t r_data[2] = {0};
	while (1) {
		W25Qxx_Select();
		if (HAL_OK != HAL_SPI_TransmitReceive(&FLASH_SPI_PORT, t_data, r_data, 2, 100)) {
			W25Qxx_Unselect();
			return false;
		}
		W25Qxx_Unselect();
		if (r_data[1] & S_BUSY) {
			if (HAL_GetTick() >= end)
				return false;
			HAL_Delay(10);
		} else {
			return true;
		}
	}
	return false;
}

static bool W25Qxx_IsSectorEmpty(uint32_t addr) {
	addr &= 0xFFFFF000;										// Align to the sector border, divided by 4096, i.e. 0xXXXXY000
	uint8_t buff[32];										// The buffer to read data into
	for (uint16_t i = 0; i < 128; ++i) {					// 4K sector is 128 chunks of 32 bytes
		if (W25Qxx_RET_OK == W25Qxx_Read(addr, buff, 32)) {
			for (uint8_t j = 0; j < 32; ++j) {
				if (buff[j] != 0xFF) {
					return false;
				}
			}
		} else {
			return false;
		}
		addr += 32;
	}
	return true;
}
