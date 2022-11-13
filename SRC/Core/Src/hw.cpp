/*
 * hw.cpp
 *
 *  Created on: 13 June 2022
 *      Author: Alex
 *
 *  2022 Nov 6
 *  	added t_amb.reset() to the HW::init() method to initialize the default ambient temperature
 */

#include <math.h>
#include "hw.h"

CFG_STATUS HW::init(void) {
	dspl.init();
	t_amb.length(ambient_emp_coeff);
	t_amb.reset(2000);										// Initialize the ambient temperature near 25 degrees (ADC value = 2000)
	iron.init();
	hotgun.init();
	i_enc.addButton(I_ENC_B_GPIO_Port, I_ENC_B_Pin);
	g_enc.addButton(G_ENC_B_GPIO_Port, G_ENC_B_Pin);
	CFG_STATUS cfg_init = 	cfg.init();
	if (cfg_init == CFG_OK || cfg_init == CFG_NO_TIP) {		// Load NLS configuration data
		nls.init(&dspl);									// Setup pointer to NLS_MSG class instance to setup messages by NLS_MSG::set() method
		const char *l = cfg.getLanguage();					// Configured language name (string)
		nls.loadLanguageData(l);
		uint8_t *font = nls.font();
		dspl.setLetterFont(font);
		uint8_t r = cfg.getDsplRotation();
		dspl.rotate((tRotation)r);
	} else {
		dspl.setLetterFont(0);								// Set default font, reallocate the bitmap for 3-digits field
		dspl.rotate(TFT_ROTATION_90);
	}
	cfg.umount();
	PIDparam pp   		= 	cfg.pidParams(d_t12);			// load IRON PID parameters
	iron.load(pp);
	pp					=	cfg.pidParams(d_gun);			// load Hot Air Gun PID parameters
	hotgun.load(pp);
	buzz.activate(cfg.isBuzzerEnabled());
	i_enc.setClockWise(cfg.isIronEncClockWise());
	g_enc.setClockWise(cfg.isGunEncClockWise());
	return cfg_init;
}

/*
 * Return ambient temperature in Celsius
 * Caches previous result to skip expensive calculations
 */
int32_t	HW::ambientTemp(void) {
static const uint16_t add_resistor	= 10000;				// The additional resistor value (10koHm)
static const float 	  normal_temp[2]= { 10000, 25 };		// nominal resistance and the nominal temperature
static const uint16_t beta 			= 3950;     			// The beta coefficient of the thermistor (usually 3000-4000)
static int32_t	average 			= 0;					// Previous value of analog read
static int 		cached_ambient 		= 0;					// Previous value of the temperature

	if (noAmbientSensor()) return default_ambient;			// If IRON handle is not connected, return default ambient temperature
	if (abs(t_amb.read() - average) < 20)
		return cached_ambient;

	average = t_amb.read();

	if (average < max_ambient_value) {						// prevent division by zero; About -30 degrees
		// convert the value to resistance
		float resistance = 4095.0 / (float)average - 1.0;
		resistance = (float)add_resistor / resistance;

		float steinhart = resistance / normal_temp[0];		// (R/Ro)
		steinhart = log(steinhart);							// ln(R/Ro)
		steinhart /= beta;									// 1/B * ln(R/Ro)
		steinhart += 1.0 / (normal_temp[1] + 273.15);  		// + (1/To)
		steinhart = 1.0 / steinhart;						// Invert
		steinhart -= 273.15;								// convert to Celsius
		cached_ambient	= round(steinhart);
	} else {
		cached_ambient	= default_ambient;
	}
	return cached_ambient;
}
