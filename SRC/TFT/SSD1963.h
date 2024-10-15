/*
 * SSD1963.h
 *
 *  Created on: May 15, 2024
 *      Author: Alex
 */

#ifndef _SSD1963_H
#define _SSD1963_H

#include "common.h"

// By default the display is in portrait mode
#define SSD1963_SCREEN_WIDTH 	(800)
#define SSD1963_SCREEN_HEIGHT 	(480)


#ifdef __cplusplus
extern "C" {
#endif

void		SSD1963_Init(void);

#ifdef __cplusplus
}
#endif

#endif
