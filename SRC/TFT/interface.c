/*
 * interface.c
 *
 *  Created on: 4 Nov 2022
 *      Author: Alex
 */

#include "config.h"
#include "interface.h"
#include "common.h"

// Hardware specific low-level function used to work with the display depending on the display interface type
#ifdef TFT_SPI_PORT
static t_TFT_Reset				pReset				= TFT_SPI_Reset;
static t_TFT_Command			pCommand			= TFT_SPI_Command;
static t_TFT_Data_Mode			pDataMode			= TFT_SPI_DATA_MODE;
static t_TFT_Read_Data			pReadData			= TFT_SPI_ReadData;
static t_TFT_Color_Block_Init	pColorBlockInit		= TFT_SPI_ColorBlockInit;
static t_TFT_Color_Block_Send	pColorBlockSend		= TFT_SPI_ColorBlockSend_16bits;
static t_TFT_Color_Block_Flush	pColorBlockFlush	= TFT_SPI_ColorBlockFlush;
static t_TFT_Draw_Pixel			pDrawPixel			= TFT_DrawPixel_16bits;

// SPI Interface
void TFT_InterfaceSetup(tTFT_PIXEL_BITS data_size, tTFT_INT_FUNC *pINT) {
	if (pINT) {
		pReset				= (pINT->pReset)?pINT->pReset:TFT_SPI_Reset;
		pCommand			= (pINT->pCommand)?pINT->pCommand:TFT_SPI_Command;
		pDataMode			= (pINT->pDataMode)?pINT->pDataMode:TFT_SPI_DATA_MODE;
		pReadData			= (pINT->pReadData)?pINT->pReadData:TFT_SPI_ReadData;
		pColorBlockInit		= (pINT->pColorBlockInit)?pINT->pColorBlockInit:TFT_SPI_ColorBlockInit;
		pColorBlockFlush	= (pINT->pColorBlockFlush)?pINT->pColorBlockFlush:TFT_SPI_ColorBlockFlush;
		if (data_size == TFT_16bits) {
			pColorBlockSend	= (pINT->pColorBlockSend)?pINT->pColorBlockSend:TFT_SPI_ColorBlockSend_16bits;
			pDrawPixel		= (pINT->pDrawPixel)?pINT->pDrawPixel:TFT_DrawPixel_16bits;
		} else {
			pColorBlockSend	= (pINT->pColorBlockSend)?pINT->pColorBlockSend:TFT_SPI_ColorBlockSend_18bits;
			pDrawPixel		= (pINT->pDrawPixel)?pINT->pDrawPixel:TFT_DrawPixel_18bits;
		}
	} else {
		pColorBlockSend		=	(data_size == TFT_16bits)?TFT_SPI_ColorBlockSend_16bits:TFT_SPI_ColorBlockSend_18bits;
		pDrawPixel			=	(data_size == TFT_16bits)?TFT_DrawPixel_16bits:TFT_DrawPixel_18bits;
	}
}
#endif

#ifdef FSMC_LCD_CMD
static t_TFT_Reset				pReset				= TFT_FSMC_Reset;
static t_TFT_Command			pCommand			= TFT_FSMC_Command;
static t_TFT_Data_Mode			pDataMode			= TFT_FSMC_DATA_MODE;
static t_TFT_Read_Data			pReadData			= TFT_FSMC_ReadData;
static t_TFT_Color_Block_Init	pColorBlockInit		= TFT_FSMC_ColorBlockInit;
static t_TFT_Color_Block_Send	pColorBlockSend		= TFT_FSMC_ColorBlockSend_16bits;
static t_TFT_Color_Block_Flush	pColorBlockFlush	= TFT_FSMC_ColorBlockFlush;
static t_TFT_Draw_Pixel			pDrawPixel			= TFT_DrawPixel_16bits;

