/*
 * ll_fsmc.h
 *
 *  Created on: 4 Nov 2022
 *      Author: Alex
 */

#ifndef _LL_FSMC_H_
#define _LL_FSMC_H_

#include <stdbool.h>
#include "main.h"
#include "config.h"

#ifdef FSMC_LCD_CMD

#ifdef __cplusplus
extern "C" {
#endif

void 		TFT_FSMC_Reset(void);
void		TFT_FSMC_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size);
void		TFT_FSMC_NT35510_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size);
void		TFT_FSMC_DrawPixel_16bits(uint16_t x,  uint16_t y, uint16_t color);
void		TFT_FSMC_DrawPixel_18bits(uint16_t x,  uint16_t y, uint16_t color);
void		TFT_FSMC_HIGH_DrawPixel_16bits(uint16_t x,  uint16_t y, uint16_t color);
void		TFT_FSMC_HIGH_DrawPixel_18bits(uint16_t x,  uint16_t y, uint16_t color);
void		TFT_FSMC_DATA_MODE(void);
bool		TFT_FSMC_ReadData(uint8_t cmd, uint8_t *data, uint16_t size);
void		TFT_FSMC_ColorBlockInit(void);
void		TFT_FSMC_ColorBlockSend_16bits(uint16_t color, uint32_t size);
void		TFT_FSMC_ColorBlockSend_18bits(uint16_t color, uint32_t size);
void		TFT_FSMC_ColorBlockFlush(void);

#ifdef __cplusplus
}
#endif

#endif				// FSMC_LCD_CMD

#endif 				// _LL_FSMC_H_
