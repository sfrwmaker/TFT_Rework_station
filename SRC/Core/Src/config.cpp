/*
 * config.cpp
 *
 *  Created on: 15 aug. 2019.
 *      Author: Alex
 */

#include <stdlib.h>
#include "config.h"
#include "tools.h"
#include "vars.h"
#include "iron_tips.h"

/*
 * The configuration data consists of two separate items:
 * 1. The configuration record (struct s_config)
 * 2. The tip calibration record (struct s_tip_list_item)
 * The external EEPROM IC, at24c32a, divided to two separate area
 * to store configuration records of each type. See eeprom.c for details
 */

#define	 NO_TIP_CHUNK	255									// The flag showing that the tip was not found in the EEPROM

// Initialize the configuration. Find the actual record in the EEPROM.
CFG_STATUS CFG::init(void) {
//	TIP_CFG::activateGun(false);

	tip_table = (TIP_TABLE*)malloc(sizeof(TIP_TABLE) * TIPS::loaded());
	if (tip_table) {
		for (uint8_t i = 0; i < TIPS::loaded(); ++i) {
			tip_table[i].tip_index 		= NO_TIP_CHUNK;
			tip_table[i].tip_mask 		= 0;
		}
	}
	FLASH_STATUS status = W25Q::init();
	if (status == FLASH_OK) {
		uint8_t tips_loaded = 0;
		if (tip_table) {
			tips_loaded = buildTipTable(tip_table);
		}

		if (loadRecord(&a_cfg)) {
			correctConfig(&a_cfg);
		} else {
			setDefaults();
			a_cfg.tip = nearActiveTip(a_cfg.tip);
		}

		selectTip(d_gun, 0);								// Load Hot Air Gun calibtarion data (virtual tip)
		selectTip(d_t12, a_cfg.tip);						// Load tip configuration data into a_tip variable
		CFG_CORE::syncConfig();								// Save spare configuration
		if (tips_loaded > 0) {
			return CFG_OK;
		} else {
			return CFG_NO_TIP;
		}
	} else {												// EEPROM is not writable or is not ready
		setDefaults();
		TIP_CFG::defaultCalibration(d_gun);
		selectTip(d_t12, 1);
		CFG_CORE::syncConfig();
	}
	if (status == FLASH_ERROR) {
		return CFG_READ_ERROR;
	} else if (status == FLASH_NO_FILESYSTEM) {
		return CFG_NO_FILESYSTEM;
	}
	return CFG_OK;
}

bool CFG::reloadTips(void) {
	if (tip_table) {
		buildTipTable(tip_table);
		return true;
	}
	return false;
}

void CFG::correctConfig(RECORD *cfg) {
	uint16_t iron_tempC = cfg->iron_temp;
	uint16_t gun_tempC	= cfg->gun_temp;
	if (!(cfg->bit_mask & CFG_CELSIUS)) {
		iron_tempC	= fahrenheitToCelsius(iron_tempC);
		gun_tempC	= fahrenheitToCelsius(gun_tempC);
	}
	iron_tempC	= constrain(iron_tempC, iron_temp_minC, iron_temp_maxC);
	gun_tempC	= constrain(gun_tempC,  gun_temp_minC,  gun_temp_maxC);
	if (!(cfg->bit_mask & CFG_CELSIUS)) {
		iron_tempC	= celsiusToFahrenheit(iron_tempC);
		gun_tempC 	= celsiusToFahrenheit(gun_tempC);
	}
	cfg->iron_temp	= iron_tempC;
	cfg->gun_temp	= gun_tempC;
	if (cfg->off_timeout > 30)
		cfg->off_timeout = 30;
	cfg->tip = nearActiveTip(cfg->tip);
	cfg->dspl_bright = constrain(cfg->dspl_bright, 10, 255);
}

