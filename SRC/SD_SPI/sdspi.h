/* vim: set ai et ts=4 sw=4: */
#ifndef _SDSPI_H_
#define _SPSPI_H__

#include "main.h"

#define SD_SPI_PORT      hspi2
extern SPI_HandleTypeDef SD_SPI_PORT;

typedef enum e_sd_type {
	TYPE_NOT_READY = 0, TYPE_SDSC, TYPE_SDHC
} SD_TYPE;

typedef struct s_SDCARD {
	SD_TYPE		type;
	uint32_t	blocks;
	uint32_t	erase_size;
	uint8_t		init_status;
} SDCARD;

typedef enum {
	CMD0_GO_IDLE_STATE			= 0,
	CMD8_SEND_IF_COND			= 8,
	CMD9_SEND_CSD				= 9,
	CMD12_STOP_TRANSMISSION		= 12,
	CMD16_SET_BLOCKLEN			= 16,
	CMD17_READ_SINGLE_BLOCK		= 17,
	CMD18_READ_MULTIPLE_BLOCK	= 18,
	CMD24_WRITE_BLOCK			= 24,
	CMD25_WRITE_MULTIPLE_BLOCK	= 25,
	CMD55_APP_CMD				= 0x37,
	ACMD41_APP_SEND_OP_COND		= 0x29,
	CMD58_READ_OCR				= 0x3a
} SPISD_CMD;

#ifdef __cplusplus
extern "C" {
#endif

// all procedures return 0 on success, > 0 on failure
uint8_t		SD_Init(SDCARD *sd);

uint8_t		SD_Read(SDCARD *sd, uint32_t start_block, uint8_t count, uint8_t* data);
uint8_t		SD_Write(SDCARD *sd, uint32_t start_block, uint8_t count, const uint8_t* data);

uint8_t 	SD_ReadSingleBlock(SDCARD *sd,  uint32_t block_num, uint8_t* data); // sizeof(data) == 512!
uint8_t		SD_WriteSingleBlock(SDCARD *sd, uint32_t block_num, const uint8_t* data); // sizeof(data) == 512!
uint8_t		SD_ReadBlocks(SDCARD *sd, uint32_t start_block, uint8_t count, uint8_t* data);
uint8_t		SD_WriteSBlocks(SDCARD *sd, uint32_t start_block, uint8_t count, const uint8_t* data);

#ifdef __cplusplus
}
#endif

#endif
