/*
 * common.h
 *
 *  Created on: Nov 4, 2022
 *      Author: Alex
 *
 *  Base component of a library. Contains set of general functions to manage TFT display family.
 *  General approach is to set rectangular area on the screen and send array of colors to fill-up that area.
 *
 *  Based on the following libraries:
 *   martnak /STM32-ILI9341, 	https://github.com/martnak/STM32-ILI9341
 *   afiskon /stm32-ili9341,	https://github.com/afiskon/stm32-ili9341
 *   Bodmer /TFT_eSPI,			https://github.com/Bodmer/TFT_eSPI
 *   olikraus /u8glib,			https://github.com/olikraus/u8glib
 *
 *  Each particular display driver have to implement two native functions:
 *  <display>_Init(void) - to initialize the display
 *  <display>_SetRotation(tRotation rotation)
 *  because these methods depends on hardware.
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <stdbool.h>
#include "main.h"
#include "interface.h"
#include "config.h"

typedef enum {
	TFT_ROTATION_0 	 = 0,
	TFT_ROTATION_90  = 1,
	TFT_ROTATION_180 = 2,
	TFT_ROTATION_270 = 3
} tRotation;

typedef enum {
	BLACK		= 0x0000,
	NAVY    	= 0x000F,
	DARKGREEN	= 0x03E0,
	DARKCYAN    = 0x03EF,
	MAROON		= 0x7800,
	PURPLE      = 0x780F,
	OLIVE       = 0x7BE0,
	LIGHTGREY	= 0xC618,
	DARKGREY	= 0x7BEF,
	BLUE		= 0x001F,
	GREEN       = 0x07E0,
	CYAN		= 0x07FF,
	RED			= 0xF800,
	MAGENTA		= 0xF81F,
	YELLOW		= 0xFFE0,
	WHITE		= 0xFFFF,
	ORANGE		= 0xFD20,
	GREENYELLOW	= 0xAFE5,
	PINK        = 0xF81F,
	GREY		= 0x52AA
} tColor;

typedef uint16_t	(*t_NextPixel)(uint16_t row, uint16_t col);
typedef double (*LineThickness)(uint16_t pos, uint16_t length);

#ifdef __cplusplus
extern "C" {
#endif

// Functions to draw graphic primitives
uint16_t	TFT_Width(void);
uint16_t	TFT_Height(void);
tRotation 	TFT_Rotation(void);
uint16_t	TFT_WheelColor(uint8_t wheel_pos);
uint16_t	TFT_Color(uint8_t red, uint8_t green, uint8_t blue);
void 		TFT_FillScreen(uint16_t color);
void		TFT_DrawFilledRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void		TFT_DrawPixel(uint16_t x,  uint16_t y, uint16_t color);
void		TFT_DrawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void		TFT_DrawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
void		TFT_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void		TFT_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
void		TFT_DrawFilledRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
void		TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void		TFT_DrawCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color);
void		TFT_DrawFilledCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color);
void		TFT_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void		TFT_DrawfilledTriangle (uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void		TFT_DrawEllipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color);
void		TFT_DrawFilledEllipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color);
void		TFT_DrawArea(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height, t_NextPixel nextPixelCB);
void		TFT_BM_Clear(uint8_t *bitmap, uint32_t size);
void		TFT_BM_DrawPixel(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y);
uint8_t		TFT_BM_Pixel(const uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y);
void		TFT_BM_DrawHLine(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y, uint16_t length);
void		TFT_BM_DrawVLine(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y, uint16_t length);
void		TFT_BM_JoinIcon(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y, const uint8_t *icon, uint16_t ic_width, uint16_t ic_height);
void		TFT_BM_DrawVGauge(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t gauge, uint8_t edged);
void		TFT_DrawBitmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,
				const uint8_t *bitmap, uint16_t bm_width, uint16_t bg_color, uint16_t fg_color);
void		TFT_DrawScrolledBitmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,
				const uint8_t *bitmap, uint16_t bm_width, int16_t offset, uint8_t gap, uint16_t bg_color, uint16_t fg_color);
void 		TFT_DrawPixmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,
				const uint8_t *pixmap, uint16_t pm_width, uint8_t depth, uint16_t palette[]);
void		TFT_DrawThickLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t thickness, uint16_t color);
void		TFT_DrawVarThickLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, LineThickness thickness, uint16_t color);

// Convert touch coordinates according with rotation. Use with FT6x36 capacitive touch screen.
void		TFT_Touch_Adjust_Rotation_XY(uint16_t *x, uint16_t *y);

// Functions to setup display
void		TFT_Setup(uint16_t generic_width, uint16_t generic_height, uint8_t madctl[4]);
void		TFT_SetRotation(tRotation rotation);
uint16_t 	TFT_ReadPixel(uint16_t x, uint16_t y, bool is16bit_color);
void 		TFT_SetAttrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void 		TFT_StartDrawArea(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height);

#ifdef __cplusplus
}
#endif

#endif