// Load calibration data of the tip from EEPROM. If the tip is not calibrated, initialize the calibration data with the default values
bool CFG::selectTip(tDevice dev_type, uint8_t index) {
	if (!tip_table) return false;
	bool result = true;
	if (dev_type == d_gun) {
		index = 0;
	} else if (dev_type != d_t12) {
		index = 1;											// 0-th tip is a Hot Air Gun
	}

	uint8_t tip_index = NO_TIP_CHUNK;
	tip_index = tip_table[index].tip_index;					// tip_table exists for sure
	if (tip_index == NO_TIP_CHUNK) {
		TIP_CFG::defaultCalibration(dev_type);
		return false;
	}
	TIP tip;
	if (loadTipData(&tip, tip_index) != TIP_OK) {
		TIP_CFG::defaultCalibration(dev_type);
		result = false;
	} else {
		if (!(tip.mask & TIP_CALIBRATED)) {					// Tip is not calibrated, load default configuration
			TIP_CFG::defaultCalibration(dev_type);
		} else if (!isValidTipConfig(&tip)) {
			TIP_CFG::defaultCalibration(dev_type);
		} else {											// Tip configuration record is completely correct
			TIP_CFG::load(tip, dev_type);
		}
	}
	return result;
}

// Change the current tip. Save configuration to the FLASH
void CFG::changeTip(uint8_t index) {
	tDevice dev_type = d_t12;
	if (index == 0) {
		dev_type = d_gun;
	} else {
		dev_type = d_t12;
	}
	if (selectTip(dev_type, index)) {
		if (dev_type == d_t12) {
			a_cfg.tip = index;
		}
		saveConfig();
	}
}

uint8_t	CFG::currentTipIndex(tDevice dev) {
	if (dev == d_t12)
		return a_cfg.tip;
	return 0;
}

/*
 * Translate the internal temperature of the IRON or Hot Air Gun to the human readable units (Celsius or Fahrenheit)
 * Parameters:
 * temp 		- Device temperature in internal units
 * ambient		- The ambient temperature
 * force_mode	-
 */
uint16_t CFG::tempToHuman(uint16_t temp, int16_t ambient, tDevice dev) {
	uint16_t tempH = TIP_CFG::tempCelsius(temp, ambient, dev);
	if (!CFG_CORE::isCelsius())
		tempH = celsiusToFahrenheit(tempH);
	return tempH;
}

// Translate the temperature from human readable units (Celsius or Fahrenheit) to the internal units
uint16_t CFG::humanToTemp(uint16_t t, int16_t ambient, tDevice dev) {
	int d = ambient - TIP_CFG::ambientTemp(dev);
	uint16_t t200	= referenceTemp(0, dev) + d;
	uint16_t t400	= referenceTemp(3, dev) + d;
	uint16_t tmin	= tempMinC(dev);
	uint16_t tmax	= tempMaxC(dev);
	if (!CFG_CORE::isCelsius()) {
		t200 = celsiusToFahrenheit(t200);
		t400 = celsiusToFahrenheit(t400);
		tmin = celsiusToFahrenheit(tmin);
		tmax = celsiusToFahrenheit(tmax);
	}
	t = constrain(t, tmin, tmax);

	uint16_t left 	= 0;
	uint16_t right 	= int_temp_max;
	uint16_t temp = map(t, t200, t400, TIP_CFG::calibration(0, dev), TIP_CFG::calibration(3, dev));

	if (temp > (left+right)/ 2) {
		temp -= (right-left) / 4;
	} else {
		temp += (right-left) / 4;
	}

	for (uint8_t i = 0; i < 20; ++i) {
		uint16_t tempH = tempToHuman(temp, ambient, dev);
		if (tempH == t) {
			return temp;
		}
		uint16_t new_temp;
		if (tempH < t) {
			left = temp;
			 new_temp = (left+right)/2;
			if (new_temp == temp)
				new_temp = temp + 1;
		} else {
			right = temp;
			new_temp = (left+right)/2;
			if (new_temp == temp)
				new_temp = temp - 1;
		}
		temp = new_temp;
	}
	return temp;
}

uint16_t CFG::lowTempInternal(int16_t ambient, tDevice dev) {
	uint16_t t200	= referenceTemp(0, dev);
	a_cfg.low_temp	= constrain(a_cfg.low_temp, ambient, t200);
	return map(a_cfg.low_temp, ambient, t200, 0, TIP_CFG::calibration(0, dev));
}

