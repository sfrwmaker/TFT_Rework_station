/*
 * config.h
 *
 *  Created on: 12 july 2021.
 *      Author: Alex
 *  2024 OCT 06, v.1.15
 *  	Added CFG::CORE::isFastGunCooling()
 *  	Added new parameter to CFG_CORE::setup()
 *  	Added new internal type (struct s_setup) into CFG_CORE class allowing to pass parameters into CFG_CORE::setup()
 *  	Added new method, CFG_CORE::getMainParams()
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <string>
#include <string.h>
#include "main.h"
#include "pid.h"
#include "flash.h"
#include "cfgtypes.h"
#include "iron_tips.h"
#include "buzzer.h"

typedef enum {CFG_OK = 0, CFG_NO_TIP, CFG_READ_ERROR, CFG_NO_FILESYSTEM} CFG_STATUS;

/*
 * The actual configuration record is loaded from the EEPROM chunk into a_cfg variable
 * The spare copy of the  configuration record is preserved into s_cfg variable
 * When update request arrives, configuration record writes into EEPROM if spare copy is different from actual copy
 */
class CFG_CORE: public TIPS {
	public:
	typedef struct s_setup {
		uint8_t off_timeout;
		bool buzzer;
		bool celsius;
		bool reed;
		bool big_temp_step;
		bool i_enc;
		bool g_enc;
		bool fast_cooling;
		bool auto_start;
		uint16_t low_temp;
		uint8_t low_to;
		uint8_t bright;
		bool ips_display;
	} t_setup_arg;

		CFG_CORE(void)									{ }
		bool		isCelsius(void) 					{ return a_cfg.bit_mask & CFG_CELSIUS;		}
		bool		isBuzzerEnabled(void)				{ return a_cfg.bit_mask & CFG_BUZZER; 		}
		bool		isReedType(void)					{ return a_cfg.bit_mask & CFG_SWITCH;		}
		bool		isBigTempStep(void)					{ return a_cfg.bit_mask & CFG_BIG_STEP;		}
		bool		isAutoStart(void)					{ return a_cfg.bit_mask & CFG_AU_START;		}
		bool		isIronEncClockWise(void)			{ return a_cfg.bit_mask & CFG_I_CLOCKWISE;	}
		bool		isGunEncClockWise(void)				{ return a_cfg.bit_mask & CFG_G_CLOCKWISE;	}
		bool		isFastGunCooling(void)				{ return a_cfg.bit_mask & CFG_FAST_COOLING;	}
		bool		isIPS(void)							{ return a_cfg.bit_mask & CFG_DSPL_TYPE;	}
		uint16_t	tempPresetHuman(void) 				{ return a_cfg.iron_temp;					}	// Human readable units
		uint16_t	gunTempPreset(void)					{ return a_cfg.gun_temp;					}	// Human readable units
		uint16_t	gunFanPreset(void)					{ return a_cfg.gun_fan_speed;				}
		uint8_t		getOffTimeout(void) 				{ return a_cfg.off_timeout; 				}
		uint16_t	getLowTemp(void)					{ return a_cfg.low_temp; 					}
		uint8_t		getLowTO(void)						{ return a_cfg.low_to; 						}	// 5-seconds intervals
		uint8_t		getDsplBrightness(void)				{ return a_cfg.dspl_bright;					}
		uint8_t		getDsplRotation(void)				{ return a_cfg.dspl_rotation;				}
		void		setDsplRotation(uint8_t rotation)	{ a_cfg.dspl_rotation = rotation;			}
		void		setLanguage(const char *lang)		{ strncpy(a_cfg.language, lang, LANG_LENGTH);}
		const char	*getLanguage(void);					// Returns current language name
		void		getMainParams(t_setup_arg &prm);
		void		setup(t_setup_arg &arg);
		void 		savePresetTempHuman(uint16_t temp_set);
		void		saveGunPreset(uint16_t temp, uint16_t fan = 0);
		uint8_t		boostTemp(void);
		uint16_t	boostDuration(void);
		void		saveBoost(uint8_t temp, uint16_t duration);
		void		restoreConfig(void);
		PIDparam	pidParams(tDevice dev);
		PIDparam	pidParamsSmooth(tDevice dev);
	protected:
		void		setDefaults(void);
		void		syncConfig(void);
		bool		areConfigsIdentical(void);
		RECORD		a_cfg;								// active configuration
	private:
		RECORD		s_cfg;								// spare configuration, used when save the configuration to the EEPROM
};

