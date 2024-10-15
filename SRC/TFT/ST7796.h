/*
 * ST7796.h
 *
 *  Created on: Nov 18, 2020
 *      Author: Alex
 *
 *  Based on the following libraries:
 *   martnak /STM32-ILI9341, 	https://github.com/martnak/STM32-ILI9341
 *   afiskon /stm32-ili9341,	https://github.com/afiskon/stm32-ili9341
 *   Bodmer /TFT_eSPI,			https://github.com/Bodmer/TFT_eSPI
 *   olikraus /u8glib,			https://github.com/olikraus/u8glib
 *
 *  CS & DC pins should share the same CPIO port
 */

#ifndef _ST7796_H_
#define _ST7796_H_

#include "common.h"

// By default the display is in portrait mode
#define ST7796_SCREEN_WIDTH 	(320)
#define ST7796_SCREEN_HEIGHT 	(480)


#ifdef __cplusplus
extern "C" {
#endif

void		ST7796_Init(void);

#ifdef __cplusplus
}
#endif

#endif
