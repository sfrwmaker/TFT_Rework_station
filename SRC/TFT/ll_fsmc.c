/*
 * ll_fsmc.c
 *
 *  Created on: 4 Nov 2022
 *      Author: Alex
 */

#include "ll_fsmc.h"

#ifdef FSMC_LCD_CMD

#ifdef FSMC_8_BITS
	static uint8_t *LCD_CMD_PORT 	= (uint8_t *)FSMC_LCD_CMD;
	static uint8_t *LCD_DATA_PORT	= (uint8_t *)(FSMC_LCD_CMD | FSMC_DATA_MASK);
#else
	static uint16_t *LCD_CMD_PORT 	= (uint16_t *)FSMC_LCD_CMD;
	static uint16_t *LCD_DATA_PORT	= (uint16_t *)(FSMC_LCD_CMD | (FSMC_DATA_MASK) << 1);
#endif

// Hard reset pin management
static void TFT_FSMC_RST(GPIO_PinState state) {
	HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, state);
}

// HARDWARE RESET
void TFT_FSMC_Reset(void) {
	TFT_FSMC_RST(GPIO_PIN_RESET);
	HAL_Delay(200);
	TFT_FSMC_RST(GPIO_PIN_SET);
}

static void TFT_FSMC_SetAttrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	uint8_t data[4];

	// column address set
	data[0] = (x0 >> 8) & 0xFF;
	data[1] = x0 & 0xFF;
	data[2] = (x1 >> 8) & 0xFF;
	data[3] = x1 & 0xFF;
	TFT_FSMC_Command(0x2A, data, 4);

	// row address set
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    TFT_FSMC_Command(0x2B, data, 4);
}

#ifdef FSMC_8_BITS
// Send command (byte) to the Display followed by the command arguments
void TFT_FSMC_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size) {
	*LCD_CMD_PORT = cmd;
	if (buff && buff_size > 0) {
		for (uint16_t i = 0; i <  buff_size; ++i) {
			*LCD_DATA_PORT	= buff[i];
		}
	}
}

bool TFT_FSMC_ReadData(uint8_t cmd, uint8_t *data, uint16_t size) {
	if (!data || size == 0) return 0;

	*LCD_CMD_PORT = cmd;
	for (uint16_t i = 0; i < size; ++i) {
		data[i] = *LCD_DATA_PORT;
	}
	return true;
}

void TFT_FSMC_ColorBlockSend_16bits(uint16_t color, uint32_t size) {
	uint8_t clr_hi = color >> 8;
	uint8_t clr_lo = color & 0xff;
	for (uint32_t i = 0; i < size; ++i) {
		*LCD_DATA_PORT = clr_hi;
		*LCD_DATA_PORT = clr_lo;
	}
}

void TFT_FSMC_ColorBlockSend_18bits(uint16_t color, uint32_t size) {
	// Convert to 18-bits color
	uint8_t r = (color & 0xF800) >> 8;
	uint8_t g = (color & 0x7E0)  >> 3;
	uint8_t b = (color & 0x1F)   << 3;
	for (uint32_t i = 0; i < size; ++i) {
		*LCD_DATA_PORT = r;
		*LCD_DATA_PORT = g;
		*LCD_DATA_PORT = b;
	}
}
#endif

#ifdef FSMC_16_BITS
// Send command (byte) to the Display followed by the command arguments
void TFT_FSMC_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size) {
	*LCD_CMD_PORT = cmd;									// First send the command
	if (buff && buff_size > 0) {
		for (uint16_t i = 0; i <  buff_size; ++i) {			// Then send the arguments (if they are exist)
			*LCD_DATA_PORT	= buff[i];
		}
	}
}

void TFT_FSMC_NT35510_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size) {
	uint16_t f_cmd = cmd << 8;
	if (buff && buff_size > 0) {
		for (uint16_t i = 0; i <  buff_size; ++i) {
			*LCD_CMD_PORT	= f_cmd ++;
			*LCD_DATA_PORT	= buff[i];
		}
	} else {
		*LCD_CMD_PORT	= f_cmd;							// Single command without argument
	}
}

// Draws single pixel using display working in 16 bits mode
void TFT_FSMC_DrawPixel_16bits(uint16_t x,  uint16_t y, uint16_t color) {
	TFT_FSMC_SetAttrWindow(x, y, x, y);
	*LCD_CMD_PORT	= 0x2C;
	*LCD_DATA_PORT	= color;
}

// Draws single pixel using display working in 18 bits mode
void TFT_FSMC_DrawPixel_18bits(uint16_t x,  uint16_t y, uint16_t color) {
	uint16_t rg = (color & 0xF800);
	rg |= (color & 0x7E0)  >> 3;
	uint16_t b = (color & 0x1F)   << 11;
	TFT_FSMC_SetAttrWindow(x, y, x, y);
	*LCD_CMD_PORT	= 0x2C;
	*LCD_DATA_PORT	= rg;
	*LCD_DATA_PORT 	= b;
}

// Draws single pixel using display working in 16 bits mode; High-byte command
void TFT_FSMC_HIGH_DrawPixel_16bits(uint16_t x,  uint16_t y, uint16_t color) {
	TFT_FSMC_SetAttrWindow(x, y, x, y);
	*LCD_CMD_PORT	= 0x2C00;
	*LCD_DATA_PORT	= color;
}

// Draws single pixel using display working in 18 bits mode; High-byte command
void TFT_FSMC_HIGH_DrawPixel_18bits(uint16_t x,  uint16_t y, uint16_t color) {
	uint16_t rg = (color & 0xF800);
	rg |= (color & 0x7E0)  >> 3;
	uint16_t b = (color & 0x1F)   << 11;
	TFT_FSMC_SetAttrWindow(x, y, x, y);
	*LCD_CMD_PORT	= 0x2C00;
	*LCD_DATA_PORT	= rg;
	*LCD_DATA_PORT 	= b;
}

bool TFT_FSMC_ReadData(uint8_t cmd, uint8_t *data, uint16_t size) {
	if (!data || size == 0) return 0;

	*LCD_CMD_PORT = cmd << 8;
	for (uint16_t i = 0; i < size / 2; ++i) {
		uint16_t value = *LCD_DATA_PORT;
		data[i++]	= value >> 8;
		data[i]		= value & 0xFF;
	}
	return true;
}

void TFT_FSMC_ColorBlockSend_16bits(uint16_t color, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i) {
		*LCD_DATA_PORT = color;
	}
}

void TFT_FSMC_ColorBlockSend_18bits(uint16_t color, uint32_t size) {
	uint8_t r = (color & 0xF800) >> 8;
	uint8_t g = (color & 0x7E0)  >> 3;
	uint8_t b = (color & 0x1F)   << 3;
	// Send data by couple of pixels sending 3 words: R1-G1, B1-R2, G2-B2
	uint32_t copules = (size + 1) >> 1;
	for (uint32_t i = 0; i < copules; ++i) {
		*LCD_DATA_PORT = ((uint16_t)r << 8) | g;
		*LCD_DATA_PORT = ((uint16_t)b << 8) | r;
		*LCD_DATA_PORT = ((uint16_t)g << 8) | b;
	}
}
#endif

void TFT_FSMC_DATA_MODE(void) { }
void TFT_FSMC_ColorBlockInit(void)	{ }
void TFT_FSMC_ColorBlockFlush(void) { }

#endif				// FSMC_LCD_CMD
