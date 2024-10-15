/*
 * ILI9341.h
 *
 *  Created on: May 20, 2020
 *      Author: Alex
 *
 *  2024 AUG 02
 *  	Added ILI9341v support
 */

#ifndef _ILI9341_H_
#define _ILI9341_H_

#include "common.h"

// By default the display is in portrait mode
#define ILI9341_SCREEN_WIDTH 	(240)
#define ILI9341_SCREEN_HEIGHT 	(320)

#ifdef __cplusplus
extern "C" {
#endif

void		ILI9341_Init(void);
void		ILI9341v_Init(void);

#ifdef __cplusplus
}
#endif

#endif