// Build the complete tip name (including "T12-" prefix)
std::string CFG::tipName(tDevice dev) {
	uint8_t tip_index = 0;
	if (dev == d_t12) {
		tip_index = a_cfg.tip;
	}
	return buildFullTipName(tip_index);
}

// Save current configuration to the flash
void CFG::saveConfig(void) {
	if (CFG_CORE::areConfigsIdentical())
		return;
	saveRecord(&a_cfg);										// calculates CRC and changes ID
	CFG_CORE::syncConfig();
}

void CFG::savePID(PIDparam &pp, tDevice dev) {
	if (dev == d_t12) {
		a_cfg.iron_Kp	= pp.Kp;
		a_cfg.iron_Ki	= pp.Ki;
		a_cfg.iron_Kd	= pp.Kd;
	} else {
		a_cfg.gun_Kp	= pp.Kp;
		a_cfg.gun_Ki	= pp.Ki;
		a_cfg.gun_Kd	= pp.Kd;
	}
	saveRecord(&a_cfg);
	CFG_CORE::syncConfig();
}

// Save new IRON tip calibration data to the FLASH only. Do not change active configuration
void CFG::saveTipCalibtarion(uint8_t index, uint16_t temp[4], uint8_t mask, int8_t ambient) {
	TIP tip;
	tip.t200		= temp[0];
	tip.t260		= temp[1];
	tip.t330		= temp[2];
	tip.t400		= temp[3];
	tip.mask		= mask;
	tip.ambient		= ambient;
	tip_table[index].tip_mask	= mask;
	const char* name	= TIPS::name(index);
	if (name && isValidTipConfig(&tip)) {
		strncpy(tip.name, name, tip_name_sz);
		int16_t tip_index = saveTipData(&tip);
		if (tip_index >= 0) {
			BUZZER::shortBeep();
			tip_table[index].tip_index	= tip_index;
			tip_table[index].tip_mask	= mask;
			return;
		}
	}
	BUZZER::failedBeep();
}

// Toggle (activate/deactivate) tip activation flag. Do not change active tip configuration
bool CFG::toggleTipActivation(uint8_t index) {
	if (!tip_table)	return false;
	bool ret = false;
	TIP tip;
	uint16_t tip_index = tip_table[index].tip_index;
	if (tip_index == NO_TIP_CHUNK) {						// This tip data is not in the FLASH, it was not active!
		const char *name = TIPS::name(index);
		if (name) {
			strncpy(tip.name, name, tip_name_sz);			// Initialize tip name
			tip.mask = TIP_ACTIVE;
			ret  = true;
		}
	} else {												// Tip configuration data exists in the EEPROM
		if (loadTipData(&tip, tip_index, true) == TIP_OK) {
			tip.mask ^= TIP_ACTIVE;
			ret = true;
		}
	}
	if (!ret) return false;

	tip_index = saveTipData(&tip, true);
	if (tip_index >= 0) {
		tip_table[index].tip_index	= tip_index;
		tip_table[index].tip_mask	= tip.mask;
		return true;
	}
	return false;
}

 // Build the tip list starting from the previous tip
int	CFG::tipList(uint8_t current, TIP_ITEM list[], uint8_t list_len, bool active_only) {
	if (!tip_table) {										// If tip_table is not initialized, return empty list
		for (uint8_t tip_index = 0; tip_index < list_len; ++tip_index) {
			list[tip_index].name[0] = '\0';					// Clear whole list
		}
		return 0;
	}

	// Seek several (previous) tips backward
	int16_t tip_index = current-1;
	uint8_t previous = 3;
	for (; tip_index > 0; --tip_index) {
		if (!active_only || (tip_table[tip_index].tip_mask & TIP_ACTIVE)) {
			if (--previous == 0)
				break;
		}
	}
	uint8_t loaded = 0;
	for (; tip_index < TIPS::loaded(); ++tip_index) {
		if (tip_index == 0) continue;						// Skip Hot Air Gun 'tip'
		if (active_only && !(tip_table[tip_index].tip_mask & TIP_ACTIVE)) // This tip is not active, but active tip list required
			continue;										// Skip this tip
		list[loaded].tip_index	= tip_index;
		list[loaded].mask		= tip_table[tip_index].tip_mask;
		std::string tip_name	= buildFullTipName(tip_index);
		strncpy(list[loaded].name, tip_name.c_str(), tip_name_sz+5);
		++loaded;
		if (loaded >= list_len)	break;
	}
	for (uint8_t tip_index = loaded; tip_index < list_len; ++tip_index) {
		list[tip_index].name[0] = '\0';						// Clear rest of the list
	}
	return loaded;
}

