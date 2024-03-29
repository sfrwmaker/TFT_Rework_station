/*
 * W25Qxx.c
 *
 *  Created on: 2020 June 14
 *      Author: Alex
 *
 *  2023 Nov 11
 *      Added Quad SPI support.
 *
 *  2024 Feb 06
 *  	Added W25Q128 support
 *
 *  2024 Feb 10
 *  	Fixed QSPI support. Write enable now start working
 */

#include "W25Qxx.h"

#define W25Qxx_DUMMY_BYTE         0xA5

typedef enum {
	CMD_WR_STAT_01			= 0x01,
	CMD_WR_ENABLE_06		= 0x06,
	CMD_WR_DISABLE_04		= 0x04,
	CMD_STAT_R1_05			= 0x05,
	CMD_STAT_R2_35			= 0x35,
	CMD_JEDEC_ID_9F			= 0x9F,
	CMD_WR_STATUS_01		= 0x01,
	CMD_WR_VST_ENABLE_50	= 0x50,
	CMD_RD_DATA_03			= 0x03,
	CMD_RD_FAST_0B			= 0x0B,
	CMD_RD_FAST_QUAD_EB		= 0xEB,
	CMD_WR_PAGE_02			= 0x02,
	CMD_WR_PAGE_QUAD_32		= 0x32,
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
static bool			W25Qxx_WriteEnable(void);
static uint32_t		W25Qxx_JEDEC_ID(void);
static uint16_t		W25Qxx_Status(bool r1_only);
static bool			W25Qxx_Command(uint8_t cmd);
static bool			W25Qxx_EraseSector(uint32_t addr);
static bool			W25Qxx_ProgramPage(uint32_t addr, uint8_t buff[256]);
static bool			W25Qxx_EraseSector(uint32_t addr);
static bool			W25Qxx_Wait(uint32_t to);
static bool			W25Qxx_IsSectorEmpty(uint32_t addr);

#ifdef QSPI
static bool			W25Qxx_WriteStatusRegister(uint16_t status);
#else
static void			W25Qxx_Select(void);
static void			W25Qxx_Unselect(void);
#endif

// JEDDEC ID are a three bytes in double word:
// Manufacturer ID =  0xEF for winbond;
// Memory type: 0x40 - SPI mode, 0x60 - QPI mode (W25Q128); (0x40 & 0xDF) = (0x60 & 0xDF) = 0x40
// Capacity: 0x18 - 16MB (2^24), 0x17 - 8MB (2^23);
uint16_t W25Qxx_SectorCount(void) {
	if (sector_count == 0) {
		uint32_t id = W25Qxx_JEDEC_ID();
		// Check the manufacture ID and memory type
		if (((id & 0xFF0000) == 0xEF0000) && ((id & 0xDF00) == 0x4000)) {
			uint8_t capacity = id & 0xFF;
			if (capacity >= 0x11 && capacity <= 0xA1) {
				sector_count = 1 << (capacity - 12);
			}
		}
	}
	return sector_count;
}

#ifdef QSPI
bool W25Qxx_Init(void) {
	uint16_t stat = 0;
	if (W25Qxx_SectorCount() > 0) {
		W25Qxx_Command(CMD_WR_ENABLE_06);
		W25Qxx_Command(CMD_WR_VST_ENABLE_50);				// Write enable for Volatile status register
		HAL_Delay(100);
		stat = W25Qxx_Status(false);
		if ((stat & S_QE) == 0) {							// Quad SPI mode not activated yet
			stat |= S_QE;
			if (!W25Qxx_WriteStatusRegister(stat))
				return false;
		}
	}
	return (sector_count > 0);
}

// Read data from any address and any size; Usually read by 4k sectors
W25Qxx_RET W25Qxx_Read(uint32_t addr, uint8_t buff[], uint16_t size) {
	if (sector_count < (addr >> 12))						// addr / 4096
		return W25Qxx_RET_ADDR;
	if (size == 0)
		return W25Qxx_RET_SIZE;
	if (!W25Qxx_Wait(1000))									// Wait for device ready
		return W25Qxx_RES_BUSY;

	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_RD_FAST_QUAD_EB;
	scmd.NbData 			= size;							// Read data length.
	scmd.DataMode 			= QSPI_DATA_4_LINES;
	scmd.AddressMode 		= QSPI_ADDRESS_4_LINES;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;	// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 6;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= addr;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	W25Qxx_RET status = W25Qxx_RET_READ;
	if (HAL_OK == HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE)) {
		if (HAL_OK == HAL_QSPI_Receive(&FLASH_QSPI, buff, HAL_QSPI_TIMEOUT_DEFAULT_VALUE)) {
			status = W25Qxx_RET_OK;
		}
	}
	return status;
}

