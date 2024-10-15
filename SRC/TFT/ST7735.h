/*
 * ST7735.h
 *
 *  Created on: Nov 19 2020
 *      Author: Alex
 */

#ifndef _ST7735_H
#define _ST7735_H

#include "common.h"

// By default the display is in portrait mode
#define ST7735_SCREEN_WIDTH 	(128)
#define ST7735_SCREEN_HEIGHT 	(160)


#ifdef __cplusplus
extern "C" {
#endif

void		ST7735_Init(void);

#ifdef __cplusplus
}
#endif

#endif