// Check the current tip is active. Return nearest active tip or 1 if no one tip has been activated
uint8_t	CFG::nearActiveTip(uint8_t current_tip) {
	if (!tip_table) {										// If tip_table is not initialized
		return 1;
	}
	uint16_t max_tip = TIPS::loaded();
	current_tip = constrain(current_tip, 1, max_tip);
	if (tip_table[current_tip].tip_mask & TIP_ACTIVE)
		return current_tip;
	uint16_t top_tip = current_tip;
	for (; top_tip > 0; --top_tip) {
		if (tip_table[top_tip].tip_mask & TIP_ACTIVE)
			break;
	}
	uint16_t bot_tip = current_tip;
	for (; bot_tip < max_tip; ++bot_tip) {
		if (tip_table[bot_tip].tip_mask & TIP_ACTIVE)
			break;
	}
	if (top_tip == 0) {										// No active tip lower than current one
		if (bot_tip != max_tip) {							// Found active tip greater than current one
			return bot_tip;
		} else {
			return 1;
		}
	} else {												// Found active tip lower than current one
		if (bot_tip == max_tip) {							// No active tip greater than current one
			return top_tip;
		} else {											// Found active tips on both sides
			if (current_tip-top_tip < bot_tip-current_tip) {
				return top_tip;
			} else {
				return bot_tip;
			}
		}
	}
}

// Initialize the configuration area. Save default configuration to the FLASH
void CFG::initConfig(void) {
	if (clearConfig()) {									// Format FLASH
		setDefaults();										// Create default configuration
		saveRecord(&a_cfg);									// Save default config
		clearAllTipsCalibration();							// Clear tip calibration data
	}
}

bool CFG::clearAllTipsCalibration(void) {
	if (tip_table) {
		for (uint8_t i = 0; i < TIPS::loaded(); ++i) {
			tip_table[i].tip_index 		= NO_TIP_CHUNK;
			tip_table[i].tip_mask 		= 0;
		}
	}
	return clearTips();
}

/*
 * Builds the tip configuration table: reads whole tip configuration area and search for configured or active tip
 * If the tip found, updates the tip_table array with the tip chunk number
 */
uint8_t	CFG::buildTipTable(TIP_TABLE tt[]) {
	TIP			tmp_tip;
	uint16_t	loaded 		= 0;
	for (int16_t i = 0; i < TIPS::loaded(); ++i) {		// Limit this loop by whole TIP list for reliability
		TIP_IO_STATUS ts = loadTipData(&tmp_tip, i, true);
		if (ts == TIP_OK) {
			int16_t glb_index = TIPS::index(tmp_tip.name);
			// Loaded existing tip data once
			if (glb_index >= 0 && tmp_tip.mask > 0 && tt[glb_index].tip_index == NO_TIP_CHUNK) {
				tt[glb_index].tip_index 	= i;
				tt[glb_index].tip_mask		= tmp_tip.mask;
				++loaded;
			}
		} else if (ts == TIP_IO) {
			break;
		}
	}
	W25Q::umount();
	return loaded;
}

