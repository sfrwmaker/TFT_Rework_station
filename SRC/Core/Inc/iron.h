/*
 * iron.h
 *
 * 2022 Nov 6
 *     Added POWER_HEATING mode to prevent soldering iron overheating in the first phase
 * 2022 Nov 9
 * 	   Added b_reset bool variable flag to initialize the IRON temperature EMP_AVERAGE values
 *
 */

#ifndef IRON_H_
#define IRON_H_

#include "stat.h"
#include "unit.h"

class IRON : public UNIT {
	public:
	typedef enum { POWER_OFF, POWER_HEATING, POWER_ON, POWER_FIXED, POWER_COOLING, POWER_PID_TUNE, POWER_BOOST } PowerMode;
		IRON(void) 											{ }
		void		init(void);
		virtual void		switchPower(bool On);
		virtual void		autoTunePID(uint16_t base_pwr, uint16_t delta_power, uint16_t base_temp, uint16_t temp);
		bool				isOn(void)						{ return (mode == POWER_ON); }
		uint16_t 			temp(void)						{ return temp_curr; }
		virtual uint16_t	presetTemp(void)				{ return temp_set; }
		virtual uint16_t	averageTemp(void)				{ return h_temp.read(); }
		virtual uint16_t 	tmpDispersion(void)				{ return d_temp.read(); }
		virtual uint16_t	pwrDispersion(void)				{ return d_power.read(); }
		virtual uint16_t    getMaxFixedPower(void)			{ return max_fix_power; }
		bool				isCold(void)					{ return (mode == POWER_OFF); }
		int32_t				tempShortAverage(int32_t t)		{ return t_iron_short.average(t);				}
		void				resetShortTemp(void)			{ t_iron_short.reset();							}
		void				setCheckPeriod(uint8_t t)		{ check_period = check_time = t;				}
		virtual void  		setTemp(uint16_t t);			// Set the temperature to be kept (internal units)
		virtual uint16_t    avgPower(void);					// Average applied power
		virtual uint8_t     avgPowerPcnt(void);				// Power applied to the IRON in percents
		virtual void		fixPower(uint16_t Power);		// Set the specified power to the the soldering IRON
		void 				adjust(uint16_t t);				// Adjust preset temperature depending on ambient temperature
		uint16_t			power(int32_t t);				// Required power to keep preset temperature
		void				reset(void);					// Iron is disconnected, clear the temp history
		void        		lowPowerMode(uint16_t t);		// Activate low power mode (preset temp.) To disable, use switchPower(true)
		void				boostPowerMode(uint16_t t);		// Activate boost power mode
	private:
		uint16_t 	temp_set				= 0;			// The temperature that should be kept
		uint16_t	temp_low				= 0;			// The temperature in low power mode (if not zero)
		uint16_t	temp_boost				= 0;			// The temperature in boost mode (if not zero)
		uint16_t    fix_power				= 0;			// Fixed power value of the IRON (or zero if off)
		volatile 	PowerMode	mode		= POWER_OFF;	// Working mode of the IRON
		volatile 	bool 		chill		= false;		// Whether the IRON should be cooled (preset temp is lower than current)
		volatile	uint16_t	temp_curr	= 0;			// The actual IRON temperature
		volatile 	uint8_t		check_period= 0;			// The period to check the current through the IRON
		volatile	uint8_t		check_time	= 0;			// The time when to check the current through the IRON
		EMP_AVERAGE t_iron_short;							// Exponential average of the IRON temperature (short period)
		EMP_AVERAGE h_power;								// Exponential average of applied power
		EMP_AVERAGE	h_temp;									// Exponential average of temperature
		EMP_AVERAGE d_power;								// Exponential average of power math dispersion
		EMP_AVERAGE d_temp;									// Exponential temperature math dispersion
		bool		t_reset					= false;		// The temperature value was reset
		const uint16_t	max_power      		= 1960;			// Maximum power to the IRON
		const uint16_t	max_fix_power  		= 1000;			// Maximum power in fixed power mode
		const uint8_t	ec	   				= 20;			// Exponential average coefficient
		const uint16_t	iron_cold			= 100;			// The internal temperature when the IRON is cold
		const uint8_t	iron_emp_coeff		= 8;			// Exponential average coefficient for IRON temperature
		const uint16_t	iron_off_value		= 500;
		const uint16_t	iron_on_value		= 1000;
		const uint8_t	iron_sw_len			= 3;			// Exponential coefficient of current through the IRON switch
		const uint8_t	sw_off_value		= 14;
		const uint8_t	sw_on_value			= 20;
		const uint8_t	sw_avg_len			= 5;
		const uint8_t	sw_tilt_len			= 2;
};

#endif
