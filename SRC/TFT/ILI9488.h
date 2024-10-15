/*
 * ILI9341.h
 *
 *  Created on: May 20, 2020
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

#ifndef _ILI9488_H_
#define _ILI9488_H_

#include "common.h"

// By default the display is in portrait mode
#define ILI9488_SCREEN_WIDTH 	(320)
#define ILI9488_SCREEN_HEIGHT 	(480)


#ifdef __cplusplus
extern "C" {
#endif

void		ILI9488_Init(void);
void		ILI9488_IPS_Init(void);

#ifdef __cplusplus
}
#endif

#endif