// Build full name of the current tip. Add prefix "T12-" for the "usual" tip or use complete name for "N*" tips
std::string CFG::buildFullTipName(const uint8_t index) {
	const char *name = TIPS::name(index);
	std::string tip_name(name, tip_name_sz);
	if (name) {
		if (index == 0 || name[0] == 'N') {					// Do not modify Hot Air Gun 'tip' name nor N* names
			return tip_name;
		} else {											// All other names should be prefixed with 'T12-'
			tip_name.insert(0, "T12-");
		}
	} else {
		tip_name = std::string("T12-def");
	}
	return tip_name;
}

// Compare two configurations
bool CFG_CORE::areConfigsIdentical(void) {
	if (a_cfg.iron_temp 		!= s_cfg.iron_temp) 		return false;
	if (a_cfg.gun_temp 			!= s_cfg.gun_temp) 			return false;
	if (a_cfg.gun_fan_speed 	!= s_cfg.gun_fan_speed)		return false;
	if (a_cfg.low_temp			!= s_cfg.low_temp)			return false;
	if (a_cfg.low_to			!= s_cfg.low_to)			return false;
	if (a_cfg.tip 				!= s_cfg.tip)				return false;
	if (a_cfg.off_timeout 		!= s_cfg.off_timeout)		return false;
	if (a_cfg.bit_mask			!= s_cfg.bit_mask)			return false;
	if (a_cfg.boost				!= s_cfg.boost)				return false;
	if (a_cfg.dspl_bright		!= s_cfg.dspl_bright)		return false;
	if (strncmp(a_cfg.language, s_cfg.language, LANG_LENGTH)  != 0)	return false;
	return true;
};

//---------------------- CORE_CFG class functions --------------------------------
void CFG_CORE::setDefaults(void) {
	a_cfg.iron_temp			= 235;
	a_cfg.gun_temp			= 300;
	a_cfg.gun_fan_speed		= 1200;
	a_cfg.tip				= 1;							// The first IRON tip. Tip #0 is the Hot Air Gun
	a_cfg.off_timeout		= 0;
	a_cfg.low_temp			= 0;
	a_cfg.low_to			= 5;
	a_cfg.bit_mask			= CFG_CELSIUS | CFG_BUZZER | CFG_I_CLOCKWISE | CFG_G_CLOCKWISE;
	a_cfg.boost				= 0;
	a_cfg.iron_Kp			= 6217;	// 2300;
	a_cfg.iron_Ki			=   37;	// 50;
	a_cfg.iron_Kd			= 2960;	// 735;
	a_cfg.gun_Kp			=  200;
	a_cfg.gun_Ki			=   64;
	a_cfg.gun_Kd			=  195;
	a_cfg.dspl_bright		=  128;
	a_cfg.dspl_rotation		=  1;							// TFT_ROTATION_90;
	strncpy(a_cfg.language, def_language, LANG_LENGTH);
}

const char *CFG_CORE::getLanguage(void) {
	if (a_cfg.language[0] == '\0')
		strncpy(a_cfg.language, def_language, LANG_LENGTH);
	return a_cfg.language;
}

// Apply main configuration parameters: automatic off timeout, buzzer and temperature units
void CFG_CORE::setup(uint8_t off_timeout, bool buzzer, bool celsius, bool reed, bool big_temp_step, bool i_enc, bool g_enc,
						bool auto_start, uint16_t low_temp, uint8_t low_to, uint8_t bright) {
	bool cfg_celsius		= a_cfg.bit_mask & CFG_CELSIUS;
	a_cfg.off_timeout		= off_timeout;
	a_cfg.low_temp			= low_temp;
	a_cfg.low_to			= low_to;
	if (cfg_celsius	!= celsius) {							// When we change units, the temperature should be converted
		if (celsius) {										// Translate preset temp. from Fahrenheit to Celsius
			a_cfg.iron_temp	= fahrenheitToCelsius(a_cfg.iron_temp);
			a_cfg.gun_temp	= fahrenheitToCelsius(a_cfg.gun_temp);
		} else {											// Translate preset temp. from Celsius to Fahrenheit
			a_cfg.iron_temp	= celsiusToFahrenheit(a_cfg.iron_temp);
			a_cfg.gun_temp	= celsiusToFahrenheit(a_cfg.gun_temp);
		}
	}
	a_cfg.bit_mask	= 0;
	if (celsius)		a_cfg.bit_mask |= CFG_CELSIUS;
	if (buzzer)			a_cfg.bit_mask |= CFG_BUZZER;
	if (reed)			a_cfg.bit_mask |= CFG_SWITCH;
	if (big_temp_step)	a_cfg.bit_mask |= CFG_BIG_STEP;
	if (i_enc)			a_cfg.bit_mask |= CFG_I_CLOCKWISE;
	if (g_enc)			a_cfg.bit_mask |= CFG_G_CLOCKWISE;
	if (auto_start) 	a_cfg.bit_mask |= CFG_AU_START;
	a_cfg.dspl_bright	= constrain(bright, 10, 255);
}