bool W25Qxx_QSPI_MemoryMapped(void) {
	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_RD_FAST_QUAD_EB;
	scmd.NbData 			= sector_count << 12;			// * 4096; Whole flash data size.
	scmd.DataMode 			= QSPI_DATA_4_LINES;			// Data line width
	scmd.AddressMode 		= QSPI_ADDRESS_4_LINES;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;	// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 6;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= 0;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	QSPI_MemoryMappedTypeDef	sMemMappedCfg = {0};
    sMemMappedCfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

    return (HAL_OK == HAL_QSPI_MemoryMapped(&FLASH_QSPI, &scmd, &sMemMappedCfg));
}

#else

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
	W25Qxx_RET status = W25Qxx_RET_READ;
	W25Qxx_Select();
	if (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, (uint8_t *)cmd, cmd_length, 100)) {
		if (HAL_OK == HAL_SPI_Receive(&FLASH_SPI_PORT, buff, size, 1000))
			status = W25Qxx_RET_OK;
	}
	W25Qxx_Unselect();
	return status;
}
#endif

// Write data by 256-bytes pages; Usually write whole 4k sector
W25Qxx_RET W25Qxx_Write(uint32_t addr, uint8_t buff[], uint16_t size) {
	if (addr & 0xFF)										// Address should be aligned to the page border, divided by 256, i.e. 0xXXXXXX00
		return W25Qxx_RET_ALIGN;
	if (size < 0x100 || (size & 0xFF))
		return W25Qxx_RET_SIZE;
	if (sector_count < (addr >> 12))						// addr / 4096
		return W25Qxx_RET_ADDR;

	if (!W25Qxx_Wait(1000))									// Wait for device ready
		return W25Qxx_RES_BUSY;

	if (!W25Qxx_WriteEnable())								// Failed to enable write operation
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

static bool W25Qxx_WriteEnable(void)  {
	uint16_t stat = W25Qxx_Status(true);
	if ((stat & S_WEL) == 0) {								// Read only
		if (W25Qxx_Command(CMD_WR_ENABLE_06))				// Enable write access
			stat = W25Qxx_Status(true);
	}
	return (stat & S_WEL);
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

#ifdef QSPI
static uint32_t W25Qxx_JEDEC_ID(void) {
	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_JEDEC_ID_9F;				// Read JDEC ID
	scmd.NbData 			= 3;							// Read 3 bytes
	scmd.DataMode 			= QSPI_DATA_1_LINE;				// Data line width
	scmd.AddressMode 		= QSPI_ADDRESS_NONE;
	scmd.AlternateByteMode	= QSPI_ALTERNATE_BYTES_NONE;	// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= 0;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	uint32_t id = 0;
	uint8_t buff[3];
	if (HAL_OK == HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE)) {
		if (HAL_OK == HAL_QSPI_Receive(&FLASH_QSPI, buff, HAL_QSPI_TIMEOUT_DEFAULT_VALUE)) {
			id = (buff[0] << 16) | (buff[1] << 8) | buff[2];
		}
	}
	return id;
}

// Reads the status register as two bytes word: R2:R1
static uint16_t W25Qxx_Status(bool r1_only) {
	uint16_t stat = 0;
	uint8_t  ans  = 0;
	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_STAT_R1_05;				// Read R1 status register
	scmd.NbData 			= 1;							// Read single byte.
	scmd.DataMode 			= QSPI_DATA_1_LINE;				// Data line width
	scmd.AddressMode 		= QSPI_ADDRESS_NONE;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;	// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= 0;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OK != HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
		return 0;
	if (HAL_OK != HAL_QSPI_Receive(&FLASH_QSPI, &ans, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
		return 0;
	stat = ans;
	if (!r1_only) {
		scmd.Instruction 		= CMD_STAT_R2_35;				// Read R2 status register
		scmd.NbData 			= 1;							// Read single byte.

		if (HAL_OK != HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
			return 0;
		if (HAL_OK != HAL_QSPI_Receive(&FLASH_QSPI, &ans, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
			return 0;
		stat |= (uint16_t)ans << 8;
	}
	return stat;
}

static bool W25Qxx_WriteStatusRegister(uint16_t status) {
	if (!W25Qxx_Wait(1000))										// Wait for device ready
		return false;
	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_WR_STAT_01;					// Write R1 status register
	scmd.NbData 			= 2;								// data length
	scmd.DataMode 			= QSPI_DATA_1_LINE;					// Data line width
	scmd.AddressMode 		= QSPI_ADDRESS_NONE;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;		// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_NONE;
	scmd.Address 			= 0;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OK != HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
		return false;
	uint8_t status_reg[2] = {status & 0xFF, status >> 8};
	return (HAL_OK == HAL_QSPI_Transmit(&FLASH_QSPI, status_reg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE));
}

// Send single command to FLASH W25Qxx
static bool W25Qxx_Command(uint8_t cmd) {
	QSPI_CommandTypeDef 	scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= cmd;
	scmd.NbData 			= 0;								// Read no data.
	scmd.DataMode 			= QSPI_DATA_NONE;					// No answer required
	scmd.AddressMode 		= QSPI_ADDRESS_NONE;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;		// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_NONE;
	scmd.Address 			= 0;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	return (HAL_OK == HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE));
}

// Page size is 256 bytes
static bool W25Qxx_ProgramPage(uint32_t addr, uint8_t buff[256]) {
	addr &= 0xFFFFFF00;											// Align to the page boundary. Page size is 256 bytes
	bool res = false;

	if (!W25Qxx_WriteEnable())
		return false;

	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_WR_PAGE_QUAD_32;				// Write Page
	scmd.NbData 			= 256;								// Write data length.
	scmd.DataMode 			= QSPI_DATA_4_LINES;				// Data line width
	scmd.AddressMode 		= QSPI_ADDRESS_1_LINE;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;		// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= addr;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OK == HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE)) {
		if (HAL_OK == HAL_QSPI_Transmit(&FLASH_QSPI, buff, HAL_QSPI_TIMEOUT_DEFAULT_VALUE)) {
			res = W25Qxx_Wait(1000);
		}
	}
	return res;
}

static bool W25Qxx_EraseSector(uint32_t addr) {
	if (sector_count < (addr >> 12))							// addr / 4096
		return false;
	if (!W25Qxx_WriteEnable())
		return false;

	// Align to the sector border, divided by 4096, i.e. 0xXXXXY000
	addr &= 0xFFFFF000;
	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_ERASE_SECTOR_20;				// Erase whole sector, 4096 bytes
	scmd.NbData 			= 0;
	scmd.DataMode 			= QSPI_DATA_NONE;
	scmd.AddressMode 		= QSPI_ADDRESS_1_LINE;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;		// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= addr;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OK != HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
		return false;
	if (W25Qxx_Wait(1000))						// Wait for device ready
		return true;
	return false;
}


static bool W25Qxx_Wait(uint32_t to) {
	uint32_t end = HAL_GetTick() + to;
	uint8_t stat_r1 = 0;
	QSPI_CommandTypeDef		scmd;
	scmd.InstructionMode	= QSPI_INSTRUCTION_1_LINE;
	scmd.Instruction 		= CMD_STAT_R1_05;					// Read status R1 register
	scmd.NbData 			= 1;								// Read data length.
	scmd.DataMode 			= QSPI_DATA_1_LINE;					// Data line width
	scmd.AddressMode 		= QSPI_ADDRESS_NONE;
	scmd.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_NONE;		// No multiplexing byte stage
	scmd.AlternateBytes 	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.AlternateBytesSize	= QSPI_ALTERNATE_BYTES_NONE;
	scmd.DummyCycles		= 0;
	scmd.AddressSize		= QSPI_ADDRESS_24_BITS;
	scmd.Address 			= 0;
	scmd.DdrMode			= QSPI_DDR_MODE_DISABLE;
	scmd.DdrHoldHalfCycle	= QSPI_DDR_HHC_ANALOG_DELAY;
	scmd.SIOOMode			= QSPI_SIOO_INST_EVERY_CMD;

	while (1) {
		if (HAL_OK != HAL_QSPI_Command(&FLASH_QSPI, &scmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
			return false;
		if (HAL_OK != HAL_QSPI_Receive(&FLASH_QSPI, &stat_r1, HAL_QSPI_TIMEOUT_DEFAULT_VALUE))
			return false;
		if (stat_r1 & S_BUSY) {
			if (HAL_GetTick() >= end)
				return false;
			HAL_Delay(10);
		} else {
			return true;
		}
	}
	return false;
}

#else

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

static bool W25Qxx_Command(uint8_t cmd) {
	W25Qxx_Select();
	bool ret = (HAL_OK == HAL_SPI_Transmit(&FLASH_SPI_PORT, &cmd, 1, 10));
	W25Qxx_Unselect();
	return ret;
}

// Page size is 256 bytes
static bool W25Qxx_ProgramPage(uint32_t addr, uint8_t buff[256]) {
	addr &= 0xFFFFFF00;						// Align to the page boundary. Page size is 256 bytes
	bool res = false;

	if (!W25Qxx_WriteEnable())
		return false;

	// Start address of page must be divided by 256, so the last byte of address is 0
	uint8_t cmd[5] = { CMD_WR_PAGE_02, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, 0, 0 };
	uint8_t cmd_length = 4;
	if (sector_count >= 0x2000) {			// W25Q256 and more
		cmd[1] = (addr >> 24) & 0xFF;
		cmd[2] = (addr >> 16) & 0xFF;
		cmd[3] = (addr >> 8)  & 0xFF;
		cmd[4] = 0;							// last byte of address is always zero
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
	if (!W25Qxx_WriteEnable())
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
static void W25Qxx_Select(void) {
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void W25Qxx_Unselect(void) {
	HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

#endif
