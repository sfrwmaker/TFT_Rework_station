/*
 * core.h
 *
 *  Created on: 16 sep 2019
 *      Author: Alex
 *
 *  2022 DEC 26
 *     Added gtimPeriod() function that return the period of GUN timer in ms
 */

#ifndef CORE_H_
#define CORE_H_

#include "main.h"

#ifndef __cplusplus
#include <stdbool.h>
#endif

// Forward function declaration
bool		isACsine(void);
uint16_t	gtimPeriod(void);

#ifdef __cplusplus
extern "C" {
#endif

void setup(void);
void loop(void);

#ifdef __cplusplus
}
#endif

#endif