void CFG_CORE::savePresetTempHuman(uint16_t temp_set) {
	a_cfg.iron_temp = temp_set;
}

void CFG_CORE::saveGunPreset(uint16_t temp_set, uint16_t fan) {
	a_cfg.gun_temp 		= temp_set;
	a_cfg.gun_fan_speed	= fan;
}

void CFG_CORE::syncConfig(void)	{
	memcpy(&s_cfg, &a_cfg, sizeof(RECORD));
}

void CFG_CORE::restoreConfig(void) {
	memcpy(&a_cfg, &s_cfg, sizeof(RECORD));					// restore configuration from spare copy
}

/*
 * Boost is a bit map. The upper 4 bits are boost increment temperature (n*5 Celsius), i.e.
 * 0000 - disabled
 * 0001 - +5  degrees
 * 1111 - +75 degrees
 * The lower 4 bits is the boost time ((n+1)* 5 seconds), i.e.
 * 0000 -  5 seconds
 * 0001 - 10 seconds
 * 1111 - 80 seconds
 */
uint8_t	CFG_CORE::boostTemp(void){
	uint8_t t = a_cfg.boost >> 4;
	return t * 5;
}

uint16_t CFG_CORE::boostDuration(void) {
	uint16_t d = a_cfg.boost & 0xF;
	return (d+1)*20;
}

// Save boost parameters to the current configuration
void CFG_CORE::saveBoost(uint8_t temp, uint16_t duration) {
	if (temp > 75)		temp = 75;
	if (duration > 320)	duration = 320;
	if (duration < 5)   duration = 5;
	temp += 4;
	temp /= 5;
	a_cfg.boost = temp << 4;
	a_cfg.boost &= 0xF0;
	a_cfg.boost |= ((duration-1)/20) & 0xF;
}


// PID parameters: Kp, Ki, Kd
PIDparam CFG_CORE::pidParams(tDevice dev) {
	if (dev == d_t12) {
		return PIDparam(a_cfg.iron_Kp, a_cfg.iron_Ki, a_cfg.iron_Ki);
	} else {
		return PIDparam(a_cfg.gun_Kp, a_cfg.gun_Ki, a_cfg.gun_Kd);
	}
}

// PID parameters: Kp, Ki, Kd for smooth work, i.e. tip calibration
PIDparam CFG_CORE::pidParamsSmooth(tDevice dev) {
	if (dev == d_t12) {
		return PIDparam(575, 10, 200);
	} else {
		return PIDparam(150, 64, 50);
	}
}


//---------------------- CORE_CFG class functions --------------------------------
void TIP_CFG::load(const TIP& ltip, tDevice dev) {
	uint8_t i = uint8_t(dev);
	tip[i].calibration[0]	= ltip.t200;
	tip[i].calibration[1]	= ltip.t260;
	tip[i].calibration[2]	= ltip.t330;
	tip[i].calibration[3]	= ltip.t400;
	tip[i].mask				= ltip.mask;
	tip[i].ambient			= ltip.ambient;
}

void TIP_CFG::dump(TIP* ltip, tDevice dev) {
	uint8_t i = uint8_t(dev);
	ltip->t200		= tip[i].calibration[0];
	ltip->t260		= tip[i].calibration[1];
	ltip->t330		= tip[i].calibration[2];
	ltip->t400		= tip[i].calibration[3];
	ltip->mask		= tip[i].mask;
	ltip->ambient	= tip[i].ambient;
}

