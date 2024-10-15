/*
 * NT35510.h
 *
 *  Created on: 5 JAN 2023
 *      Author: Alex
 */

#ifndef _NT35510_H
#define _NT35510_H

#include "common.h"

// By default the display is in portrait mode
#define NT35510_SCREEN_WIDTH 	(480)
#define NT35510_SCREEN_HEIGHT 	(800)

#ifdef __cplusplus
extern "C" {
#endif

void		NT35510_Init(void);

#ifdef __cplusplus
}
#endif

#endif 		/* _NT35510_H */
