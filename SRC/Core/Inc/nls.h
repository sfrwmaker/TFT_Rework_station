/*
 * nls.h
 *
 * 2022 Nov 6
 *    Removed 'change tip' menu item from parameters menu
 *
 */

#ifndef MSG_NLS_H_
#define MSG_NLS_H_

#include <string>

typedef enum e_msg { MSG_MENU_MAIN, MSG_MENU_SETUP = 12, MSG_MENU_BOOST = 12+16, MSG_MENU_CALIB = 12+16+4, MSG_MENU_GUN = 12+16+4+5,
					MSG_ON = 12+16+4+5+6, MSG_OFF, MSG_FAN, MSG_PWR, MSG_REF_POINT, MSG_REED, MSG_TILT, MSG_DEG, MSG_MINUTES, MSG_SECONDS,
					MSG_CW, MSG_CCW, MSG_SET, MSG_ERROR, MSG_TUNE_PID, MSG_SELECT_TIP,
					MSG_EEPROM_READ, MSG_EEPROM_WRITE, MSG_EEPROM_DIRECTORY, MSG_FORMAT_EEPROM, MSG_FORMAT_FAILED,
					MSG_SAVE_ERROR, MSG_HOT_AIR_GUN, MSG_SAVE_Q, MSG_YES, MSG_NO, MSG_DELETE_FILE, MSG_FLASH_DEBUG,
					MSG_SD_MOUNT, MSG_SD_NO_CFG, MSG_SD_NO_LANG, MSG_SD_MEMORY, MSG_SD_INCONSISTENT,
					MSG_LAST,
					MSG_ACTIVATE_TIPS 	= MSG_MENU_MAIN + 5,
					MSG_TUNE_IRON		= MSG_MENU_MAIN + 6,
					MSG_ABOUT 			= MSG_MENU_MAIN + 10,
					MSG_TUNE_GUN		= MSG_MENU_GUN + 2,
					MSG_AUTO			= MSG_MENU_CALIB + 1,
					MSG_MANUAL			= MSG_MENU_CALIB + 2
} t_msg_id;

typedef struct s_msg_nls {
	const char		*msg;
	std::string		msg_nls;
} t_msg;

class NLS_MSG {
	public:
		NLS_MSG()											{ }
		void			activate(bool use_nls)				{ this->use_nls = use_nls; }
		const char*		msg(t_msg_id id);
		std::string		str(t_msg_id id);
		uint8_t			menuSize(t_msg_id id);
		bool			set(std::string& parameter, std::string& value, std::string& parent);
	protected:
		bool	use_nls		= false;
		t_msg		message[MSG_LAST] = {
				// MAIN MENU
				{"Main Menu",		std::string()},			// Title is the first element of each menu
				{"parameters",		std::string()},
				{"boost setup",		std::string()},
				{"change tip",		std::string()},
				{"calibrate tip",	std::string()},
				{"activate tips",	std::string()},			// Change MSG_ACTIVATE_TIPS if new item menu inserted
				{"tune iron",		std::string()},			// Change MSG_TUNE_IRON if new item menu inserted
				{"gun setup",		std::string()},
				{"reset config",	std::string()},
				{"tune iron PID",	std::string()},
				{"about",			std::string()},			// Change MSG_ABOUT if new item menu inserted
				{"quit",			std::string()},
				// SETUP MENU
				{"Parameters",		std::string()},			// Title
				{"units",			std::string()},
				{"buzzer",			std::string()},
				{"iron encoder",	std::string()},
				{"gun encoder",		std::string()},
				{"switch type",		std::string()},
				{"temp. step",		std::string()},
				{"auto start",		std::string()},
				{"auto off",		std::string()},			// Change in-place menu item
				{"standby temp",	std::string()},			// Change in-place menu item
				{"standby time",	std::string()},			// Change in-place menu item
				{"brightness",		std::string()},			// Change in-place menu item
				{"rotation",		std::string()},			// Change in-place menu item
				{"language",		std::string()},			// Change in-place menu item
				{"save",			std::string()},
				{"cancel",			std::string()},
				// BOOST MENU
				{"Boost setup",		std::string()},			// Title
				{"temperature",		std::string()},
				{"duration",		std::string()},
				{"back to menu",	std::string()},
				// IRON TIP CALIBRATION MENU
				{"Calibrate",		std::string()},			// Title
				{"automatic",		std::string()},			// Change MSG_AUTO if new item menu inserted
				{"manual",			std::string()},			// Change MSG_MANUAL if new item menu inserted
				{"clear",			std::string()},
				{"quit",			std::string()},
				// GUN MENU
				{"Hot Air Gun",		std::string()},			// Title
				{"calibrate",		std::string()},
				{"tune gun",		std::string()},			// Change MSG_TUNE_GUN if new item menu inserted
				{"tune gun PID",	std::string()},
				{"clear",			std::string()},
				{"exit",			std::string()},
				// SINGLE MESSAGE STRINGS
				{"ON",				std::string()},
				{"OFF",				std::string()},
				{"Fan:",			std::string()},
				{"pwr:",			std::string()},
				{"Ref. #",			std::string()},
				{"REED",			std::string()},
				{"TILT",			std::string()},
				{"deg.",			std::string()},
				{"min",				std::string()},
				{"sec",				std::string()},
				{"cw",				std::string()},
				{"ccw",				std::string()},
				{"Set:",			std::string()},
				{"ERROR",			std::string()},
				{"Tune PID",		std::string()},
				{"Select tip",		std::string()},
				{"FLASH read error",		std::string()},
				{"FLASH write error",		std::string()},
				{"No directory",			std::string()},
				{"format FLASH?",			std::string()},
				{"Failed to format FLASH",	std::string()},
				{"saving configuration",	std::string()},
				{"Hot Gun",					std::string()},
				{"Save?",					std::string()},
				{"Yes",						std::string()},
				{"No",						std::string()},
				{"Delete file?",			std::string()},
				{"FLASH debug",				std::string()},
				{"Failed mount SD",			std::string()},
				{"NO config file",			std::string()},
				{"No lang. specified",		std::string()},
				{"No memory",				std::string()},
				{"Inconsistent lang",		std::string()}
		};
		const t_msg_id menu[5] = { MSG_MENU_MAIN, MSG_MENU_SETUP, MSG_MENU_BOOST, MSG_MENU_CALIB, MSG_MENU_GUN };
};

#endif
