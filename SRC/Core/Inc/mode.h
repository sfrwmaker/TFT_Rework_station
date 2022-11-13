/*
 * mode.h
 *
 */

#include <vector>
#include <string>
#include "hw.h"
#include "sdload.h"

#ifndef _MODE_H_
#define _MODE_H_

class MODE {
	public:
		MODE(HW *pCore)										{ this->pCore = pCore; 	}
		void			setup(MODE* return_mode, MODE* short_mode, MODE* long_mode);
		virtual void	init(void)							{ }
		virtual MODE*	loop(void)							{ return 0; }
		virtual			~MODE(void)							{ }
		void			useDevice(tDevice dev)				{ dev_type 	= dev; 	}
		MODE*			returnToMain(void);
		UNIT*			unit(void);
	protected:
		void 			resetTimeout(void);
		void 			setTimeout(uint16_t t);
		tDevice			dev_type		= d_t12;			// Some modes can work with iron(s) or gun (tune, calibrate, pid_tune)
		HW*				pCore			= 0;
		uint16_t		timeout_secs	= 0;				// Timeout to return to main mode, seconds
		uint32_t		time_to_return 	= 0;				// Time in ms when to return to the main mode
		uint32_t		update_screen	= 0;				// Time in ms when the screen should be updated
		MODE*			mode_return		= 0;				// Previous working mode
		MODE*			mode_spress		= 0;				// When encoder button short pressed
		MODE*			mode_lpress		= 0;				// When encoder button long  pressed

};

//-------------------- The iron main working mode, keep the temperature ----------
// mode_spress 	- tip selection mode
// mode_lpress	- main menu (when the IRON is OFF)
typedef enum { IRPH_OFF = 0, IRPH_NOHANDLE, IRPH_HEATING, IRPH_READY, IRPH_NORMAL,
				IRPH_BOOST, IRPH_LOWPWR, IRPH_GOINGOFF,
				IRPH_COOLING, IRPH_COLD } tIronPhase;