typedef struct s_TIP_RECORD	TIP_RECORD;
struct s_TIP_RECORD {
	uint16_t	calibration[4];
	uint8_t		mask;
	int8_t		ambient;
};

class TIP_CFG {
	public:
		TIP_CFG(void)									{ }
		bool 		isTipCalibrated(void) 				{ return tip[0].mask & TIP_CALIBRATED; 		}
		uint16_t	tempMinC(tDevice dev)				{ return (dev == d_gun)?gun_temp_minC:iron_temp_minC;	}
		uint16_t	tempMaxC(tDevice dev)				{ return (dev == d_gun)?gun_temp_maxC:iron_temp_maxC;	}
		void		load(const TIP& tip, tDevice dev = d_t12);
		void		dump(TIP* tip, tDevice dev = d_t12);
		int8_t		ambientTemp(tDevice dev);
		uint16_t	calibration(uint8_t index, tDevice dev);
		uint16_t	referenceTemp(uint8_t index, tDevice dev);
		uint16_t	tempCelsius(uint16_t temp, int16_t ambient, tDevice dev);
		void		getTipCalibtarion(uint16_t temp[4], tDevice dev);
		void		applyTipCalibtarion(uint16_t temp[4], int8_t ambient, tDevice dev);
		void		resetTipCalibration(tDevice dev);
	protected:
		void 		defaultCalibration(tDevice dev = d_t12);
		bool		isValidTipConfig(TIP *tip);
	private:
		TIP_RECORD	tip[2];								// Active IRON tip (0) and Hot Air Gun virtual tip (1)
		const uint16_t	temp_ref_iron[4]	= { 200, 260, 330, 400};
		const uint16_t	temp_ref_gun[4]		= { 200, 300, 400, 500};
};

class CFG : public W25Q, public CFG_CORE, public TIP_CFG, public BUZZER {
	public:
		CFG(void)				{ }
		CFG_STATUS	init(void);
		bool		reloadTips(void);
		uint16_t	tempToHuman(uint16_t temp, int16_t ambient, tDevice dev);
		uint16_t	humanToTemp(uint16_t temp, int16_t ambient, tDevice dev);
		uint16_t	lowTempInternal(int16_t ambient, tDevice dev);
		std::string tipName(tDevice dev);
		void     	changeTip(uint8_t index);
		uint8_t		currentTipIndex(tDevice dev);
		void		saveTipCalibtarion(uint8_t index, uint16_t temp[4], uint8_t mask, int8_t ambient);
		bool		toggleTipActivation(uint8_t index);
		int			tipList(uint8_t second, TIP_ITEM list[], uint8_t list_len, bool active_only);
		uint8_t		nearActiveTip(uint8_t current_tip);
		void		saveConfig(void);
		void		savePID(PIDparam &pp, tDevice dev = d_t12);
		void 		initConfig(void);
		bool		clearAllTipsCalibration(void);		// Remove tip calibration data
	private:
		void		correctConfig(RECORD *cfg);
		bool 		selectTip(tDevice dev_type, uint8_t index);
		uint8_t		buildTipTable(TIP_TABLE tt[]);
		std::string buildFullTipName(const uint8_t index);
		TIP_TABLE	*tip_table = 0;						// Tip table - chunk number of the tip or 0xFF if does not exist in the EEPROM
};

#endif