// FSMC Interface
void TFT_InterfaceSetup(tTFT_PIXEL_BITS data_size, tTFT_INT_FUNC *pINT) {
	if (pINT) {
		pReset				= (pINT->pReset)?pINT->pReset:TFT_FSMC_Reset;
		pCommand			= (pINT->pCommand)?pINT->pCommand:TFT_FSMC_Command;
		pDataMode			= (pINT->pDataMode)?pINT->pDataMode:TFT_FSMC_DATA_MODE;
		pReadData			= (pINT->pReadData)?pINT->pReadData:TFT_FSMC_ReadData;
		pColorBlockInit		= (pINT->pColorBlockInit)?pINT->pColorBlockInit:TFT_FSMC_ColorBlockInit;
		pColorBlockFlush	= (pINT->pColorBlockFlush)?pINT->pColorBlockFlush:TFT_FSMC_ColorBlockFlush;
		if (data_size == TFT_16bits) {
			pColorBlockSend	= (pINT->pColorBlockSend)?pINT->pColorBlockSend:TFT_FSMC_ColorBlockSend_16bits;
			pDrawPixel		= (pINT->pDrawPixel)?pINT->pDrawPixel:TFT_FSMC_DrawPixel_16bits;
		} else {
			pColorBlockSend	= (pINT->pColorBlockSend)?pINT->pColorBlockSend:TFT_FSMC_ColorBlockSend_18bits;
			pDrawPixel		= (pINT->pDrawPixel)?pINT->pDrawPixel:TFT_FSMC_DrawPixel_18bits;
		}
	} else {
		pColorBlockSend		=	(data_size == TFT_16bits)?TFT_FSMC_ColorBlockSend_16bits:TFT_FSMC_ColorBlockSend_18bits;
		pDrawPixel			=	(data_size == TFT_16bits)?TFT_FSMC_DrawPixel_16bits:TFT_FSMC_DrawPixel_18bits;
	}
}
#endif

void TFT_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size) {
	(*pCommand)(cmd, buff, buff_size);
}

void IFACE_DataMode(void) {
	(*pDataMode)();
}

void IFACE_ColorBlockInit(void) {
	(*pColorBlockInit)();
}

void TFT_ColorBlockSend(uint16_t color, uint32_t size) {
	(*pColorBlockSend)(color, size);
}

void TFT_FinishDrawArea(void) {
	(*pColorBlockFlush)();								// Flush color block buffer
}

void IFACE_DrawPixel(uint16_t x,  uint16_t y, uint16_t color) {
	(*pDrawPixel)(x, y, color);
}

bool TFT_ReadData(uint8_t cmd, uint8_t *data, uint16_t size) {
	return (*pReadData)(cmd, data, size);
}

void TFT_DEF_Reset(void) {
	(*pReset)();
}

// Turn display to the sleep mode. Make sure particular display command is the same
void TFT_DEF_SleepIn(void) {
	(*pCommand)(0x10, 0, 0);
	TFT_Delay(5);
}

// Deactivates sleep mode. Make sure particular display command is the same
void TFT_DEF_SleepOut(void) {
	(*pCommand)(0x11, 0, 0);
	TFT_Delay(120);
}

// Turn display to Inversion mode. Make sure particular display command is the same
void TFT_DEF_InvertDisplay(bool on) {
	uint8_t cmd = on?0x21:0x20;
	(*pCommand)(cmd, 0, 0);
}

// Turn display on or off. Make sure particular display command is the same
void TFT_DEF_DisplayOn(bool on) {
	uint8_t cmd = on?0x29:0x28;
	(*pCommand)(cmd, 0, 0);
}

// Turn display to idle/normal mode. Make sure particular display command is the same
void TFT_DEF_IdleMode(bool on) {
	uint8_t cmd = on?0x39:0x38;
	(*pCommand)(cmd, 0, 0);
}

// Draws single pixel using display working in 16 bits mode
void TFT_DrawPixel_16bits(uint16_t x,  uint16_t y, uint16_t color) {
	if ((x >=TFT_Width()) || (y >=TFT_Height())) return;
	uint8_t clr[2] = { (color>>8) & 0xFF, color & 0xFF };
	TFT_SetAttrWindow(x, y, x, y);
	TFT_Command(0x2C, clr, 2);
}

// Draws single pixel using display working in 18 bits SPI mode (ILI9488 supports 3 bits or 18 bits per pixel in SPI mode)
void TFT_DrawPixel_18bits(uint16_t x,  uint16_t y, uint16_t color) {
	if ((x >=TFT_Width()) || (y >=TFT_Height())) return;
	uint8_t clr[3] = { (color & 0xF800) >> 8, (color & 0x7E0) >> 3, (color & 0x1F) << 3 };
	TFT_SetAttrWindow(x, y, x, y);
	TFT_Command(0x2C, clr, 3);						// write to RAM
}
