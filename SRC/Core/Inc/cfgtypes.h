/*
 * cfgtypes.h
 *
 *  Created on: 12 july 2021
 *      Author: Alex
 *
 * 2022 Nov 6
 *    Removed 'CFG_TIP_MANUAL' configuration parameter
 *  2024 OCT 06 v.1.15
 *  	Added CFG_FAST_COOLING and CFG_DSPL_TYPE entries to the CFG_BIT_MASK
 *  	Changed the type of bit_mask field (uint8_t -> uint16_t) in the RECORD struct.
 */

#ifndef CFGTYPES_H_
#define CFGTYPES_H_
#include "iron_tips.h"
#include "vars.h"

/*
 * The configuration bit map:
 * CFG_CELSIUS		- The temperature units: Celsius (1) or Fahrenheit (0)
 * CFG_BUZZER		- Is the Buzzer Enabled (1)
 * CFG_SWITCH		- Switch type: Tilt (0) or REED (1)
 * CFG_AU_START		- Powering on the HAkko T12 iron at startup
 * CFG_U_CLOCKWISE	- Upper Encoder increments clockwise
 * CFG_L_CLOCKWISE	- Lower Encoder increments clockwise
 * CFG_FAST_COOLING	- Start cooling the Hot Air Gun at maximum fan speed
 * CFG_BIG_STEP		- The temperature step 1 degree (0) 5 degree (1)
 * CFG_DSPL_TYPE	- The ili9341 display variant: TFT (0) IPS (1)
 */
typedef enum { CFG_CELSIUS = 1, CFG_BUZZER = 2, CFG_SWITCH = 4, CFG_AU_START = 8,
				CFG_I_CLOCKWISE = 16, CFG_G_CLOCKWISE = 32, CFG_FAST_COOLING = 64, CFG_BIG_STEP = 128,
				CFG_DSPL_TYPE = 256 } CFG_BIT_MASK;

typedef enum { d_t12 = 0, d_gun = 1, d_unknown } tDevice;

typedef enum {FLASH_OK = 0, FLASH_ERROR, FLASH_NO_FILESYSTEM, FLASH_NO_DIRECTORY} FLASH_STATUS;

/* Configuration record in the EEPROM (after the tip table) has the following format:
 * Records are aligned by 2**n bytes (in this case, 32 bytes)
 *
 * Boost is a bit map. The upper 4 bits are boost increment temperature (n*5 Celsius), i.e.
 * 0000 - disabled
 * 0001 - +4  degrees
 * 1111 - +75 degrees
 * The lower 4 bits is the boost time ((n+1)* 20 seconds), i.e.
 * 0000 -  20 seconds
 * 0001 -  40 seconds
 * 1111 - 320 seconds
 */
typedef struct s_config RECORD;
struct s_config {
	uint16_t	crc;								// The checksum
	uint16_t	iron_temp;							// The IRON preset temperature in degrees (Celsius or Fahrenheit)
	uint16_t	gun_temp;							// The Hot Air Gun preset temperature in degrees (Celsius or Fahrenheit)
	uint16_t	gun_fan_speed;						// The Hot Air Gun fan speed
	uint16_t	iron_Kp, iron_Ki, iron_Kd;			// The IRON PID coefficients
	uint16_t	gun_Kp,  gun_Ki,  gun_Kd;			// The Hot Air Gun PID coefficients
	uint16_t	low_temp;							// The low power temperature (C) or 0 if the tilt sensor is disabled
	uint8_t		low_to;								// The low power timeout (5 seconds intervals)
	uint8_t		boost;								// Two 4-bits parameters: The boost increment temperature and boost time. See description above
	uint8_t		tip;								// Current tip index
	uint8_t		off_timeout;						// The Automatic switch-off timeout in minutes [0 - 30]
	uint16_t	bit_mask;							// See CFG_BIT_MASK
	uint8_t		dspl_bright;						// The display brightness
	uint8_t		dspl_rotation;						// The display rotation (TFT_ROTATION_0, TFT_ROTATION_90, TFT_ROTATION_180, TFT_ROTATION_270)
	char		language[LANG_LENGTH];				// The language. lLANG_LENGTH defined in vars.h
};

/*
 * Configuration data of each initialized tip are saved in the tipcal.dat file (16 bytes per tip record).
 * The tip configuration record has the following format:
 * 4 reference temperature points
 * tip status bitmap
 * tip suffix name
 */
typedef struct s_tip TIP;
struct s_tip {
	uint16_t	t200, t260, t330, t400;				// The internal temperature in reference points
	uint8_t		mask;								// The bit mask: TIP_ACTIVE + TIP_CALIBRATED
	char		name[tip_name_sz];					// T12 tip name suffix, JL02 for T12-JL02
	int8_t		ambient;							// The ambient temperature in Celsius when the tip being calibrated
	uint8_t		crc;								// CRC checksum
};

// This tip structure is used to show available tips when tip is activating
typedef struct s_tip_list_item	TIP_ITEM;
struct s_tip_list_item {
	uint8_t		tip_index;							// Index of the tip in the global list in EEPROM
	uint8_t		mask;								// The bit mask: 0 - active, 1 - calibrated
	char		name[tip_name_sz+5];				// Complete tip name, i.e. T12-***
};

/*
 * This structure presents a tip record for all possible tips, declared in iron_tips.c
 * During controller initialization phase, the buildTipTable() function creates
 * the tip list in memory of all possible tips. If the tip is calibrated, i.e. has a record
 * in the tipcal.dat file on W25Qxx flash, the tip record saves tip record index in the file
 */
typedef struct s_tip_table		TIP_TABLE;
struct s_tip_table {
	uint8_t		tip_index;							// The tip index in the calib.tip file
	uint8_t		tip_mask;							// The bit mask: 0 - active, 1 - calibrated
};

typedef enum tip_status { TIP_ACTIVE = 1, TIP_CALIBRATED = 2 } TIP_STATUS;

#endif
