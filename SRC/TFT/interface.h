/*
 * interface.h
 *
 *  Created on: 4 Nov 2022
 *      Author: Alex
 */

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <stdbool.h>
#include "main.h"
#include "ll_spi.h"
#include "ll_fsmc.h"

// Hardware specific low-level function used to work with the display depending on the display interface type
typedef void 		(*t_TFT_Reset)(void);
typedef void		(*t_TFT_Command)(uint8_t cmd, const uint8_t* buff, size_t buff_size);
typedef void		(*t_TFT_Data_Mode)(void);
typedef bool		(*t_TFT_Read_Data)(uint8_t cmd, uint8_t *data, uint16_t size);
typedef void		(*t_TFT_Color_Block_Init)(void);
typedef void		(*t_TFT_Color_Block_Send)(uint16_t color, uint32_t size);
typedef void		(*t_TFT_Color_Block_Flush)(void);
typedef void 		(*t_TFT_Draw_Pixel)(uint16_t x,  uint16_t y, uint16_t color);

typedef struct {
	t_TFT_Reset				pReset;
	t_TFT_Command			pCommand;
	t_TFT_Data_Mode			pDataMode;
	t_TFT_Read_Data			pReadData;
	t_TFT_Color_Block_Init	pColorBlockInit;
	t_TFT_Color_Block_Send	pColorBlockSend;
	t_TFT_Color_Block_Flush	pColorBlockFlush;
	t_TFT_Draw_Pixel		pDrawPixel;
} tTFT_INT_FUNC;

typedef enum {
	TFT_16bits	= 0,
	TFT_18bits	= 1
} tTFT_PIXEL_BITS;

#ifdef __cplusplus
extern "C" {
#endif

// Function that setups connection
void		TFT_InterfaceSetup(tTFT_PIXEL_BITS data_size, tTFT_INT_FUNC *pINT);

// Interface functions
void		TFT_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size);
void		TFT_ColorBlockSend(uint16_t color, uint32_t size);
void		TFT_FinishDrawArea();
bool 		TFT_ReadData(uint8_t cmd, uint8_t *data, uint16_t size);
void 		TFT_DrawPixel_16bits(uint16_t x,  uint16_t y, uint16_t color);
void 		TFT_DrawPixel_18bits(uint16_t x,  uint16_t y, uint16_t color);

// Interface functions
void 		IFACE_DataMode(void);
void 		IFACE_ColorBlockInit(void);
void		IFACE_DrawPixel(uint16_t x,  uint16_t y, uint16_t color);

// Functions to manage display
void		TFT_DEF_Reset(void);
void 		TFT_DEF_SleepIn(void);
void 		TFT_DEF_SleepOut(void);
void 		TFT_DEF_InvertDisplay(bool on);
void		TFT_DEF_DisplayOn(bool on);
void		TFT_DEF_IdleMode(bool on);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_H_ */