class MWORK : public MODE {
	public:
		MWORK(HW *pCore) : MODE(pCore), idle_pwr(5)		{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		void 			adjustPresetTemp(void);
		bool			hwTimeout(bool tilt_active);
		void 			swTimeout(uint16_t temp, uint16_t temp_set, uint16_t temp_setH, uint32_t td, uint32_t pd, uint16_t ap);
		void			changeIronShort(void);				// The IRON encoder button short press callback
		void			changeIronLong(void);				// The IRON encoder button long  press callback
		bool			ironEncoder(uint16_t new_value);	// The IRON encoder rotated callback
		void			ironPhaseEnd(void);					// Proceed end of phase
		bool			idleMode(void);						// Check the iron is used. Return tilt is active
		EMP_AVERAGE  	idle_pwr;							// Exponential average value for idle power
		uint32_t		phase_end		= 0;				// Time when to change phase (ms)
		uint32_t		lowpower_time	= 0;				// Time when switch to standby power mode
		uint32_t		swoff_time		= 0;				// Time when to switch the IRON off by sotfware method (see swTimeout())
		uint32_t		tilt_time		= 0;				// Time when to change tilt status (ms)
		uint16_t 		i_old_temp_set	= 0;				// The IRON preset temperature (Human Readable)
		uint16_t		g_old_temp_set	= 0;				// The Hot Air Gun preset temperature (Human Readable)
		uint16_t		alt_temp		= 0;				// BOOST or low  power temperature
		tIronPhase		i_phase			= IRPH_OFF;			// Current IRON phase
		bool			start			= true;				// Flag indicating the controller just started (used to turn the IRON on)
		bool			edit_temp		= true;				// The HOT AIR GUN Encoder mode (Edit Temp/Edit fan)
		bool			fan_blowing		= false;			// Used to draw grey or animated and colored fan
		uint32_t		return_to_temp	= 0;				// Time when to return to temperature edit mode (ms)
		uint32_t		fan_animate		= 0;				// Time when draw new fan animation
		const uint16_t	period			= 500;				// Redraw display period (ms)
		const uint16_t	edit_fan_timeout = 3000;			// The time to edit fan speed (ms)
		const uint16_t	tilt_show_time	= 1500;				// Time the tilt icon to be shown
};

//---------------------- The tip selection mode ----------------------------------
#define MSLCT_LEN		(10)
class MSLCT : public MODE {
	public:
		MSLCT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		void			changeTip(uint8_t index);
		TIP_ITEM		tip_list[MSLCT_LEN];
		uint32_t 		tip_begin_select	= 0;			// The time in ms when we started to select new tip
		uint8_t			old_index = 0;
		uint32_t		tip_disconnected	= 0;			// When the tip has been disconnected
		bool			manual_change		= false;
};

//---------------------- The Activate tip mode: select tips to use ---------------
class MTACT : public MODE {
	public:
		MTACT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint8_t			old_index = 255;
};

//---------------------- The Menu mode -------------------------------------------
class MMENU : public MODE {
	public:
		MMENU(HW* pCore, MODE* m_boost, MODE *m_change_tip, MODE *m_params, MODE* m_calib, MODE* m_act, MODE* m_tune, MODE* m_pid, MODE* m_gun_menu, MODE *m_about);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*		mode_menu_boost;
		MODE*		mode_change_tip;
		MODE*		mode_menu_setup;
		MODE*		mode_calibrate_menu;
		MODE*		mode_activate_tips;
		MODE*		mode_tune;
		MODE*		mode_tune_pid;
		MODE*		mode_gun_menu;
		MODE*		mode_about;
		uint8_t		mode_menu_item 	= 1;					// Save active menu element index to return back later
		const uint16_t	min_standby_C	= 120;				// Minimum standby temperature, Celsius
		enum { MM_PARAMS = 0, MM_BOOST_SETUP, MM_CHANGE_TIP, MM_CALIBRATE_TIP, MM_ACTIVATE_TIPS, MM_TUNE_IRON, MM_GUN_MENU,
			MM_RESET_CONFIG, MM_TUNE_IRON_PID, MM_ABOUT, MM_QUIT
		};
};

//---------------------- The Setup menu mode -------------------------------------------
class MSETUP : public MODE {
public:
	MSETUP(HW* pCore) : MODE(pCore)						{ }
	virtual void	init(void);
	virtual MODE*	loop(void);
private:
	uint8_t		off_timeout		= 0;					// Automatic switch off timeout in minutes or 0 to disable
	uint16_t	low_temp		= 0;					// The low power temperature (Celsius) 0 - disable tilt sensor
	uint8_t		low_to			= 0;					// The low power timeout, seconds
	bool		buzzer			= true;					// Whether the buzzer is enabled
	bool		celsius			= true;					// Temperature units: C/F
	bool		reed			= false;				// IRON switch type: reed/tilt
	bool		temp_step		= false;				// The preset temperature step (1/5)
	bool		i_clock_wise	= true;					// The rotary encoder mode
	bool		g_clock_wise	= true;
	bool		auto_start		= false;				// Automatic power on iron at startup
	uint8_t		dspl_bright		= 128;					// Display brightness
	uint8_t		dspl_rotation	= 0;					// Display rotation
	uint8_t		lang_index		= 0;					// Language Index (0 - english)
	uint8_t		num_lang		= 0;					// Number of the loaded languages
	uint8_t		set_param		= 0;					// The index of the modifying parameter
	uint8_t		mode_menu_item 	= 0;					// Save active menu element index to return back later
	// When new menu item added, in_place_start, in_place_end, tip_calib_menu constants should be adjusted
	const uint8_t	in_place_start	= 8;				// See the menu names. Index of the first parameter that can be changed inside menu (see nls.h)
	const uint8_t	in_place_end	= 13;				// See the menu names. Index of the last parameter that can be changed inside menu
	const uint16_t	min_standby_C	= 120;				// Minimum standby temperature, Celsius
	enum { MM_UNITS = 0, MM_BUZZER, MM_I_ENC, MM_G_ENC, MM_SWITCH_TYPE, MM_TEMP_STEP, MM_AUTO_START,
			MM_AUTO_OFF, MM_STANDBY_TEMP, MM_STANDBY_TIME, MM_BRIGHT, MM_ROTATION, MM_LANGUAGE, MM_SAVE, MM_CANCEL
	};
};

//---------------------- The Boost setup menu mode -------------------------------
class MMBST : public MODE {
	public:
		MMBST(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint8_t			delta_temp	= 0;					// The temperature increment
		uint16_t		duration	= 0;					// The boost period (secs)
		uint8_t			mode		= 0;					// The current mode: 0: select menu item, 1 - change temp, 2 - change duration
		uint8_t 		old_item 	= 0;
};

//---------------------- Calibrate tip menu --------------------------------------
class MCALMENU : public MODE {
	public:
		MCALMENU(HW* pCore, MODE* cal_auto, MODE* cal_manual);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*			mode_calibrate_tip;
		MODE*			mode_calibrate_tip_manual;
		uint8_t  		old_item = 4;
};

//---------------------- Hot Air Gun setup menu ----------------------------------
class MENU_GUN : public MODE {
	public:
		MENU_GUN(HW* pCore, MODE* calib, MODE* pot_tune, MODE* pid_tune);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*			mode_calibrate;
		MODE*			mode_tune;
		MODE*			mode_pid;
		uint8_t  		old_item	= 5;
};

//---------------------- The calibrate tip mode: automatic calibration -----------
#define MCALIB_POINTS	8
class MCALIB : public MODE {
	public:
		MCALIB(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		bool 		calibrationOLS(uint16_t* tip, uint16_t min_temp, uint16_t max_temp);
		uint8_t		closestIndex(uint16_t temp);
		void 		updateReference(uint8_t indx);
		void 		buildFinishCalibration(void);
		uint8_t		ref_temp_index	= 0;					// Which temperature reference to change: [0-MCALIB_POINTS]
		uint16_t	calib_temp[2][MCALIB_POINTS];			// The calibration data: real temp. [0] and temp. in internal units [1]
		uint16_t	tip_temp_max	= 0;					// the maximum possible tip temperature in the internal units
		bool		tuning			= false;
		int16_t		old_encoder 	= 3;
		uint32_t	ready_to		= 0;					// The time when the Iron should be ready to enter real temperature (ms)
		uint32_t	phase_change	= 0;					// The heating phase change time (ms)
		enum {MC_OFF = 0, MC_GET_READY, MC_HEATING, MC_COOLING, MC_HEATING_AGAIN, MC_READY}
					phase = MC_OFF;							// The Iron getting the Reference temperature phase
		const uint16_t start_int_temp = 600;				// Minimal temperature in internal units, about 100 degrees Celsius
		const uint32_t phase_change_time = 3000;
};

//---------------------- The calibrate tip mode: manual calibration --------------
class MCALIB_MANUAL : public MODE {
	public:
		MCALIB_MANUAL(HW *pCore) : MODE(pCore)				{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		void 		buildCalibration(int8_t ambient, uint16_t tip[], uint8_t ref_point);
		void		restorePIDconfig(CFG *pCFG, UNIT* pUnit);
		uint8_t		ref_temp_index	= 0;					// Which temperature reference to change: [0-3]
		uint16_t	calib_temp[4];							// The calibration temp. in internal units in reference points
		bool		ready			= 0;					// Whether the temperature has been established
		bool		tuning			= 0;					// Whether the reference temperature is modifying (else we select new reference point)
		uint32_t	temp_setready_ms	= 0;				// The time in ms when we should check the temperature is ready
		int16_t		old_encoder 	= 4;
		uint16_t	fan_speed		= 1500;					// The Hot Air Gun fan speed during calibration
};

//---------------------- The tune mode -------------------------------------------
class MTUNE : public MODE {
	public:
		MTUNE(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		uint16_t 	old_power 		= 0;
		bool		powered   		= true;
		bool		check_connected	= false;				// Flag indicating to check IRON or Hot Air Gun is connected
		uint32_t	check_delay		= 0;					// Time in ms when to start checking Hot Air Gun is connected
};

//---------------------- The PID coefficients tune mode --------------------------
class MTPID : public MODE {
	public:
		MTPID(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		bool		confirm(void);							// Confirmation dialog
		uint32_t	data_update	= 0;						// When read the data from the sensors (ms)
		uint8_t		data_index	= 0;						// Active coefficient
		bool        modify		= 0;						// Whether is modifying value of coefficient
		bool		on			= 0;						// Whether the IRON is turned on
		bool		reset_dspl	= false;					// The display should be reset flag
		bool		allocated	= false;					// Flag indicating the data allocated sucessfully
		uint16_t 	old_index 	= 3;
};

//---------------------- The PID coefficients automatic tune mode ----------------
class MAUTOPID : public MODE {
	public:
	typedef enum { TUNE_OFF, TUNE_HEATING, TUNE_BASE, TUNE_PLUS_POWER, TUNE_MINUS_POWER, TUNE_RELAY } TuneMode;
	typedef enum { FIX_PWR_NONE = 0, FIX_PWR_DECREASED, FIX_PWR_INCREASED, FIX_PWR_DONE } FixPWR;
		MAUTOPID(HW *pCore) : MODE(pCore)					{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
		bool			updatePID(UNIT *pUnit);
	private:
		uint16_t	td_limit	= 6;						// Temperature dispersion limits
		uint32_t	pwr_ch_to	= 5000;						// Power change timeout
		FixPWR		pwr_change	= FIX_PWR_NONE;				// How the fixed power was adjusted
		uint32_t	data_update	= 0;						// When read the data from the sensors (ms)
		uint32_t	next_mode	= 0;						// When next mode can be activated (ms)
		uint32_t	phase_to	= 0;						// Phase timeout
		uint16_t	base_pwr	= 0;						// The applied power when preset temperature reached
		uint16_t	base_temp	= 0;						// The temperature when base power applied
		uint16_t	old_temp	= 0;						// The previous temperature allowing adjust the supplied power
		uint16_t	delta_temp  = 0;						// The temperature limit (base_temp - delta_temp <= t <= base_temp + delta_temp)
		uint16_t	delta_power = 0;						// Extra power
		uint16_t	data_period = 250;						// Graph data update period (ms)
		TuneMode	mode		= TUNE_OFF;					// The preset temperature reached
		uint16_t	tune_loops	= 0;						// The number of oscillation loops elapsed in relay mode
		const uint16_t	max_delta_temp 		= 6;			// Maximum possible temperature difference between base_temp and upper temp.
		const uint32_t	msg_to	= 2000;						// Show message timeout (ms)
};

//---------------------- The Fail mode: display error message --------------------
class MFAIL : public MODE {
	public:
		MFAIL(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
		void			setMessage(const t_msg_id msg, const char *parameter = 0);
	private:
		char			parameter[20] = {0};
		t_msg_id		message	= MSG_LAST;
};

//---------------------- The About dialog mode. Show about message ---------------
class MABOUT : public MODE {
	public:
		MABOUT(HW *pCore) : MODE(pCore)						{ }
		virtual void	init(void);
		virtual MODE*	loop(void);
};


//---------------------- The Debug mode: display internal parameters ------------
class MDEBUG : public MODE {
	public:
		MDEBUG(HW *pCore, MODE* flash_debug);
		virtual void	init(void);
		virtual MODE*	loop(void);
	private:
		MODE*			flash_debug;						// Flash debug mode pointer
		uint16_t		old_ip 			= 0;				// Old IRON encoder value
		uint16_t		old_fp			= 0;				// Old GUN encoder value
		bool			gun_is_on 		= false;			// Flag indicating the gun is powered on
		const uint16_t	max_iron_power 	= 300;
		const uint16_t	min_fan_speed	= 600;
		const uint16_t	max_fan_power 	= 1999;
		const uint8_t	gun_power		= 5;
};

//---------------------- The Flash debug mode: display flash status & content ---
class FDEBUG : public MODE {
	public:
		FDEBUG(HW *pCore, MFAIL *pFail) : MODE(pCore)		{ this->pFail = pFail; }
		virtual void	init(void);
		virtual MODE*	loop(void);
		void			readDirectory();
	private:
		SDLOAD		lang_loader;							// To load language data from sd-card to flash
		FLASH_STATUS	status	=	FLASH_OK;
		uint16_t		old_ge 			= 0;				// Old Gun encoder value
		std::string		c_dir			= "/";				// Current directory path
		std::vector<std::string>	dir_list;				// Directory list
		int8_t			delete_index	= -1;				// File to be deleted index
		bool			confirm_format	= false;			// Confirmation dialog activates
		t_msg_id		msg				= MSG_LAST;			// Error message index
		MFAIL			*pFail;
		const uint32_t	update_timeout	= 60000;			// Default update display timeout, ms

};

#endif