int8_t TIP_CFG::ambientTemp(tDevice dev) {
	uint8_t i = uint8_t(dev);
	return tip[i].ambient;
}

uint16_t TIP_CFG::calibration(uint8_t index, tDevice dev) {
	if (index >= 4)
		return 0;
	uint8_t i = uint8_t(dev);
	return tip[i].calibration[index];
}

uint16_t TIP_CFG::referenceTemp(uint8_t index, tDevice dev) {
	if (index >= 4)
		return 0;
	if (dev == d_gun)
		return temp_ref_gun[index];
	else
		return temp_ref_iron[index];
}

// Translate the internal temperature of the IRON or Hot Air Gun to Celsius
uint16_t TIP_CFG::tempCelsius(uint16_t temp, int16_t ambient, tDevice dev) {
	uint8_t i 		= uint8_t(dev);								// Select appropriate calibration tip or gun
	int16_t tempH 	= 0;

	// The temperature difference between current ambient temperature and ambient temperature during tip calibration
	int d = ambient - tip[i].ambient;
	if (temp < tip[i].calibration[0]) {							// less than first calibration point
	    tempH = map(temp, 0, tip[i].calibration[0], ambient, referenceTemp(0, dev)+d);
	} else {
		if (temp <= tip[i].calibration[3]) {					// Inside calibration interval
			for (uint8_t j = 1; j < 4; ++j) {
				if (temp < tip[i].calibration[j]) {
					tempH = map(temp, tip[i].calibration[j-1], tip[i].calibration[j],
							referenceTemp(j-1, dev)+d, referenceTemp(j, dev)+d);
					break;
				}
			}
		} else {												// Greater than maximum
			if (tip[i].calibration[1] < tip[i].calibration[3]) { // If tip calibrated correctly
				tempH = map(temp, tip[i].calibration[1], tip[i].calibration[3],
					referenceTemp(1, dev)+d, referenceTemp(3, dev)+d);
			} else {											// Perhaps, the tip calibration process
				tempH = map(temp, tip[i].calibration[1], int_temp_max,
							referenceTemp(1, dev)+d, referenceTemp(3, dev)+d);
			}
		}
	}
	tempH = constrain(tempH, ambient, 999);
	return tempH;
}

// Return the reference temperature points of the IRON tip calibration
void TIP_CFG::getTipCalibtarion(uint16_t temp[4], tDevice dev) {
	uint8_t i = uint8_t(dev);
	for (uint8_t j = 0; j < 4; ++j)
		temp[j]	= tip[i].calibration[j];
}

// Apply new IRON tip calibration data to the current configuration
void TIP_CFG::applyTipCalibtarion(uint16_t temp[4], int8_t ambient, tDevice dev) {
	uint8_t i = uint8_t(dev);
	for (uint8_t j = 0; j < 4; ++j)
		tip[i].calibration[j]	= temp[j];
	tip[i].ambient	= ambient;
	tip[i].mask		= TIP_CALIBRATED | TIP_ACTIVE;
	if (tip[i].calibration[3] > int_temp_max) tip[i].calibration[3] = int_temp_max;
}

// Initialize the tip calibration parameters with the default values
void TIP_CFG::resetTipCalibration(tDevice dev) {
	defaultCalibration(dev);
}

// Apply default calibration parameters of the tip; Prevent overheating of the tip
void TIP_CFG::defaultCalibration(tDevice dev) {
	uint8_t i = uint8_t(dev);
	tip[i].calibration[0]	=  680;
	tip[i].calibration[1]	=  964;
	tip[i].calibration[2]	= 1290;
	tip[i].calibration[3]	= 1600;
	tip[i].ambient			= default_ambient;					// vars.cpp
	tip[i].mask				= TIP_ACTIVE;
}

bool TIP_CFG::isValidTipConfig(TIP *tip) {
	return (tip->t200 < tip->t260 && tip->t260 < tip->t330 && tip->t330 < tip->t400);
}
