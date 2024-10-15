/*
 * low_level.h
 *
 *  Created on: Nov 20, 2020
 *      Author: Alex
 *
 * Low-level functions to deal with TFT displays
 *
 */
#ifndef _LL_SPI_H
#define _LL_SPI_H

#include <stdbool.h>
#include "main.h"
#include "config.h"

#ifdef TFT_SPI_PORT

#ifdef __cplusplus
extern "C" {
#endif

void 		TFT_SPI_Reset(void);
void		TFT_SPI_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size);
void		TFT_SPI_DATA_MODE(void);
bool		TFT_SPI_ReadData(uint8_t cmd, uint8_t *data, uint16_t size);
void		TFT_SPI_ColorBlockInit(void);
void		TFT_SPI_ColorBlockSend_16bits(uint16_t color, uint32_t size);
void		TFT_SPI_ColorBlockSend_18bits(uint16_t color, uint32_t size);
void		TFT_SPI_ColorBlockFlush(void);

#ifdef __cplusplus
}
#endif

#endif 			// TFT_SPI_PORT
#endif			// _LL_SPI_H
