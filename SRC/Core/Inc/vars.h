/*
 * vars.h
 *
 *  Created on: 23 сент. 2019 г.
 *      Author: Alex
 */

#ifndef VARS_H_
#define VARS_H_

#include "main.h"
#include "ff.h"

extern const uint16_t	int_temp_max;
extern const uint8_t	auto_pid_hist_length;
extern const uint8_t	hot_gun_hist_length;
extern const uint8_t	ec;

extern const uint16_t	iron_temp_minC;
extern const uint16_t 	iron_temp_maxC;
extern const uint16_t	gun_temp_minC;
extern const uint16_t 	gun_temp_maxC;

extern const uint8_t	default_ambient;
extern const TCHAR		nsl_cfg[9];
extern const char		def_language[8];
extern const char		*standalone_msg;

#define LANG_LENGTH		(20)

#endif
