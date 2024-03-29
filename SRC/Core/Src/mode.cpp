/*
 * mode.cpp
 *
 * 2022 NOV 26
 *     Changed the uint8_t p = 3; in case MM_STANDBY_TIME of MSETUP::loop() method
 *
 * 2022 DEC 26
 *     Changed MDEBUG::loop() to debug the GUN timer period
 *
 * 2024 MAR 28
 *	   Changed the MFAIL::loop() to manage long press of gun encoder button
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mode.h"
#include "cfgtypes.h"
#include "core.h"
#include "unit.h"

//---------------------- The Menu mode -------------------------------------------
void MODE::setup(MODE* return_mode, MODE* short_mode, MODE* long_mode) {
	mode_return	= return_mode;
	mode_spress	= short_mode;
	mode_lpress	= long_mode;
}

MODE* MODE::returnToMain(void) {
	if (mode_return && time_to_return && HAL_GetTick() >= time_to_return)
		return mode_return;
	return this;
}

void MODE::resetTimeout(void) {
	if (timeout_secs) {
		time_to_return = HAL_GetTick() + timeout_secs * 1000;
	}
}
void MODE::setTimeout(uint16_t t) {
	timeout_secs = t;
}

UNIT* MODE::unit(void) {
	UNIT*	pUnit	= 0;
	switch (dev_type) {
		case d_t12:
			pUnit = (UNIT*)&pCore->iron;
			break;
		case d_gun:
		default:
			pUnit = (UNIT*)&pCore->hotgun;
			break;
	}
	return pUnit;
}

void MWORK::init(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;
	RENC*	pIE		= &pCore->i_enc;
	RENC*	pGE		= &pCore->g_enc;

	pCore->iron.setCheckPeriod(6);							// Start checking the current through T12 IRON
	bool		celsius 	= pCFG->isCelsius();
	int16_t  	ambient		= pCore->ambientTemp();
	i_old_temp_set			= pCFG->tempPresetHuman();
	uint16_t 	i_temp_set	= pCFG->humanToTemp(i_old_temp_set, ambient, d_t12);
	uint16_t	fan			= pCFG->gunFanPreset();
	g_old_temp_set			= pCFG->gunTempPreset();
	uint16_t 	g_temp_set	= pCFG->humanToTemp(g_old_temp_set, ambient, d_gun);

	pIron->setTemp(i_temp_set);
	pHG->setFan(fan);
	pHG->setTemp(g_temp_set);

	uint16_t it_min		= pCFG->tempMinC(d_t12);			// The minimum IRON preset temperature, defined in main.h
	uint16_t it_max		= pCFG->tempMaxC(d_t12);			// The maximum IRON preset temperature
	uint16_t gt_min		= pCFG->tempMinC(d_gun);			// The minimum GUN preset temperature, defined in main.h
	uint16_t gt_max		= pCFG->tempMaxC(d_gun);			// The maximum GUN preset temperature
	if (!celsius) {											// The preset temperature saved in selected units
		it_min	= celsiusToFahrenheit(it_min);
		it_max	= celsiusToFahrenheit(it_max);
		gt_min	= celsiusToFahrenheit(gt_min);
		gt_max	= celsiusToFahrenheit(gt_max);
	}
	if (pCFG->isBigTempStep()) {							// The preset temperature step is 5 degrees
		i_old_temp_set -= i_old_temp_set % 5;				// The preset temperature should be rounded to 5
		pIE->reset(i_old_temp_set, it_min, it_max, 5, 5, false);
		pGE->reset(g_old_temp_set, gt_min, gt_max, 5, 5, false);
	} else {
		pIE->reset(i_old_temp_set, it_min, it_max, 1, 1, false);
		pGE->reset(g_old_temp_set, gt_min, gt_max, 1, 1, false);
	}

	pD->clear();
	pD->drawTipName(pCFG->tipName(d_t12), pCFG->isTipCalibrated(), u_upper);
	pD->drawTempSet(i_old_temp_set, u_upper);
	pD->msgOFF(u_lower);
	pD->drawTempSet(g_old_temp_set, u_lower);
	pD->drawFanPcnt(pHG->presetFanPcnt());
	pD->drawAmbient(ambient, celsius);
	pD->stopFan();
	if (start && pCFG->isAutoStart()) {
		pIron->switchPower(true);
		i_phase	= IRPH_HEATING;
		pD->msgON(u_upper);
		start = false;										// Prevent to start IRON automatically when return from menu
	} else {
		pIron->switchPower(false);
		i_phase	= pIron->isCold()?IRPH_OFF:IRPH_COOLING;
		pD->msgOFF(u_upper);
	}
	fan_blowing		= false;
	edit_temp		= true;									// Turn to edit temperature mode on the Hot Air Gun
	return_to_temp	= 0;
	update_screen	= 0;									// Force to redraw the screen
	tilt_time		= 0;									// No tilt change
	alt_temp		= 0;									// Low power or boost temperature not used
	fan_animate		= 0;									// Not animate the fan
	lowpower_time	= 0;									// Reset the low power mode time
}

void MWORK::adjustPresetTemp(void) {
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;

	uint16_t presetTemp	= pIron->presetTemp();
	uint16_t tempH     	= pCFG->tempPresetHuman();
	int16_t  ambient	= pCore->ambientTemp();
	uint16_t temp  		= pCFG->humanToTemp(tempH, ambient, d_t12); // Expected temperature of IRON in internal units
	if (temp != presetTemp) {								// The ambient temperature have changed, we need to adjust preset temperature
		pIron->adjust(temp);
		pCore->dspl.drawAmbient(ambient, pCFG->isCelsius());
	}
}

bool MWORK::hwTimeout(bool tilt_active) {
	CFG*	pCFG	= &pCore->cfg;

	uint32_t now_ms = HAL_GetTick();
	if (lowpower_time == 0 || tilt_active) {				// If the IRON is used, reset standby time
		lowpower_time = now_ms + pCFG->getLowTO() * 5000;	// Convert timeout (5 secs interval) to milliseconds
	}
	if (now_ms >= lowpower_time) {
		return true;
	}
	return false;
}

// Use applied power analysis to automatically power-off the IRON
void MWORK::swTimeout(uint16_t temp, uint16_t temp_set, uint16_t temp_setH, uint32_t td, uint32_t pd, uint16_t ap) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;

	int ip = idle_pwr.read();
	if ((temp <= temp_set) && (temp_set - temp <= 4) && (td <= 200) && (pd <= 25)) {
		// Evaluate the average power in the idle state
		ip = idle_pwr.average(ap);
	}

	// Check the IRON current status: idle or used
	if (abs(ap - ip) >= 150) {								// The applied power is different than idle power. The IRON being used!
		swoff_time 			= HAL_GetTick() + pCFG->getOffTimeout() * 60000;
		pD->msgON(u_upper);
		i_phase = IRPH_NORMAL;
	} else {												// The IRON is in its idle state
		if (swoff_time == 0)
			swoff_time 	= HAL_GetTick() + pCFG->getOffTimeout() * 60000;
		uint32_t to = (swoff_time - HAL_GetTick()) / 1000;
		if (to < 100) {
			pD->timeToOff(to);								// Show the time remaining to switch off the IRON
			if (i_phase != IRPH_GOINGOFF) {
				pCore->buzz.lowBeep();
				i_phase = IRPH_GOINGOFF;
			}
		} else {
			pD->msgIdle(u_upper);
		}
	}
}

// The IRON encoder button short press callback
void MWORK::changeIronShort(void) {
	switch (i_phase) {
		case IRPH_OFF:										// The IRON is powered OFF, switch it ON
		case IRPH_COOLING:
		case IRPH_COLD:
			pCore->iron.switchPower(true);
			i_phase	= IRPH_HEATING;
			pCore->dspl.msgON(u_upper);
			break;
		case IRPH_NOHANDLE:									// Cannot change the IROn state, the handle is not connected
			break;
		default:											// Switch off the IRON
			pCore->iron.switchPower(false);
			i_phase	= IRPH_COOLING;
			pCore->dspl.msgOFF(u_upper);
			pCore->cfg.saveConfig();
			pCore->dspl.drawTempSet(pCore->cfg.tempPresetHuman(), u_upper);
			pCore->dspl.drawPower(0, u_upper);
			break;
	}
	lowpower_time	= 0;									// User activity detected, reset the low mode time
}

// The IRON encoder button long press callback
void MWORK::changeIronLong(void) {
	switch (i_phase) {
		case IRPH_OFF:										// IRON is powered off, do nothing
		case IRPH_COOLING:
		case IRPH_COLD:
			pCore->buzz.shortBeep();
			pCore->iron.switchPower(true);
			i_phase	= IRPH_HEATING;
			pCore->dspl.msgON(u_upper);
			break;
		case IRPH_NOHANDLE:
			pCore->buzz.failedBeep();
			break;
		case IRPH_BOOST:									// The IRON is in the boost mode, return to the normal mode
			pCore->iron.switchPower(true);
			pCore->dspl.drawTempSet(pCore->cfg.tempPresetHuman(), u_upper);
			pCore->dspl.msgON(u_upper);
			i_phase	= IRPH_HEATING;
			phase_end	= 0;
			pCore->buzz.shortBeep();
			break;
		default:											// The IRON is working, go to the BOOST mode
		{
			uint8_t  bt		= pCore->cfg.boostTemp();		// Additional temperature (Degree)
			uint32_t bd		= pCore->cfg.boostDuration();	// Boost duration (s)
			if (bt > 0 && bd > 0) {
				if (!pCore->cfg.isCelsius())
					bt = (bt * 9 + 3) / 5;
				int16_t  ambient = pCore->ambientTemp();
				uint16_t tset	= pCore->iron.presetTemp();		// Current preset temperature, internal units
				alt_temp	= pCore->cfg.tempToHuman(tset, ambient, d_t12) + bt;
				tset			= pCore->cfg.humanToTemp(alt_temp, ambient, d_t12);
				pCore->iron.boostPowerMode(tset);
				pCore->dspl.msgBoost(u_upper);
				i_phase = IRPH_BOOST;
				phase_end	= HAL_GetTick() + (uint32_t)bd * 1000;
				pCore->dspl.drawTempSet(alt_temp, u_upper);
				pCore->buzz.shortBeep();
			}
		}
			break;
	}
	lowpower_time	= 0;									// User activity detected, reset the low mode time
}

// The IRON encoder rotated callback
bool MWORK::ironEncoder(uint16_t new_value) {
	lowpower_time	= 0;									// User activity detected, reset the low mode time
	switch (i_phase) {
		case IRPH_NOHANDLE:
		case IRPH_BOOST:
			return false;
		case IRPH_OFF:
			return true;
		case IRPH_LOWPWR:
		case IRPH_GOINGOFF:
			pCore->iron.switchPower(true);
			i_phase = IRPH_HEATING;
			pCore->dspl.msgON(u_upper);
			return false;
		default:
			break;
	}
	int16_t  ambient	= pCore->ambientTemp();
	uint16_t tsetH		= pCore->cfg.tempPresetHuman();
	uint16_t tset		= pCore->cfg.humanToTemp(tsetH, ambient, d_t12);
	pCore->iron.setTemp(tset);
	i_phase = IRPH_HEATING;
	return true;
}

void MWORK::ironPhaseEnd(void) {
	switch (i_phase) {
		case IRPH_READY:
			i_phase = IRPH_NORMAL;
			break;
		case IRPH_BOOST:
			pCore->iron.switchPower(true);
			pCore->dspl.msgON(u_upper);
			i_phase	= IRPH_HEATING;
			pCore->buzz.lowBeep();
			pCore->dspl.drawTempSet(i_old_temp_set, u_upper); // redraw actual temperature
			break;
		case IRPH_LOWPWR:
		case IRPH_GOINGOFF:
			i_phase = IRPH_COOLING;
			pCore->iron.switchPower(false);
			pCore->dspl.msgOFF(u_upper);
			pCore->dspl.drawTempSet(i_old_temp_set, u_upper); // redraw actual temperature
			break;
		case IRPH_COLD:
			i_phase = IRPH_OFF;
			pCore->dspl.msgOFF(u_upper);
			break;
		default:
			break;
	}
	phase_end = 0;
}

bool MWORK::idleMode(void) {
	IRON*	pIron		= &pCore->iron;
	int temp			= pIron->averageTemp();
	int temp_set		= pIron->presetTemp();				// Now the preset temperature in internal units!!!
	uint16_t temp_set_h = pCore->cfg.tempPresetHuman();

	uint32_t td			= pIron->tmpDispersion();			// The temperature dispersion
	uint32_t pd 		= pIron->pwrDispersion();			// The power dispersion
	int ap      		= pIron->avgPower();				// Actually applied power to the IRON

	// Check the IRON reaches the preset temperature
	if ((abs(temp_set - temp) < 6) && (td <= 500) && (ap > 0))  {
	    if (i_phase == IRPH_HEATING) {
	    	i_phase = IRPH_READY;
			phase_end = HAL_GetTick() + 2000;
	    	pCore->dspl.msgReady(u_upper);
	    	pCore->buzz.shortBeep();
	    }
	}

	bool low_power_enabled = pCore->cfg.getLowTemp() > 0;
	bool tilt_active = false;
	if (low_power_enabled)									// If low power mode enabled, check tilt switch status
		tilt_active = pIron->isReedSwitch(pCore->cfg.isReedType());	// True if iron was used

	// If the low power mode is enabled, check the IRON status
	int16_t  ambient = pCore->ambientTemp();
	if (i_phase == IRPH_NORMAL) {							// The IRON has reaches the preset temperature and 'Ready' message is already cleared
		if (low_power_enabled) {							// Use hardware tilt switch if low power mode enabled
			if (hwTimeout(tilt_active)) {
				uint16_t temp 	 = pCore->cfg.lowTempInternal(ambient, d_t12);
				alt_temp	 	 = pCore->cfg.getLowTemp();
				pIron->lowPowerMode(temp);
				i_phase = IRPH_LOWPWR;						// Switch to low power mode
				phase_end = HAL_GetTick() + pCore->cfg.getOffTimeout() * 60000;
				pCore->dspl.msgStandby(u_upper);
				pCore->dspl.drawTempSet(alt_temp, u_upper);
			}
		} else if (pCore->cfg.getOffTimeout() > 0) {		// Do not use tilt switch, use software auto-off feature
			swTimeout(temp, temp_set, temp_set_h, td, pd, ap); // Update time_to_return value based IRON status
		}
	} else if (i_phase == IRPH_LOWPWR && tilt_active) {		// Re-activate the IRON in normal mode
		pIron->switchPower(true);
		i_phase = IRPH_HEATING;
		lowpower_time = 0;									// Reset the lowpower mode timeout
		pCore->dspl.msgON(u_upper);
		uint16_t t_set = pCore->iron.presetTemp();
		t_set = pCore->cfg.tempToHuman(t_set, ambient, d_t12);
		pCore->dspl.drawTempSet(t_set, u_upper);			// Redraw the preset temperature
	}
	return tilt_active;
}

MODE* MWORK::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;
	RENC*	pIE		= &pCore->i_enc;
	RENC*	pGE		= &pCore->g_enc;

	//  Manage the Hot Air Gun by The REED switch
    if (pHG->isReedSwitch(true)) {							// The Reed switch is open, switch the Hot Air Gun ON
    	if (!pHG->isOn()) {
			//pD->msgON(d_gun);
			pHG->switchPower(true);
		}
    } else {												// The Reed switch is closed, switch the Hot Air Gun OFF
		if (pHG->isOn()) {
			pD->msgOFF(u_lower);
			pHG->switchPower(false);
			pD->drawPower(0, u_lower);
			fan_animate = 0;
			pCore->cfg.saveConfig();
		}
	}

	// Check the T12 IRON is connected
	if (i_phase != IRPH_NOHANDLE) { 						// The T12 handle is not connected
		if (pCore->noAmbientSensor()) {
			i_phase = IRPH_NOHANDLE;
			pIron->setCheckPeriod(0);						// Stop checking the current through the T12 IRON
		}
	} else {
		if (!pCore->noAmbientSensor()) {
			i_phase = IRPH_OFF;
			pIron->setCheckPeriod(6);						// Start checking the current through the T12 IRON
		}
	}

	// if IRON tip is disconnected, activate Tip selection mode
    if (mode_spress && !pIron->isConnected() && isACsine()) {
    	if (i_phase == IRPH_OFF || i_phase == IRPH_COOLING || i_phase == IRPH_COLD) {
    		mode_spress->useDevice(d_t12);
    		return mode_spress;
    	}
	}

    uint16_t temp_set_h = pIE->read();
    uint8_t  button		= pIE->buttonStatus();
    if (button == 1) {										// The IRON encoder button pressed shortly, change the working mode
		changeIronShort();
		update_screen = 0;
    } else if (button == 2) {								// The IRON encoder button was pressed for a long time, activate the main menu
		changeIronLong();
		update_screen = 0;
    }

    if (temp_set_h != i_old_temp_set) {						// Preset temperature changed
    	if (ironEncoder(temp_set_h)) {
			i_old_temp_set = temp_set_h;
			if (i_phase != IRPH_BOOST && i_phase != IRPH_LOWPWR) {
				pCFG->savePresetTempHuman(i_old_temp_set);
				pD->drawTempSet(i_old_temp_set, u_upper);
				idle_pwr.reset();
			}
		}
		update_screen = 0;
    }

	if (phase_end > 0 && HAL_GetTick() >= phase_end) {
		ironPhaseEnd();
	}

    // The fan speed modification mode has 'return_to_temp' timeout
	if (return_to_temp && HAL_GetTick() >= return_to_temp) {// This reads the Hot Air Gun configuration Also
		bool celsius 		= pCFG->isCelsius();
		g_old_temp_set		= pCFG->gunTempPreset();
		uint16_t t_min		= pCFG->tempMinC(d_gun);		// The minimum preset temperature, defined in iron.h
		uint16_t t_max		= pCFG->tempMaxC(d_gun);		// The maximum preset temperature
		if (!celsius) {										// The preset temperature saved in selected units
			t_min	= celsiusToFahrenheit(t_min);
			t_max	= celsiusToFahrenheit(t_max);
		}
		if (pCFG->isBigTempStep()) {						// The preset temperature step is 5 degrees
			g_old_temp_set -= g_old_temp_set % 5;			// The preset temperature should be rounded to 5
			pGE->reset(g_old_temp_set, t_min, t_max, 5, 5, false);
		} else {
			pGE->reset(g_old_temp_set, t_min, t_max, 1, 1, false);
		}
		edit_temp		= true;
		pD->drawFanPcnt(pHG->presetFanPcnt());				// Redraw in standard mode
		return_to_temp	= 0;
	}

    temp_set_h	= pGE->read();
    button		= pGE->buttonStatus();
    if (button == 1) {										// Short press, the HOT AIR GUN button was pressed, toggle temp/fan
    	if (edit_temp) {									// Switch to edit fan speed
    		uint16_t fan = pHG->presetFan();
    		uint16_t max = pHG->maxFanSpeed();
    		pGE->reset(fan, 0, max, 5, 10, false);
    		edit_temp 		= false;
    		g_old_temp_set	= fan;
    		temp_set_h		= fan;
    		return_to_temp	= HAL_GetTick() + edit_fan_timeout;
    		pD->drawFanPcnt(pHG->presetFanPcnt(), true);
			update_screen 	= 0;
    	} else {
    		return_to_temp	= HAL_GetTick();				// Force to return to edit temperature
    		return this;
    	}
    } else if (button == 2) {
    	if (mode_lpress) {									// Go to the main menu
    		pCore->buzz.shortBeep();
    		return mode_lpress;
    	}
    }

    if (temp_set_h != g_old_temp_set) {						// Changed preset temperature or fan speed
    	g_old_temp_set = temp_set_h;						// In first loop the preset temperature will be setup for sure
    	uint16_t t	= pHG->presetTemp();					// Internal units
    	uint16_t f	= pHG->presetFan();
		int16_t  ambient	= pCore->ambientTemp();
    	t = pCFG->tempToHuman(t, ambient, d_gun);
    	if (edit_temp) {
    		t = temp_set_h;									// New temperature value
    		pD->drawTempSet(temp_set_h, u_lower);
    		uint16_t g_temp_set	= pCFG->humanToTemp(g_old_temp_set, ambient, d_gun);
    		pHG->setTemp(g_temp_set);
    	} else {
    		f = temp_set_h;									// New fan value
    		pD->drawFanPcnt(pHG->presetFanPcnt(), true);
    		pHG->setFan(f);
    		return_to_temp	= HAL_GetTick() + edit_fan_timeout;
    	}
    	pCFG->saveGunPreset(t, f);
    }

	if (pHG->isFanWorking() && HAL_GetTick() >= fan_animate && pHG->isConnected()) {
		int16_t  temp		= pHG->averageTemp();
		int16_t  temp_s		= pHG->presetTemp();
		pD->animateFan(temp-temp_s);
		fan_animate = HAL_GetTick() + 100;
	}

    if (HAL_GetTick() < update_screen) return this;
    update_screen = HAL_GetTick() + period;

	if (i_phase == IRPH_COOLING && pIron->isCold()) {
    	pD->msgCold(u_upper);
    	pCore->buzz.lowBeep();
		i_phase = IRPH_COLD;
		phase_end = HAL_GetTick() + 20000;
	}

	if (i_phase != IRPH_OFF && HAL_GetTick() > tilt_time) {	// Time to redraw tilt switch status
		if (idleMode()) {									// tilt switch is active
			tilt_time = HAL_GetTick() + tilt_show_time;
			pD->ironActive(true, u_upper);
		} else if (tilt_time > 0) {
			tilt_time = 0;
			pD->ironActive(false, u_upper);
		}
	}

	// Perhaps, we should show time to shutdown
	if (i_phase == IRPH_LOWPWR && pCore->cfg.getLowTemp() > 0) {
		uint32_t to = (phase_end - HAL_GetTick()) / 1000;
		if (to < 100) {
			pD->timeToOff(to);								// Show the time remaining to switch off the IRON
		}
	}

	adjustPresetTemp();

	int16_t	 ambient	= pCore->ambientTemp();
	uint16_t temp  		= pIron->averageTemp();
	uint16_t i_temp_h 	= pCFG->tempToHuman(temp, ambient, d_t12);
	temp				= pIron->presetTemp();
	uint16_t i_temp_s	= pCFG->tempToHuman(temp, ambient, d_t12);
	uint8_t	 ipwr		= pIron->avgPowerPcnt();
	temp				= pHG->averageTemp();
	uint16_t g_temp_h	= pCFG->tempToHuman(temp, ambient, d_gun);
	temp				= pHG->presetTemp();
	uint16_t g_temp_s	= pCFG->tempToHuman(temp, ambient, d_gun);
	uint8_t	 gpwr		= pHG->avgPowerPcnt();
	if (i_phase == IRPH_HEATING || i_phase == IRPH_NORMAL) {
		pD->animatePower(u_upper, i_temp_h - i_temp_s);

	} else if (i_phase == IRPH_OFF && pIron->isCold()) {	// Prevent the temperature changing when the IRON or Hot Gun is off and cold
		i_temp_h = ambient;
	}
	if (!pHG->isFanWorking()) {
		if (fan_blowing) {
			pD->stopFan();
			fan_blowing = false;
			pD->msgOFF(u_lower);
		}
		g_temp_h = ambient;
	} else {
		fan_blowing	= true;
		if (pHG->isOn()) {
			pD->animatePower(u_lower, g_temp_h - g_temp_s);
		}
	}

	bool celsius = pCFG->isCelsius();
	if (i_phase == IRPH_COOLING) {
		pD->animateTempCooling(i_temp_h, celsius, u_upper);
	} else if (i_phase == IRPH_NOHANDLE) {
		pD->drawTemp(ambient, u_upper);						// The T12 iron is not connected, show the ambient temperature
	} else {
		pD->drawTemp(i_temp_h, u_upper);
	}
	if (alt_temp > 0 && (i_phase == IRPH_BOOST || i_phase == IRPH_LOWPWR))
		i_temp_s = alt_temp;
	pD->drawTempGauge(i_temp_h-i_temp_s, u_upper, i_phase != IRPH_OFF && i_phase != IRPH_NOHANDLE);
	pD->drawPower(ipwr, u_upper);
	if (fan_blowing && !pHG->isOn()) {
		pD->animateTempCooling(g_temp_h, celsius, u_lower);
	} else {
		pD->drawTemp(g_temp_h, u_lower);
	}
	pD->drawTempGauge(g_temp_h-g_temp_s, u_lower, /*pHG->isOn()*/fan_blowing);
	pD->drawPower(gpwr, u_lower);
	return this;
}

//---------------------- The tip selection mode ----------------------------------
void MSLCT::init(void) {;
	CFG*	pCFG	= &pCore->cfg;

	manual_change		= false;
	tip_disconnected	= 0;
	if (dev_type == d_unknown) {							// Manual TIP selection mode
		manual_change = true;
		dev_type = d_t12;
	}
	if (!manual_change) {
		if (dev_type == d_t12) {
			pCore->iron.setCheckPeriod(3);					// Start checking the current through the T12 IRON rapidly
			tip_disconnected  = HAL_GetTick();				// As soon as the current would detected, the mode can be finished
		}
	}
	uint8_t tip_index 	= pCFG->currentTipIndex(dev_type);
	// Build list of the active tips; The current tip is the second element in the list
	uint8_t list_len 	= pCFG->tipList(tip_index, tip_list, MSLCT_LEN, true);

	// The current tip could be inactive, so we should find nearest tip (by ID) in the list
	uint8_t closest		= 0;								// The index of the list of closest tip ID
	uint8_t diff  		= 0xff;
	for (uint8_t i = 0; i < list_len; ++i) {
		uint8_t delta;
		if ((delta = abs(tip_index - tip_list[i].tip_index)) < diff) {
			diff 	= delta;
			closest = i;
		}
	}
	pCore->g_enc.reset(closest, 0, list_len-1, 1, 1, false);
	tip_begin_select = HAL_GetTick();						// We stared the tip selection procedure
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_SELECT_TIP);
	old_index = list_len;
	update_screen	= 0;									// Force to redraw the screen
}

MODE* MSLCT::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;
	UNIT*	pUnit	= unit();								// Unit can be Hakko T12 or JBC IRON

	uint8_t	 index	= pEnc->read();
	uint8_t	button	= pEnc->buttonStatus();

	if (old_index != index) {
		old_index			= index;
		tip_begin_select 	= 0;
		update_screen 		= 0;
	}

	if (button > 0 && manual_change) { 						// The button was pressed
		changeTip(index);
		return mode_return;
	}

	if (!manual_change && tip_disconnected > 0 && (pUnit->isConnected() || !isACsine())) {	// See core.cpp for isACsine()
		// Prevent bouncing event, when the IRON connection restored back too quickly.
		if (tip_begin_select && (HAL_GetTick() - tip_begin_select) < 1000) {
			return 0;
		}
		if (HAL_GetTick() > tip_disconnected + 1000) {		// Wait at least 1 second before reconnect the IRON tip again
			changeTip(index);
			return mode_return;
		}
	}

    if (button == 2) {										// The button was pressed for a long time
	    return mode_lpress;
	}

    if (tip_disconnected == 0 && !pUnit->isConnected()) {	// Finally, the IRON tip has been disconnected
    	tip_disconnected = HAL_GetTick();					// Now the tip is disconnected
    }

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 20000;

	for (int8_t i = index; i >= 0; --i) {
		if (tip_list[(uint8_t)i].name[0]) {
			index = i;
			break;
		}
	}
	uint8_t tip_index = tip_list[index].tip_index;
	for (uint8_t i = 0; i < MSLCT_LEN; ++i)
		tip_list[i].name[0] = '\0';
	uint8_t list_len = pCFG->tipList(tip_index, tip_list, MSLCT_LEN, true);
	if (list_len == 0)										// There is no active tip in the list
		return mode_spress;									// Activate tips mode

	uint8_t ii = 0;
	for (uint8_t i = 0; i < list_len; ++i) {
		if (tip_index == tip_list[i].tip_index) {
			ii = i;
			break;
		}
	}
	pCore->g_enc.reset(ii, 0, list_len-1, 1, 1, false);
	pD->drawTipList(tip_list, list_len, tip_index, true);
	return this;
}

void MSLCT::changeTip(uint8_t index) {
	uint8_t tip_index = tip_list[index].tip_index;
	pCore->cfg.changeTip(tip_index);
	// Clear temperature history and switch iron mode to "power off"
	pCore->iron.reset();
}

//---------------------- The Activate tip mode: select tips to use ---------------
void MTACT::init(void) {
	CFG*	pCFG	= &pCore->cfg;

	uint8_t tip_index = pCFG->currentTipIndex(d_t12);
	pCore->g_enc.reset(tip_index, 1, pCFG->TIPS::loaded()-1, 1, 1, false);	// Start from tip #1, because 0-th 'tip' is a Hot Air Gun
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_ACTIVATE_TIPS);
	old_index = 255;
	update_screen = 0;
}

MODE* MTACT::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t	tip_index 	= pEnc->read();
	uint8_t	button		= pEnc->buttonStatus();

	if (button == 1) {										// The button pressed
		pD->BRGT::dim(50);									// Turn-off the brightness, processing
		if (!pCFG->toggleTipActivation(tip_index)) {
			pFail->setMessage(MSG_EEPROM_WRITE);
			return 0;
		}
		pD->BRGT::on();										// Restore the display brightness
		update_screen = 0;									// Force redraw the screen
	} else if (button == 2) {								// Tip activation finished
		pCFG->close();										// Finish tip list editing
		pCFG->reloadTips();
		return mode_lpress;
	}

	if (old_index != tip_index) {
		old_index = tip_index;
		update_screen = 0;
	}

	if (HAL_GetTick() >= update_screen) {
		TIP_ITEM	tip_list[7];
		uint8_t loaded = pCFG->tipList(tip_index, tip_list, 7, false);
		pD->drawTipList(tip_list, loaded, tip_index, false);
		update_screen = HAL_GetTick() + 60000;
	}
	return this;
}

//---------------------- The Menu mode -------------------------------------------
MMENU::MMENU(HW* pCore, MODE* m_boost, MODE *m_change_tip, MODE *m_params, MODE* m_calib, MODE* m_act, MODE* m_tune,
		MODE* m_pid, MODE* m_gun_menu, MODE *m_about) : MODE(pCore) {
	mode_menu_boost		= m_boost;
	mode_change_tip		= m_change_tip;
	mode_menu_setup		= m_params;
	mode_calibrate_menu	= m_calib;
	mode_activate_tips	= m_act;
	mode_tune			= m_tune;
	mode_tune_pid		= m_pid;
	mode_gun_menu		= m_gun_menu;
	mode_about			= m_about;
}

void MMENU::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	if (!pCFG->isTipCalibrated())
		mode_menu_item	= MM_CALIBRATE_TIP;					// Index of 'calibrate tip' menu item
	uint8_t menu_len = pCore->dspl.menuSize(MSG_MENU_MAIN);
	pEnc->reset(mode_menu_item, 0, menu_len-1, 1, 1, true);
	update_screen = 0;
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_MENU_MAIN);					// "Main menu"
}

MODE* MMENU::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t item 		= pEnc->read();
	uint8_t  button		= pEnc->buttonStatus();

	// Change the configuration parameters value in place
	if (mode_menu_item != item) {							// The encoder has been rotated
		mode_menu_item = item;
		update_screen = 0;									// Force to redraw the screen
	}

	// Going through the main menu
	if (button > 0) {										// The button was pressed, current menu item can be selected for modification
		switch (item) {										// item is a menu item
			case MM_PARAMS:
				return mode_menu_setup;
			case MM_BOOST_SETUP:							// Boost parameters
				return mode_menu_boost;
			case MM_CHANGE_TIP:								// Change tip
				mode_change_tip->useDevice(d_unknown);		// Activate the manual tip change mode
				return mode_change_tip;
			case MM_CALIBRATE_TIP:							// calibrate IRON tip
				mode_menu_item = 0;
				return mode_calibrate_menu;
			case MM_ACTIVATE_TIPS:							// activate tips
				return mode_activate_tips;
			case MM_TUNE_IRON:								// tune the IRON potentiometer
				mode_menu_item = 0;							// We will not return from tune mode to this menu
				mode_tune->useDevice(d_t12);
				return mode_tune;
			case MM_GUN_MENU:								// Hot Air Gun menu
				return mode_gun_menu;
			case MM_RESET_CONFIG:							// Initialize the configuration
				pCFG->clearConfig();
				mode_menu_item = 0;							// We will not return from tune mode to this menu
				return mode_return;
			case MM_TUNE_IRON_PID:							// Tune PID
				mode_tune_pid->useDevice(d_t12);			// Tune IRON
				return mode_tune_pid;
			case MM_ABOUT:									// About dialog
				mode_menu_item = 0;
				return mode_about;
			default:										// quit
				mode_menu_item = 0;
				return mode_return;
		}
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 10000;
	pD->menuShow(MSG_MENU_MAIN, item, 0, false);
	return this;
}

//---------------------- The Setup menu mode -------------------------------------
void MSETUP::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	off_timeout		= pCFG->getOffTimeout();
	low_temp		= pCFG->getLowTemp();
	low_to			= pCFG->getLowTO();
	buzzer			= pCFG->isBuzzerEnabled();
	celsius			= pCFG->isCelsius();
	reed			= pCFG->isReedType();
	temp_step		= pCFG->isBigTempStep();
	i_clock_wise	= pCFG->isIronEncClockWise();
	g_clock_wise	= pCFG->isGunEncClockWise();
	auto_start		= pCFG->isAutoStart();
	lang_index		= pCore->nls.languageIndex();
	num_lang		= pCore->nls.numLanguages();
	dspl_bright		= pCFG->getDsplBrightness();
	dspl_rotation	= pCFG->getDsplRotation();
	set_param		= 0;
	uint8_t menu_len = pCore->dspl.menuSize(MSG_MENU_SETUP);
	pEnc->reset(mode_menu_item, 0, menu_len-1, 1, 1, true);
	update_screen = 0;
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_MENU_SETUP);						// "Parameters"
}



MODE* MSETUP::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t item 		= pEnc->read();
	uint8_t  button		= pEnc->buttonStatus();

	// Change the configuration parameters value in place
	if (mode_menu_item != item) {								// The encoder has been rotated
		mode_menu_item = item;
		switch (set_param) {									// Setup new value of the parameter in place
			case MM_AUTO_OFF:									// Setup auto off timeout
				if (item) {
					off_timeout	= item + 2;
				} else {
					off_timeout = 0;
				}
				break;
			case MM_STANDBY_TEMP:								// Setup low power (standby) temperature
				if (item >= min_standby_C) {
					low_temp = item;
				} else {
					low_temp = 0;
				}
				break;
			case MM_STANDBY_TIME:								// Setup low power (standby) timeout
				low_to	= item;
				break;
			case MM_BRIGHT:
				dspl_bright = constrain(item, 10, 255);
				pCore->dspl.BRGT::set(dspl_bright);
				break;
			case MM_ROTATION:
				dspl_rotation = constrain(item, 0, 3);
				pCore->dspl.rotate((tRotation)dspl_rotation);
				pCore->dspl.clear();
				pCore->dspl.drawTitle(MSG_MENU_SETUP);			// "Parameters"
				break;
			case MM_LANGUAGE:
				lang_index = item;
				break;
			default:
				break;
		}
		update_screen = 0;										// Force to redraw the screen
	}

	// Select setup menu Item
	if (!set_param) {											// Menu item (parameter) to modify was not selected yet
		if (button > 0) {										// The button was pressed, current menu item can be selected for modification
			switch (item) {										// item is a menu item
				case MM_UNITS:									// units C/F
					celsius	= !celsius;
					break;
				case MM_BUZZER:									// buzzer ON/OFF
					buzzer	= !buzzer;
					break;
				case MM_I_ENC:
					i_clock_wise = !i_clock_wise;
					break;
				case MM_G_ENC:
					g_clock_wise = !g_clock_wise;
					break;
				case MM_SWITCH_TYPE:							// REED/TILT
					reed = !reed;
					break;
				case MM_TEMP_STEP:								// Preset temperature step (1/5)
					temp_step  = !temp_step;
					break;
				case MM_AUTO_START:								// Automatic startup ON/OFF
					auto_start = !auto_start;
					break;
				case MM_AUTO_OFF:								// auto off timeout
					{
					set_param = item;
					uint8_t to = off_timeout;
					if (to > 2) to -=2;
					pEnc->reset(to, 0, 28, 1, 1, false);
					break;
					}
				case MM_STANDBY_TEMP:							// Standby temperature
					{
					set_param = item;
					uint16_t max_standby_C = pCFG->referenceTemp(0, d_t12);
					// When encoder value is less than min_standby_C, disable low power mode
					pEnc->reset(low_temp, min_standby_C-1, max_standby_C, 1, 5, false);
					break;
					}
				case MM_STANDBY_TIME:							// Standby timeout
					set_param = item;
					pEnc->reset(low_to, 1, 255, 1, 1, false);
					break;
				case MM_BRIGHT:
					set_param = item;
					pEnc->reset(dspl_bright, 10, 255, 1, 5, false);
					break;
				case MM_ROTATION:
					set_param = item;
					pEnc->reset(dspl_rotation, 0, 3, 1, 1, true);
					break;
				case MM_LANGUAGE:
					if (num_lang > 0) {
						set_param = item;
						pEnc->reset(lang_index, 0, num_lang-1, 1, 1, true);
					}
					break;
				case MM_SAVE:									// save
				{
					pD->clear();
					pD->errorMessage(MSG_SAVE_ERROR, 100);
					pCFG->umount();								// Prepare to load font data, clear status of the configuration file
					pD->dim(50);								// decrease the display brightness while saving configuration data
					// Perhaps, we should change the language: load messages and font
					if (lang_index != pCore->nls.languageIndex()) {
						pCore->nls.loadLanguageData(lang_index);
						// Check if the language successfully changed
						if (lang_index == pCore->nls.languageIndex()) {
							uint8_t *font = pCore->nls.font();
							pD->setLetterFont(font);
							std::string	l_name = pCore->nls.languageName(lang_index);
							pCFG->setLanguage(l_name.c_str());
						}
					}
					pCFG->setDsplRotation(dspl_rotation);
					pCFG->setup(off_timeout, buzzer, celsius, reed, temp_step, i_clock_wise, g_clock_wise,
									auto_start, low_temp, low_to, dspl_bright);
					pCFG->saveConfig();
					pCore->i_enc.setClockWise(i_clock_wise);
					pCore->g_enc.setClockWise(g_clock_wise);
					pCore->buzz.activate(buzzer);
					//pCore->dspl.rotate((tRotation)dspl_rotation);
					mode_menu_item = 0;
					return mode_return;
				}
				default:										// cancel
					pCFG->restoreConfig();
					mode_menu_item = 0;
					return mode_return;
			}
		}
	} else {													// Finish modifying  parameter, return to menu mode
		if (button == 1) {
			item 			= set_param;
			mode_menu_item 	= set_param;
			set_param = 0;
			uint8_t menu_len = pD->menuSize(MSG_MENU_SETUP);
			pEnc->reset(mode_menu_item, 0, menu_len-1, 1, 1, true);
		}
	}

	// Prepare to modify menu item in-place using built-in editor
	bool modify = false;
	if (set_param >= in_place_start && set_param <= in_place_end) {
		item = set_param;
		modify 	= true;
	}

	if (button > 0) {											// Either short or long press
		update_screen 	= 0;									// Force to redraw the screen
	}
	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 10000;

	// Build current menu item value
	const uint8_t value_length = 20;
	char item_value[value_length+1];
	item_value[1] = '\0';
	const char *msg_on 	= pD->msg(MSG_ON);
	const char *msg_off	= pD->msg(MSG_OFF);
	switch (item) {
		case MM_UNITS:											// units: C/F
			item_value[0] = 'F';
			if (celsius)
				item_value[0] = 'C';
			break;
		case MM_BUZZER:											// Buzzer setup
			sprintf(item_value, buzzer?msg_on:msg_off);
			break;
		case MM_SWITCH_TYPE:									// TILT/REED
			strncpy(item_value, pD->msg((reed)?MSG_REED:MSG_TILT), value_length);
			break;
		case MM_TEMP_STEP:										// Preset temperature step (1/5)
			sprintf(item_value, "%1d ", temp_step?5:1);
			strncpy(&item_value[2], pD->msg(MSG_DEG), value_length-2);
			break;
		case MM_AUTO_START:										// Auto start ON/OFF
			sprintf(item_value, auto_start?msg_on:msg_off);
			break;
		case MM_AUTO_OFF:										// auto off timeout
			if (off_timeout) {
				sprintf(item_value, "%2d ", off_timeout);
				strncpy(&item_value[3], pD->msg(MSG_MINUTES), value_length - 3);
			} else {
				strncpy(item_value, pD->msg(MSG_OFF), value_length);
			}
			break;
		case MM_STANDBY_TEMP:									// Standby temperature
			if (low_temp) {
				if (celsius) {
					sprintf(item_value, "%3d C", low_temp);
				} else {
					sprintf(item_value, "%3d F", celsiusToFahrenheit(low_temp));
				}
			} else {
				strncpy(item_value, pD->msg(MSG_OFF), value_length);
			}
			break;
		case MM_STANDBY_TIME:									// Standby timeout (5 secs intervals)
			if (low_temp) {
				uint16_t to = (uint16_t)low_to * 5;				// Timeout in seconds
				if (to < 60) {
					sprintf(item_value, "%2d ", to);
					strncpy(&item_value[3], pD->msg(MSG_SECONDS), value_length - 3);
				} else if (to % 60) {							// Minutes and seconds
					sprintf(item_value, "%2d ", to/60);
					uint8_t p = 3;
					strncpy(&item_value[p], pD->msg(MSG_MINUTES), value_length-p);
					p += strlen(pD->msg(MSG_MINUTES));
					if (p < value_length) {
						sprintf(&item_value[p], " %2d ", to%60);
						p += 4;
						if (p < value_length) {
							strncpy(&item_value[p], pD->msg(MSG_SECONDS), value_length-p);
						}
					}
				} else {
					sprintf(item_value, "%2d ", to/60);
					strncpy(&item_value[3], pD->msg(MSG_MINUTES), value_length-3);
				}
			} else {
				strncpy(item_value, pD->msg(MSG_OFF), value_length);
			}
			break;
		case MM_I_ENC:
			strncpy(item_value, pD->msg((i_clock_wise)?MSG_CW:MSG_CCW), value_length);
			break;
		case MM_G_ENC:
			strncpy(item_value, pD->msg((g_clock_wise)?MSG_CW:MSG_CCW), value_length);
			break;
		case MM_BRIGHT:
			sprintf(item_value, "%3d", dspl_bright);
			break;
		case MM_ROTATION:
			sprintf(item_value, "%3d", dspl_rotation*90);
			break;
		case MM_LANGUAGE:
		{
			std::string	l_name = pCore->nls.languageName(lang_index);
			strncpy(item_value, l_name.c_str(), value_length);
		}
			break;
		default:
			item_value[0] = '\0';
			break;
	}

	pD->menuShow(MSG_MENU_SETUP, item, item_value, modify);
	return this;
}

//---------------------- The Boost setup menu mode -------------------------------
void MMBST::init(void) {
	CFG*	pCFG	= &pCore->cfg;

	delta_temp	= pCFG->boostTemp();							// The boost temp is in the internal units
	duration	= pCFG->boostDuration();
	mode		= 0;
	uint8_t menu_len = pCore->dspl.menuSize(MSG_MENU_BOOST);
	//pCore->g_enc.reset(0, 0, 2, 1, 1, true);
	pCore->g_enc.reset(0, 0, menu_len-1, 1, 1, true);			// Select the boot menu item
	old_item	= 0;
	update_screen = 0;
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_MENU_BOOST);						// Boost setup
}

MODE* MMBST::loop(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t item 	= pEnc->read();
	uint8_t  button	= pEnc->buttonStatus();

	if (button == 1) {
		update_screen = 0;										// Force to redraw the screen
	} else if (button == 2) {									// The button was pressed for a long time
		// Save the boost parameters to the current configuration. Do not write it to the EEPROM!
		pCFG->saveBoost(delta_temp, duration);
		return mode_lpress;
	}

	if (old_item != item) {
		old_item = item;
		switch (mode) {
			case 1:												// New temperature increment
				delta_temp	= item;
				break;
			case 2:												// New duration period
				duration	= item;
				break;
		}
		update_screen = 0;										// Force to redraw the screen
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 30000;

	if (!mode) {												// The boost menu item selection mode
		if (button == 1) {										// The button was pressed
			switch (item) {
				case 0:											// delta temperature
					{
					mode 	= 1;
					pEnc->reset(delta_temp, 0, 75, 5, 5, false);
					break;
					}
				case 1:											// duration
					{
					mode 	= 2;
					pEnc->reset(duration, 20, 320, 20, 20, false);
					break;
					}
				case 2:											// save
				default:
					// Save the boost parameters to the current configuration. Do not write it to the EEPROM!
					pCFG->saveBoost(delta_temp, duration);
					pCore->dspl.clear();
					pCore->dspl.errorMessage(MSG_SAVE_ERROR, 100);
					pCore->dspl.dim(50);						// decrease the display brightness while saving configuration data
					pCFG->saveConfig();
					return mode_return;
			}
		}
	} else {													// Return to the item selection mode
		if (button == 1) {
			pEnc->reset(mode-1, 0, 2, 1, 1, true);
			mode = 0;
			return this;
		}
	}

	// Show current menu item
	const uint8_t value_length = 20;
	char item_value[value_length+1];
	item_value[1] = '\0';
	if (mode) {
		item = mode - 1;
	}
	switch (item) {
		case 0:													// delta temperature
			if (delta_temp) {
				uint16_t delta_t = delta_temp;
				char sym = 'C';
				if (!pCFG->isCelsius()) {
					delta_t = (delta_t * 9 + 3) / 5;
					sym = 'F';
				}
				sprintf(item_value, "+%2d %c", delta_t, sym);
			} else {
				strncpy(item_value, pCore->dspl.msg(MSG_OFF), value_length);
			}
			break;
		case 1:													// duration (secs)
		    sprintf(item_value, "%3d ", duration);
		    strncpy(&item_value[4], pCore->dspl.msg(MSG_SECONDS), value_length-4);
			break;
		default:
			item_value[0] = '\0';
			break;
	}

	pCore->dspl.menuShow(MSG_MENU_BOOST, item, item_value, mode);
	return this;
}

//---------------------- Calibrate tip menu --------------------------------------
MCALMENU::MCALMENU(HW* pCore, MODE* cal_auto, MODE* cal_manual) : MODE(pCore) {
	mode_calibrate_tip = cal_auto; mode_calibrate_tip_manual = cal_manual;
}

void MCALMENU::init(void) {
	uint8_t menu_len = pCore->dspl.menuSize(MSG_MENU_CALIB);
	pCore->g_enc.reset(0, 0, menu_len-1, 1, 1, true);
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_MENU_CALIB);						// "Calibrate"
	old_item		= 4;
	update_screen	= 0;
}

MODE* MCALMENU::loop(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t item 	= pEnc->read();
	uint8_t button	= pEnc->buttonStatus();

	if (button == 1) {
		update_screen = 0;										// Force to redraw the screen
	} else if (button == 2) {									// The button was pressed for a long time
	   	return mode_lpress;
	}

	if (old_item != item) {
		old_item = item;
		update_screen = 0;										// Force to redraw the screen
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 30000;

	if (button == 1) {											// The button was pressed
		switch (item) {
			case 0:												// Calibrate tip automatically
				mode_calibrate_tip->useDevice(d_t12);
				return mode_calibrate_tip;
			case 1:												// Calibrate tip manually
				mode_calibrate_tip_manual->useDevice(d_t12);
				return mode_calibrate_tip_manual;
			case 2:												// Initialize tip calibration data
				pCFG->resetTipCalibration(d_t12);
				pCore->buzz.shortBeep();
				pEnc->write(0);
				return this;
			default:											// exit
				return mode_return;
		}
	}

	pCore->dspl.menuShow(MSG_MENU_CALIB, item, 0, false);
	return this;
}

//---------------------- Hot Air Gun setup menu ----------------------------------
MENU_GUN::MENU_GUN(HW* pCore, MODE* calib, MODE* pot_tune, MODE* pid_tune) : MODE(pCore) {
	mode_calibrate	= calib;
	mode_tune		= pot_tune;
	mode_pid		= pid_tune;
}

void MENU_GUN::init(void) {
	uint8_t menu_len = pCore->dspl.menuSize(MSG_MENU_GUN);
	//pCore->g_enc.reset(0, 0, 4, 1, 1, true);
	pCore->g_enc.reset(0, 0, menu_len-1, 1, 1, true);
	old_item		= 5;
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_MENU_GUN);						// "Hot Air Gun"
	update_screen	= 0;
}

MODE* MENU_GUN::loop(void) {
	CFG*	pCFG	= &pCore->cfg;
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t item 	= pEnc->read();
	uint8_t button	= pEnc->buttonStatus();

	if (button == 1) {
		update_screen = 0;										// Force to redraw the screen
	} else if (button == 2) {									// The button was pressed for a long time
	   	return mode_lpress;
	}

	if (old_item != item) {
		old_item = item;
		update_screen = 0;										// Force to redraw the screen
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 30000;

	if (button == 1) {											// The button was pressed
		switch (item) {
			case 0:												// Calibrate Hot Air Gun
				if (mode_calibrate) {
					mode_calibrate->useDevice(d_gun);
					return mode_calibrate;
				}
				break;
			case 1:												// Tune Hot air Gun potentiometer
				if (mode_tune) {
					mode_tune->useDevice(d_gun);
					return mode_tune;
				}
				break;
			case 2:												// Tune Hot Air Gun PID parameters
				if (mode_pid) {
					mode_pid->useDevice(d_gun);					// Tune Hot Air Gun
					return mode_pid;
				}
				break;
			case 3:												// Initialize Hot Air Gun calibration data
				pCFG->resetTipCalibration(d_gun);
				return mode_return;
			default:											// exit
				return mode_return;
		}
	}

	pCore->dspl.menuShow(MSG_MENU_GUN, item, 0, false);
	return this;
}

//---------------------- The automatic calibration tip mode ----------------------
// Used to automatically calibrate iron tip
// There are 4 temperature calibration points of the tip in the controller,
// but during calibration procedure we will use more points to cover whole set
// of the internal temperature values.
void MCALIB::init(void) {
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;

	// Prepare to enter real temperature
	uint16_t min_t 		= 50;
	uint16_t max_t		= 600;
	if (!pCFG->isCelsius()) {
		min_t 	=  122;
		max_t 	= 1111;
	}
	PIDparam pp = pCFG->pidParamsSmooth(d_t12);					// Load PID parameters to stabilize the temperature of unknown tip
	pIron->PID::load(pp);
	pCore->g_enc.reset(0, min_t, max_t, 1, 1, false);
	for (uint8_t i = 0; i < MCALIB_POINTS; ++i) {
		calib_temp[0][i] = 0;									// Real temperature. 0 means not entered yet
		calib_temp[1][i] = map(i, 0, MCALIB_POINTS-1, start_int_temp, int_temp_max / 2); // Internal temperature
	}
	ref_temp_index 	= 0;
	tuning			= false;
	old_encoder 	= 3;
	phase			= MC_OFF;
	ready_to		= 0;
	phase_change	= 0;
	update_screen 	= 0;
	tip_temp_max 	= int_temp_max / 2;							// The maximum possible temperature defined in iron.h
	const char *calibrate = pCore->dspl.msg(MSG_MENU_CALIB);	// "Calibrate <tip name>"
	uint8_t len = strlen(calibrate);
	if (len > 19) len = 19;										// Limit maximum string length
	std::string title(calibrate, len);
	title = title + " " + pCFG->tipName(d_t12);
	pCore->dspl.clear();
	pCore->dspl.drawTitleString(title.c_str());
}

/*
 * Calculate tip calibration parameter using linear approximation by Ordinary Least Squares method
 * Y = a * X + b, where
 * Y - internal temperature, X - real temperature. a and b are double coefficients
 * a = (N * sum(Xi*Yi) - sum(Xi) * sum(Yi)) / ( N * sum(Xi^2) - (sum(Xi))^2)
 * b = 1/N * (sum(Yi) - a * sum(Xi))
 */
bool MCALIB::calibrationOLS(uint16_t* tip, uint16_t min_temp, uint16_t max_temp) {
	long sum_XY = 0;											// sum(Xi * Yi)
	long sum_X 	= 0;											// sum(Xi)
	long sum_Y  = 0;											// sum(Yi)
	long sum_X2 = 0;											// sum(Xi^2)
	long N		= 0;

	for (uint8_t i = 0; i < MCALIB_POINTS; ++i) {
		uint16_t X 	= calib_temp[0][i];
		uint16_t Y	= calib_temp[1][i];
		if (X >= min_temp && X <= max_temp) {
			sum_XY 	+= X * Y;
			sum_X	+= X;
			sum_Y   += Y;
			sum_X2  += X * X;
			++N;
		}
	}

	if (N <= 2)													// Not enough real temperatures have been entered
		return false;

	double	a  = (double)N * (double)sum_XY - (double)sum_X * (double)sum_Y;
			a /= (double)N * (double)sum_X2 - (double)sum_X * (double)sum_X;
	double 	b  = (double)sum_Y - a * (double)sum_X;
			b /= (double)N;

	for (uint8_t i = 0; i < 4; ++i) {
		double temp = a * (double)pCore->cfg.referenceTemp(i, d_t12) + b;
		tip[i] = round(temp);
	}
	if (tip[3] > int_temp_max) tip[3] = int_temp_max;			// Maximal possible temperature (main.h)
	return true;
}

// Find the index of the reference point with the closest temperature
uint8_t MCALIB::closestIndex(uint16_t temp) {
	uint16_t diff = 1000;
	uint8_t index = MCALIB_POINTS;
	for (uint8_t i = 0; i < MCALIB_POINTS; ++i) {
		uint16_t X = calib_temp[0][i];
		if (X > 0 && abs(X-temp) < diff) {
			diff = abs(X-temp);
			index = i;
		}
	}
	return index;
}

void MCALIB::updateReference(uint8_t indx) {					// Update reference points
	CFG*	pCFG	= &pCore->cfg;
	uint16_t expected_temp 	= map(indx, 0, MCALIB_POINTS, pCFG->tempMinC(d_t12), pCFG->tempMaxC(d_t12));
	uint16_t r_temp			= calib_temp[0][indx];
	if (indx < 5 && r_temp > (expected_temp + expected_temp/4)) {	// The real temperature is too high
		tip_temp_max -= tip_temp_max >> 2;						// tip_temp_max *= 0.75;
		if (tip_temp_max < int_temp_max / 4)
			tip_temp_max = int_temp_max / 4;					// Limit minimum possible value of the highest temperature

	} else if (r_temp > (expected_temp + expected_temp/8)) { 	// The real temperature is biger than expected
		tip_temp_max += tip_temp_max >> 3;						// tip_temp_max *= 1.125;
		if (tip_temp_max > int_temp_max)
			tip_temp_max = int_temp_max;
	} else if (indx < 5 && r_temp < (expected_temp - expected_temp/4)) { // The real temperature is too low
		tip_temp_max += tip_temp_max >> 2;						// tip_temp_max *= 1.25;
		if (tip_temp_max > int_temp_max)
			tip_temp_max = int_temp_max;
	} else if (r_temp < (expected_temp - expected_temp/8)) { 	// The real temperature is lower than expected
		tip_temp_max += tip_temp_max >> 3;						// tip_temp_max *= 1.125;
		if (tip_temp_max > int_temp_max)
			tip_temp_max = int_temp_max;
	} else {
		return;
	}

	// rebuild the array of the reference temperatures
	for (uint8_t i = indx+1; i < MCALIB_POINTS; ++i) {
		calib_temp[1][i] = map(i, 0, MCALIB_POINTS-1, start_int_temp, tip_temp_max);
	}
}


void MCALIB::buildFinishCalibration(void) {
	CFG* 	pCFG 	= &pCore->cfg;
	uint16_t tip[4];
	if (calibrationOLS(tip, 150, pCFG->referenceTemp(2, d_t12))) {
		uint8_t near_index	= closestIndex(pCFG->referenceTemp(3, d_t12));
		tip[3] = map(pCFG->referenceTemp(3, d_t12), pCFG->referenceTemp(2, d_t12), calib_temp[0][near_index],
				tip[2], calib_temp[1][near_index]);
		if (tip[3] > int_temp_max) tip[3] = int_temp_max;		// Maximal possible temperature (main.h)

		uint8_t tip_index 	= pCFG->currentTipIndex(d_t12);
		int16_t ambient 	= pCore->ambientTemp();
		pCFG->applyTipCalibtarion(tip, ambient, d_t12);
		pCFG->saveTipCalibtarion(tip_index, tip, TIP_ACTIVE | TIP_CALIBRATED, ambient);
	}
}
/*
 * The calibration procedure consists of 8 steps (number of temperature reference points)
 * Each step is to check the real temperature at given reference point/ The step begins when the rotary encoder pressed
 * see 'Next step begins here'. As soon as PID algorithm used, there is some bouncing process before reference reached, damped oscillations.
 * Reaching required reference temperature process is split into several phases:
 * MC_GET_READY (if the temperature is higher) -> MC_HEATING (over temperature) -> MC_COOLING (under temperature) -> MC_HEATING_AGAIN -> MC_READY
 * The oscillation process limited by some timeout, ready_to
 */
MODE* MCALIB::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	IRON*	pIron	= &pCore->iron;
	RENC*	pEnc	= &pCore->g_enc;

	uint16_t encoder	= pEnc->read();
    uint8_t  button		= pEnc->buttonStatus();

    if (encoder != old_encoder) {
    	old_encoder = encoder;
    	update_screen = 0;
    }

	if (button == 1) {											// The button pressed
		if (tuning) {											// New reference temperature was entered
			pIron->switchPower(false);
		    if (phase == MC_READY) {							// The temperature was stabilized and real data can be entered
		    	phase = MC_OFF;
			    uint16_t temp	= pIron->averageTemp();			// The temperature of the IRON in internal units
			    uint16_t r_temp = encoder;						// The real temperature entered by the user
			    if (!pCFG->isCelsius())							// Always save the human readable temperature in Celsius
			    	r_temp = fahrenheitToCelsius(r_temp);
			    uint8_t ref = ref_temp_index;
			    calib_temp[0][ref] = r_temp;
			    calib_temp[1][ref] = temp;
			    if (r_temp < pCFG->tempMaxC(d_t12) - 20) {
			    	updateReference(ref_temp_index);			// Update reference points
			    	++ref_temp_index;
			    	// Try to update the current tip calibration
			    	uint16_t tip[4];
			    	 if (calibrationOLS(tip, 150, 600))
			    		 pCFG->applyTipCalibtarion(tip, pCore->ambientTemp(), d_t12);
			    } else {										// Finish calibration
			    	ref_temp_index = MCALIB_POINTS;
			    }
		    	ready_to		= 0;
		    	phase_change	= 0;
		    } else {											// Stop heating, return from tuning mode
		    	tuning = false;
		    	update_screen = 0;
		    	return this;
		    }
		    tuning = false;
		}
		if (!tuning) {											// Next step begins here. Heating to the next reference temperature
			if (ref_temp_index < MCALIB_POINTS) {
				tuning = true;
				uint16_t temp_set	= calib_temp[1][ref_temp_index];
				uint16_t temp 		= pIron->averageTemp();
				phase = (temp_set < temp)?MC_GET_READY:MC_HEATING;	// Start heating the Iron
				pIron->setTemp(temp_set);
				pIron->switchPower(true);
				ready_to 		= HAL_GetTick() + 120000;		// We should be ready in 2 minutes
				phase_change	= HAL_GetTick() + phase_change_time;
			} else {											// All reference points are entered
				buildFinishCalibration();
				PIDparam pp = pCFG->pidParams(dev_type);		// Restore default PID parameters
				pIron->PID::load(pp);
				return mode_lpress;
			}
		}
		update_screen = 0;
	} else if (!tuning && button == 2) {						// The button was pressed for a long time, save tip calibration
		buildFinishCalibration();
		PIDparam pp = pCFG->pidParams(dev_type);				// Restore default PID parameters
		pIron->PID::load(pp);
	    return mode_lpress;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 500;

	int16_t	 ambient	= pCore->ambientTemp();
	uint16_t real_temp 	= encoder;
	uint16_t temp_set	= pIron->presetTemp();
	uint16_t temp 		= pIron->averageTemp();
	uint8_t  power		= pIron->avgPowerPcnt();
	uint16_t tempH 		= pCFG->tempToHuman(temp, ambient, d_t12);

	if (temp >= int_temp_max) {									// Prevent soldering IRON overheat, save current calibration
		buildFinishCalibration();
		PIDparam pp = pCFG->pidParams(dev_type);				// Restore default PID parameters
		pIron->PID::load(pp);
		return mode_lpress;
	}

	if (phase_change > 0 && HAL_GetTick() >= phase_change) {
		if (tuning && (abs(temp_set - temp) <= 16) && (pIron->pwrDispersion() <= 200) && power > 1)  {
			switch (phase) {
				case MC_HEATING:								// First wave heating, over the preset temperature, start cooling
					phase = MC_COOLING;
					phase_change	= HAL_GetTick() + phase_change_time;
					break;
				case MC_HEATING_AGAIN:							// Second wave of heating, ready to enter the real temperature
					pCore->buzz.shortBeep();
					pEnc->write(tempH);
					phase = MC_READY;
					phase_change	= HAL_GetTick() + phase_change_time;
					break;
				case MC_READY:
				case MC_OFF:
					break;
				default:
					break;
			}
		}
		if (phase == MC_COOLING && (temp_set > temp + 8)) {		// Cooled lower than the preset temperature, start heating again
			phase = MC_HEATING_AGAIN;
			phase_change	= HAL_GetTick() + phase_change_time;
		}
		if (phase == MC_GET_READY && (temp_set > temp + 8)) {
			phase = MC_HEATING;
			phase_change	= HAL_GetTick() + phase_change_time;
		}
	}
	// Check the timeout
	if (ready_to > 0 && phase != MC_OFF && phase != MC_READY && HAL_GetTick() > ready_to)
		phase = MC_READY;

	uint8_t	int_temp_pcnt = 0;
	if (temp >= start_int_temp)
		int_temp_pcnt = map(temp, start_int_temp, int_temp_max, 0, 100);	// int_temp_max defined in vars.cpp
	pD->calibShow(ref_temp_index+1, tempH, real_temp, pCFG->isCelsius(), power, tuning, phase == MC_READY, int_temp_pcnt);
	return this;
}

//---------------------- The manual calibration tip mode -------------------------
// Here the operator should 'guess' the internal temperature readings for desired temperature.
// Rotate the encoder to change temperature preset in the internal units
// and controller would keep that temperature.
// This method is more accurate one, but it requires more time.
void MCALIB_MANUAL::init(void) {
	CFG*	pCFG		= &pCore->cfg;
	PIDparam pp 		= pCFG->pidParamsSmooth(dev_type);
	if (dev_type == d_t12) {
		pCore->iron.PID::load(pp);
	} else {
		pCore->hotgun.PID::load(pp);
		pCore->hotgun.setFan(fan_speed);
	}
	pCore->g_enc.reset(0, 0, 3, 1, 1, true);					// Select reference temperature point using Encoder
	pCFG->getTipCalibtarion(calib_temp, dev_type);				// Load current calibration data
	ref_temp_index 		= 0;
	ready				= false;
	tuning				= false;
	temp_setready_ms	= 0;
	old_encoder			= 4;
	update_screen		= 0;
	const char *calibrate = pCore->dspl.msg(MSG_MENU_CALIB);	// "Calibrate <tip name>"
	uint8_t len = strlen(calibrate);
	if (len > 19) len = 19;										// Limit maximum string length
	std::string title(calibrate, len);
	title += " ";
	if (dev_type == d_t12)
		title += pCFG->tipName(d_t12);
	else
		title += pCore->dspl.msg(MSG_HOT_AIR_GUN);
	pCore->dspl.clear();
	pCore->dspl.drawTitleString(title.c_str());
}

// Make sure the tip[0] < tip[1] < tip[2] < tip[3];
// And the difference between next points is greater than req_diff
// Change neighborhood temperature data to keep this difference
void MCALIB_MANUAL::buildCalibration(int8_t ablient, uint16_t tip[], uint8_t ref_point) {
	if (tip[3] > int_temp_max) tip[3] = int_temp_max;			// int_temp_max is a maximum possible temperature (vars.cpp)

	const int req_diff = 200;
	if (ref_point <= 3) {										// tip[0-3] - internal temperature readings for the tip at reference points (200-400)
		for (uint8_t i = ref_point; i <= 2; ++i) {				// ref_point is 0 for 200 degrees and 3 for 400 degrees
			int diff = (int)tip[i+1] - (int)tip[i];
			if (diff < req_diff) {
				tip[i+1] = tip[i] + req_diff;					// Increase right neighborhood temperature to keep the difference
			}
		}
		if (tip[3] > int_temp_max)								// The high temperature limit is exceeded, temp_max. Lower all calibration
			tip[3] = int_temp_max;

		for (int8_t i = 3; i > 0; --i) {
			int diff = (int)tip[i] - (int)tip[i-1];
			if (diff < req_diff) {
				int t = (int)tip[i] - req_diff;					// Decrease left neighborhood temperature to keep the difference
				if (t < 0) t = 0;
				tip[i-1] = t;
			}
		}
	}
}

void MCALIB_MANUAL::restorePIDconfig(CFG *pCFG, UNIT* pUnit) {
	PIDparam pp = pCFG->pidParams(dev_type);
	pUnit->PID::load(pp);
}

MODE* MCALIB_MANUAL::loop(void) {
	CFG*	pCFG	= &pCore->cfg;
	UNIT*	pUnit	= unit();
	RENC*	pEnc	= &pCore->g_enc;

	uint16_t encoder	= pEnc->read();
    uint8_t  button		= pEnc->buttonStatus();

    if (encoder != old_encoder) {
    	old_encoder = encoder;
    	if (tuning) {											// Preset temperature (internal units)
    		pUnit->setTemp(encoder);
    		ready = false;
    		temp_setready_ms = HAL_GetTick() + 5000;    		// Prevent beep just right the new temperature setup
    	}
    	update_screen = 0;
    }

	int16_t ambient = pCore->ambientTemp();
	if (dev_type == d_t12) {
    	if (!pUnit->isConnected()) {
    		restorePIDconfig(pCFG, pUnit);
    		return 0;
    	}
	} else if (temp_setready_ms && (HAL_GetTick() > temp_setready_ms) && !pUnit->isConnected()) {
		restorePIDconfig(pCFG, pUnit);
		return 0;
	}

	if (button == 1) {											// The button pressed
		if (tuning) {											// New reference temperature was confirmed
			pUnit->switchPower(false);
		    if (ready) {										// The temperature has been stabilized
		    	ready = false;
		    	uint16_t temp	= pUnit->averageTemp();			// The temperature of the IRON of Hot Air Gun in internal units
			    uint8_t ref 	= ref_temp_index;
			    calib_temp[ref] = temp;
			    uint16_t tip[4];
			    for (uint8_t i = 0; i < 4; ++i) {
			    	tip[i] = calib_temp[i];
			    }
			    buildCalibration(ambient, tip, ref);			// ref is 0 for 200 degrees and 3 for 400 degrees
			    pCFG->applyTipCalibtarion(tip, ambient, dev_type);
		    }
		    tuning	= false;
			encoder = ref_temp_index;
		    pEnc->reset(encoder, 0, 3, 1, 1, true);				// Turn back to the reference temperature point selection mode
		} else {												// Reference temperature index was selected from the list
			ref_temp_index 	= encoder;
			tuning 			= true;
			uint16_t tempH 	= pCFG->referenceTemp(encoder, dev_type); // Read the preset temperature from encoder
			uint16_t temp 	= pCFG->humanToTemp(tempH, ambient, dev_type);
			pEnc->reset(temp, 100, int_temp_max, 1, 5, false); // int_temp_max declared in vars.cpp
			pUnit->setTemp(temp);
			pUnit->switchPower(true);
		}
		update_screen = 0;
	} else if (button == 2) {									// The button was pressed for a long time, save tip calibration
		uint8_t tip_index = pCFG->currentTipIndex(dev_type);	// IRON actual tip index or Hot Air Gun (tip index = 0)
		buildCalibration(ambient, calib_temp, 10); 				// 10 is bigger then the last index in the reference temp. Means build final calibration
		pCFG->applyTipCalibtarion(calib_temp, ambient, dev_type);
		pCFG->saveTipCalibtarion(tip_index, calib_temp, TIP_ACTIVE | TIP_CALIBRATED, ambient);
		restorePIDconfig(pCFG, pUnit);
	    return mode_lpress;
	}

	uint8_t rt_index = encoder;									// rt_index is a reference temperature point index. Read it fron encoder
	if (tuning) {
		rt_index	= ref_temp_index;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 500;

	uint16_t temp_set		= pUnit->presetTemp();;				// Prepare the parameters to be displayed
	uint16_t temp			= pUnit->averageTemp();
	uint8_t  power			= pUnit->avgPowerPcnt();
	uint16_t pwr_disp		= pUnit->pwrDispersion();
	uint16_t pwr_disp_max	= (dev_type == d_t12)?200:40;
	if (tuning && (abs(temp_set - temp) <= 16) && (pwr_disp <= pwr_disp_max) && power > 1)  {
		if (!ready && temp_setready_ms && (HAL_GetTick() > temp_setready_ms)) {
			pCore->buzz.shortBeep();
			ready 				= true;
			temp_setready_ms	= 0;
	    }
	}

	uint16_t temp_setup = temp_set;
	if (!tuning) {
		uint16_t tempH 	= pCFG->referenceTemp(encoder, dev_type);
		temp_setup 		= pCFG->humanToTemp(tempH, ambient, dev_type);
	}

	pCore->dspl.calibManualShow(pCFG->referenceTemp(rt_index, dev_type), temp, temp_setup, pCFG->isCelsius(), power, tuning, ready);
	return	this;
}

//---------------------- The tune mode -------------------------------------------
void MTUNE::init(void) {
	RENC*	pEnc	= &pCore->g_enc;
	uint16_t max_power = 0;
	if (dev_type == d_t12) {
		max_power = pCore->iron.getMaxFixedPower();
		check_delay	= 0;										// IRON connection can be checked any time
	} else {
		HOTGUN*	pHG	= &pCore->hotgun;
		max_power 	= pHG->getMaxFixedPower();
		pHG->setFan(1500);										// Make sure the fan will be blowing well.
		check_delay		= HAL_GetTick() + 2000;					// Wait 2 seconds before checking Hot Air Gun
	}
	pEnc->reset(max_power/3, 10, max_power, 1, 5, false);
	old_power		= 0;
	powered			= true;
	check_connected	= false;
	pCore->dspl.clear();
	pCore->dspl.drawTitle((dev_type == d_t12)?MSG_TUNE_IRON:MSG_TUNE_GUN);
}

MODE* MTUNE::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	UNIT*	pUnit	= unit();
	RENC*	pEnc	= &pCore->g_enc;

    uint16_t power 	= pEnc->read();
    uint8_t  button	= pEnc->buttonStatus();

    if (!check_connected) {
    	check_connected = HAL_GetTick() >= check_delay;
    } else {
    	if (!pUnit->isConnected()) {
    		if (dev_type == d_t12) {
    			return 0;
    		} else {
    			if (pCore->hotgun.isFanWorking())
    				return 0;
    		}
    	}
    }

	if (button == 1) {											// The button pressed
		powered = !powered;
	} else if (button == 2) {									// The button was pressed for a long time, exit tune mode
		pCore->buzz.shortBeep();
		return mode_lpress;
	}
	if (!powered) power = 0;

	if (power != old_power) {
   		pUnit->fixPower(power);
    	old_power = power;
    	update_screen = 0;
    }

	if (HAL_GetTick() < update_screen) return this;
    update_screen = HAL_GetTick() + 500;

    uint16_t tune_temp = (dev_type == d_t12)?iron_temp_maxC:gun_temp_maxC;	// vars.cpp
    uint16_t temp		= pUnit->averageTemp();
    uint8_t	 p_pcnt		= pUnit->avgPowerPcnt();
    pD->tuneShow(tune_temp, temp, p_pcnt);
    return this;
}

//---------------------- The PID coefficients tune mode --------------------------
void MTPID::init(void) {
	DSPL*	pD		= &pCore->dspl;
	RENC*	pEnc	= &pCore->g_enc;

	allocated 			= pD->pidStart();
	pEnc->reset(0, 0, 2, 1, 1, true);							// Select the coefficient to be modified
	data_update 		= 0;
	data_index 			= 0;
	modify				= false;
	on					= false;
	old_index			= 3;
	update_screen 		= 0;
	reset_dspl			= true;
}

MODE* MTPID::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	CFG*	pCFG	= &pCore->cfg;
	UNIT*	pUnit	= unit();
	RENC*	pEnc	= &pCore->g_enc;

	if (!allocated) {
		pCore->buzz.failedBeep();
		return mode_lpress;
	}

	uint16_t index 	= pEnc->read();
	uint8_t  button	= pEnc->buttonStatus();

    if (!pUnit->isConnected()) {
    	if (dev_type == d_t12) {
    		return 0;
    	} else {
    		if (pCore->hotgun.isFanWorking())
    			return 0;
    	}
    }

	if (button || old_index != index)
		update_screen = 0;

	if (HAL_GetTick() >= data_update) {
		data_update = HAL_GetTick() + 100;
		int16_t  temp = pUnit->averageTemp() - pUnit->presetTemp();;
		uint32_t disp = pUnit->pwrDispersion();
		pD->GRAPH::put(temp, disp);
	}

	if (HAL_GetTick() < update_screen) return this;

	PID* pPID	= pUnit;
	if (modify) {											// The Coefficient is selected, start to show the Graphs
		update_screen = HAL_GetTick() + 100;
		if (button == 1) {									// Short button press: select another PID coefficient
			modify = false;
			pEnc->reset(data_index, 0, 2, 1, 1, true);
			reset_dspl = true;
			return this;									// Restart the procedure
		} else if (button == 2) {							// Long button press: toggle the power
			on = !on;
			pUnit->switchPower(on);
			if (on) pD->GRAPH::reset();							// Reset display graph history
			pCore->buzz.shortBeep();
		}
		if (reset_dspl) {									// Flag indicating we should completely redraw display
			reset_dspl = false;
			pD->clear();
			pD->pidAxis("manual PID", "T", "D(P)");
		}
		if (old_index != index) {
			old_index = index;
			pPID->changePID(data_index+1, index);
			pD->pidModify(data_index, index);
			update_screen = HAL_GetTick() + 1000;			// Show new parameter value for 1 second
			return this;
		}
		pD->pidShowGraph();
	} else {												// Selecting the PID coefficient to be tuned
		update_screen = HAL_GetTick() + 1000;

		if (old_index != index) {
			old_index	= index;
			data_index  = index;
		}

		if (button == 1) {									// Short button press: select another PID coefficient
			modify = true;
			data_index  = index;
			// Prepare to change the coefficient [index]
			uint16_t k = 0;
			k = pPID->changePID(index+1, -1);				// Read the PID coefficient from the IRON or Hot Air Gun
			pEnc->reset(k, 0, 20000, 1, 10, false);
			reset_dspl	= true;
			return this;									// Restart the procedure
		} else if (button == 2) {							// Long button press: save the parameters and return to menu
			if (confirm()) {
				PIDparam pp = pPID->dump();
				pCFG->savePID(pp, dev_type);
				pCore->buzz.shortBeep();
			} else {
				pCore->buzz.failedBeep();
			}
			pD->pidDestroyData();
			return mode_lpress;
		}

		if (reset_dspl) {									// Flag indicating we should completely redraw display
			reset_dspl = false;
			pD->clear();
		}
		uint16_t pid_k[3];
		for (uint8_t i = 0; i < 3; ++i) {
			pid_k[i] = 	pPID->changePID(i+1, -1);
		}
		pD->pidShowMenu(pid_k, data_index);
	}
	return this;
}

bool MTPID::confirm(void) {
	pCore->g_enc.reset(0, 0, 1, 1, 1, true);
	pCore->dspl.clear();
	pCore->buzz.shortBeep();
	PID* pPID	= unit();
	uint16_t pid_k[3];
	for (uint8_t i = 0; i < 3; ++i) {
		pid_k[i] = 	pPID->changePID(i+1, -1);
	}
	pCore->dspl.pidShowMenu(pid_k, 3);

	while (true) {
		if (pCore->dspl.adjust())							// Adjust display brightness
			HAL_Delay(5);
		uint8_t answer = pCore->g_enc.read();
		if (pCore->g_enc.buttonStatus() > 0)
			return answer == 0;
		pCore->dspl.showDialog(MSG_SAVE_Q, 150, answer == 0);
	}
	return false;
}

//---------------------- The PID coefficients automatic tune mode ----------------
void MAUTOPID::init(void) {
	DSPL*	pD		= &pCore->dspl;

	PIDparam pp = pCore->cfg.pidParamsSmooth(dev_type);		// Load PID parameters to stabilize the temperature of unknown tip
	if (dev_type == d_t12)
		pCore->iron.PID::load(pp);
	else
		pCore->hotgun.PID::load(pp);
	pD->pidStart();
	if (dev_type == d_t12) {
		td_limit	= 6;
		pwr_ch_to	= 5000;									// Power change timeout (ms)
	} else {
		td_limit	= 500;
		pwr_ch_to	= 20000;
		if (!pCore->hotgun.isConnected()) {					// Check the Hot Air connected
			pCore->hotgun.fanControl(true);
			HAL_Delay(1000);
			pCore->hotgun.fanControl(false);
		}
	}
	data_update 	= 0;
	data_period		= 250;
	phase_to		= 0;
	mode			= TUNE_OFF;
	pD->clear();
	pD->pidAxis("Auto PID", "T", "p");
	update_screen 	= 0;
}

MODE* MAUTOPID::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	UNIT*	pUnit	= unit();
	RENC*	pEnc	= &pCore->g_enc;

	uint8_t  button		= pEnc->buttonStatus();

    if (!pUnit->isConnected()) {
    	if (dev_type == d_t12) {
    		return 0;
    	} else {
    		if (pCore->hotgun.isFanWorking())
    			return 0;
    	}
    }

    if(button)
		update_screen = 0;

	if (HAL_GetTick() >= data_update) {
	    int16_t temp	= pUnit->averageTemp() - base_temp;
	    uint8_t p		= pUnit->avgPowerPcnt();
		data_update 	= HAL_GetTick() + data_period;
		pD->GRAPH::put(temp, p);
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 500;

	if (button == 1) {											// Short button press: switch on/off the power
		data_period	= 250;
		if (mode == TUNE_OFF) {
			mode = TUNE_HEATING;
			base_temp 		= pUnit->presetTemp();
			pD->GRAPH::reset();									// Reset display graph history
			pUnit->switchPower(true);							// First, heat the IRON or Hot Air Gun to the preset temperature
			pD->pidShowMsg("Heating");
			uint32_t n 		= HAL_GetTick();
			update_screen 	= n + msg_to;
			phase_to		= n + 300000;						// 5 minutes to heat up
			next_mode 		= n + 120000;						// Wait the for temperature stabilized
			return this;
		} else {												// Running mode
			pUnit->switchPower(false);
			if ((mode == TUNE_RELAY) && (tune_loops > 8) && updatePID(pUnit)) {
				if (mode_spress) {
					pD->pidDestroyData();
					mode_spress->useDevice(dev_type);
					return mode_spress;
				}
			}
			mode = TUNE_OFF;
			pD->pidShowMsg("Stop");
			update_screen = HAL_GetTick() + msg_to;
			return this;
		}
	} else if (button == 2 && mode_lpress) {					// Long button press
		pD->pidDestroyData();
		PIDparam pp = pCore->cfg.pidParams(dev_type);			// Restore standard PID parameters
		pUnit->PID::load(pp);
		mode_lpress->useDevice(dev_type);
		return mode_lpress;
	}

	if (mode_return && pCore->i_enc.buttonStatus() > 0) {		// The IRON encoder button pressed
		pD->pidDestroyData();
		return mode_return;
	}

	int16_t  temp		= pUnit->averageTemp();
	uint32_t td			= pUnit->tmpDispersion();
	uint32_t pd			= pUnit->pwrDispersion();
	int32_t  ap			= pUnit->avgPower();

	if (next_mode <= HAL_GetTick()) {
		switch (mode) {
			case TUNE_HEATING:									// Heating to the preset temperature
				if ((temp > base_temp) && (temp < base_temp + 7) && (td <= td_limit) && (pd <= 4) && (ap > 0)) {
					base_pwr = ap + (ap+10)/20;					// Add 5%
					pUnit->fixPower(base_pwr);					// Apply base power
					pD->pidShowMsg("Base power");
					pCore->buzz.shortBeep();
					uint32_t n = HAL_GetTick();
					update_screen = n + msg_to;
					next_mode = n + pwr_ch_to;					// Wait before go to supply fixed power
					phase_to  = n + 180000;
					mode = TUNE_BASE;
					old_temp = 0;
					pwr_change	= FIX_PWR_NONE;
					return this;
				}
				break;
			case TUNE_BASE:										// Applying base power
			{
				bool power_changed = false;
				if (old_temp == 0) {							// Setup starting temperature
					old_temp = temp;
					next_mode = HAL_GetTick() + 1000;
					return this;
				} else {
					next_mode = HAL_GetTick() + 1000;
					if (pwr_change != FIX_PWR_DONE && temp < base_temp && old_temp > temp) {
						if (dev_type == d_t12) {				// Add 1% of power
							base_pwr += pUnit->getMaxFixedPower()/100;
						} else {
							++base_pwr;
						}
						pUnit->fixPower(base_pwr);
						power_changed = true;
						next_mode = HAL_GetTick() + pwr_ch_to;
						if (pwr_change == FIX_PWR_DECREASED)
							pwr_change = FIX_PWR_DONE;
						else
							pwr_change = FIX_PWR_INCREASED;
					} else if (pwr_change != FIX_PWR_DONE && temp > base_temp && old_temp < temp) {
						if (dev_type == d_t12) {				// Subtract 1% of power
							base_pwr -= pUnit->getMaxFixedPower()/100;

						} else {
							--base_pwr;
						}
						pUnit->fixPower(base_pwr);
						power_changed = true;
						next_mode = HAL_GetTick() + pwr_ch_to;
						if (pwr_change == FIX_PWR_INCREASED)
							pwr_change = FIX_PWR_DONE;
						else
							pwr_change = FIX_PWR_DECREASED;
					}
					old_temp = temp;
					if (power_changed) return this;
				}
				if (old_temp && (td <= td_limit) && (pwr_change == FIX_PWR_DONE || abs(temp - base_temp) < 20)) {
					base_temp	= temp;
					delta_power = base_pwr/4;
					pD->GRAPH::reset();							// Redraw graph, because base temp has been changed!
					pD->pidShowMsg("pwr plus");
					pUnit->fixPower(base_pwr + delta_power);
					pCore->buzz.shortBeep();
					uint32_t n = HAL_GetTick();
					update_screen = n + msg_to;
					next_mode = n + 20000;						// Wait to change the temperature accordingly
					mode = TUNE_PLUS_POWER;
					phase_to	    = 0;
					if (td_limit < 150)
						td_limit = 150;
					return this;
				}
				break;
			}
			case TUNE_PLUS_POWER:								// Applying base_power+delta_power
				if ((td <= td_limit) && (pd <= 4)) {
					delta_temp	= temp - base_temp;
					pD->pidShowMsg("pwr minus");
					pUnit->fixPower(base_pwr - delta_power);
					pCore->buzz.shortBeep();
					uint32_t n = HAL_GetTick();
					update_screen = n + msg_to;
					next_mode = n + 40000;						// Wait to change the temperature accordingly
					mode = TUNE_MINUS_POWER;
					phase_to	= 0;
					return this;
				}
				break;
			case TUNE_MINUS_POWER:								// Applying base_power-delta_power
				if ((temp < (base_temp - delta_temp)) && (td <= td_limit) && (pd <= 4)) {
					tune_loops	= 0;
					uint16_t delta = base_temp - temp;
					if (delta < delta_temp)
						delta_temp = delta;						// delta_temp is minimum of upper and lower differences
					delta_temp <<= 1;							// use 2/3
					delta_temp /= 3;
					if (delta_temp < max_delta_temp)
						delta_temp = max_delta_temp;
					if ((dev_type == d_t12) && delta_temp > max_delta_temp)
						delta_temp = max_delta_temp;			// limit delta_temp in case of IRON
					pUnit->autoTunePID(base_pwr, delta_power, base_temp, delta_temp);
					pCore->buzz.doubleBeep();
					pD->pidShowMsg("start tuning");
					update_screen = HAL_GetTick() + msg_to;
					mode = TUNE_RELAY;
					phase_to	= 0;
					return this;
				}
				break;
			case TUNE_RELAY:									// Automatic tuning of PID parameters using relay method
				if (pUnit->autoTuneLoops() > tune_loops) {		// New oscillation loop
					tune_loops = pUnit->autoTuneLoops();
					if (tune_loops > 3) {
						if (tune_loops < 12) {
							uint16_t tune_period = pUnit->autoTunePeriod();
							tune_period += 250; tune_period -= tune_period%250;
							data_period	= constrain(tune_period/80, 50, 2000);	// Try to display two periods on the screen
						}
						uint16_t period = pUnit->autoTunePeriod();
						period = constrain((period+50)/100, 0, 999);
						pD->pidShowInfo(period, tune_loops);
					}
					if ((tune_loops >= 24) || ((tune_loops >= 16) && pUnit->periodStable())) {
						pUnit->switchPower(false);
						updatePID(pUnit);
						mode = TUNE_OFF;
						if (mode_spress) {
							pD->pidDestroyData();
							mode_spress->useDevice(dev_type);
							return mode_spress;
						}
					}
				}
				break;
			case TUNE_OFF:
			default:
				break;
		}
	}

	if (phase_to && HAL_GetTick() > phase_to) {
		pUnit->switchPower(false);
		mode = TUNE_OFF;
		pD->pidShowMsg("Stop");
		update_screen = HAL_GetTick() + msg_to;
		phase_to	  = 0;
		return this;
	}
	pD->pidShowGraph();
	return this;
}

/*
 * diff  = alpha^2 - epsilon^2, where
 * alpha	- the amplitude of temperature oscillations
 * epsilon	- the temperature hysteresis (see max_delta_temp)
 */
bool MAUTOPID::updatePID(UNIT *pUnit) {
	uint32_t alpha	= (pUnit->tempMax() - pUnit->tempMin() + 1) / 2;
	int32_t diff	= alpha*alpha - delta_temp*delta_temp;
	if (diff > 0) {
		pUnit->newPIDparams(delta_power, diff, pUnit->autoTunePeriod());
		pCore->buzz.shortBeep();
		return true;
	}
	return false;
}

//---------------------- The Fail mode: display error message --------------------
void MFAIL::init(void) {
	pCore->g_enc.reset(0, 0, 1, 1, 1, false);
	pCore->g_enc.reset(0, 0, 1, 1, 1, false);
	pCore->buzz.failedBeep();
	pCore->dspl.clear();
	pCore->dspl.errorMessage(message, 100);						// Write ERROR message specified with setMessage()
	if (parameter[0])
		pCore->dspl.debugMessage(parameter, 50, 200, 170);
	update_screen = 0;
}

MODE* MFAIL::loop(void) {
	uint8_t ge = pCore->g_enc.buttonStatus();
	if (ge == 2) {
		message = MSG_LAST;										// Clear message
		return mode_lpress;
	}
	if (ge || pCore->i_enc.buttonStatus()) {
		message = MSG_LAST;										// Clear message
		return mode_return;
	}
	return this;
}

void MFAIL::setMessage(const t_msg_id msg, const char *parameter) {
	message = msg;
	if (parameter) {
		strncpy(this->parameter, parameter, 19);
	} else {
		this->parameter[0] = '\0';
	}
}

//---------------------- The About dialog mode. Show about message ---------------
void MABOUT::init(void) {
	pCore->g_enc.reset(0, 0, 1, 1, 1, false);
	setTimeout(20);												// Show version for 20 seconds
	resetTimeout();
	pCore->dspl.clear();
	update_screen = 0;
}

MODE* MABOUT::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	RENC*	pEnc	= &pCore->g_enc;
	uint8_t b_status = pEnc->buttonStatus();
	if (b_status == 1) {										// Short button press
		return mode_return;										// Return to the main menu
	} else if (b_status == 2) {
		return mode_lpress;										// Activate debug mode
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 60000;

	pD->showVersion();
	return this;
}

//---------------------- The Debug mode: display internal parameters ------------
MDEBUG::MDEBUG(HW *pCore, MODE* flash_debug) : MODE(pCore)	{
	this->flash_debug = flash_debug;
}

void MDEBUG::init(void) {
	pCore->i_enc.reset(0, 0, max_iron_power, 1, 5, false);
	pCore->g_enc.reset(min_fan_speed, min_fan_speed, max_fan_power,  1, 1, false);
	pCore->dspl.clear();
	pCore->dspl.drawTitleString("Debug info");
	gun_is_on = false;
	update_screen	= 0;
}

MODE* MDEBUG::loop(void) {
	DSPL*	pD		= &pCore->dspl;
	IRON*	pIron	= &pCore->iron;
	HOTGUN*	pHG		= &pCore->hotgun;

	uint16_t pwr = pCore->i_enc.read();
	if (pwr != old_ip) {
		old_ip = pwr;
		update_screen = 0;
		pIron->fixPower(pwr);
	}
	pwr = pCore->g_enc.read();
	if (pwr != old_fp) {
		old_fp = pwr;
		update_screen = 0;
		if (gun_is_on)
			pHG->fanFixed(pwr);
		else
			pHG->fixPower(0);
	}

	if (pHG->isReedSwitch(true)) {
		if (!gun_is_on) {
			pHG->fixPower(gun_power);
			gun_is_on = true;
		}
	} else {
		if (gun_is_on) {
			pHG->fixPower(0);
			gun_is_on = false;
		}
	}

	if (pCore->i_enc.buttonStatus() == 2) {						// The iron button was pressed for a long time, launch flash debug mode
		if (flash_debug)
			return flash_debug;
	}
	if (pCore->g_enc.buttonStatus() == 2) {						// The Hot Air Gun button was pressed for a long time, exit debug mode
	   	return mode_lpress;
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + 491;						// The screen update period is a primary number to update TIM1 counter value

	uint16_t data[9];
	data[0]	= pIron->unitCurrent();
	data[1]	= pHG->unitCurrent();
	data[2]	= old_ip;
	data[3]	= pIron->temp();
	data[4]	= pIron->reedInternal();
	data[5]	= old_fp;
	data[6] = pHG->averageTemp();
	data[7]	= gtimPeriod();										// GUN_TIM period
	data[8]	= pCore->ambientInternal();

	bool gtim_ok = isACsine() && (abs(data[7] - 1000) < 50);
	pD->debugShow(data, (old_ip > 0), pHG->isReedSwitch(true), pIron->isConnected(), pHG->isConnected(), gtim_ok);
	return this;
}

void FDEBUG::init(void) {
	msg = MSG_LAST;												// No error message yet
	pCore->dspl.clear();
	pCore->dspl.drawTitle(MSG_FLASH_DEBUG);
	if (!pCore->cfg.W25Q::mount()) {							// The flash can be already mounted
		if (!pCore->cfg.W25Q::reset()) {						// Check the flash size, ensure flash is connected
			msg		= MSG_EEPROM_READ;
			status	= FLASH_ERROR;
			return;
		}
		pCore->g_enc.reset(1, 0, 1, 1, 1, true);				// "Yes or No" dialog menu
		msg		= MSG_FORMAT_EEPROM;
		status	= FLASH_NO_FILESYSTEM;
		return;
	}
	readDirectory();
	update_screen = 0;
	status = FLASH_OK;
	delete_index	= -1;										// No file o be deleted selected
	confirm_format	= false;									// No confirmation dialog activated yet
}

MODE* FDEBUG::loop(void) {
	if (pCore->i_enc.buttonStatus() == 2) {						// Iron encoder button long press, load data from the SD-card
		pCore->cfg.umount();									// SPI FLASH will be mounted later to copy data files
		pCore->dspl.clear();
		pCore->dspl.dim(50);
		pCore->dspl.debugMessage("Copying files", 10, 100, 100);
		t_msg_id e = lang_loader.load();
		if (e == MSG_LAST) {
			pCore->buzz.shortBeep();
		} else {
			char sd_status[5];
			sprintf(sd_status, "%3d", lang_loader.sdStatus());	// SD status initialized by SD_Init() function (see sdspi.c)
			pFail->setMessage(e, sd_status);
			return pFail;
		}
		return mode_lpress;
	}

	uint8_t b_status = pCore->g_enc.buttonStatus();
	if (b_status == 2) {										// The button was pressed for a long time, finish mode
	   	return mode_lpress;
	}

	if (status == FLASH_OK) {									// Flash is OK, draw the directory list
		uint8_t f_index = (delete_index > 0)?delete_index:old_ge;
		if (b_status == 1  && pCore->cfg.canDelete(dir_list[f_index].c_str())) {
			if (delete_index >= 0) {
				if (pCore->g_enc.read() == 0) {					// Confirmed to delete file
					f_unlink(dir_list[delete_index].c_str());
					readDirectory();
				} else {
					pCore->g_enc.reset(delete_index, 0, dir_list.size()-1, 1, 1, false);
				}
				pCore->dspl.clear();
				pCore->dspl.drawTitle(MSG_FLASH_DEBUG);
				delete_index = -1;
			} else {
				pCore->g_enc.reset(1, 0, 1, 1, 1, true);		// Prepare to show confirmation dialog, "No" by default
				pCore->dspl.clear();
				delete_index = old_ge;
				old_ge = 1;
			}
			update_screen = 0;
		}
		uint16_t entry = pCore->g_enc.read();					// Directory entry number
		if (entry != old_ge) {
			old_ge = entry;
			update_screen = 0;
		}
	} else if (status == FLASH_NO_FILESYSTEM) {
		if (b_status == 1 && confirm_format) {					// Button pressed
			if (old_ge == 0) {									// "Yes" selected
				if (pCore->cfg.formatFlashDrive()) {			// Format flash drive
					if (pCore->cfg.W25Q::mount()) {				// mount flash drive
						c_dir = "/";
						readDirectory();
						msg		= MSG_LAST;
						status	= FLASH_OK;
						confirm_format = false;
						update_screen = 0;
					}
				}
			} else {
				return mode_lpress;
			}
		}
	}

	if (HAL_GetTick() < update_screen) return this;
	update_screen = HAL_GetTick() + update_timeout;
	if (status == FLASH_OK) {
		if (delete_index >= 0) {
			pCore->dspl.showDialog(MSG_DELETE_FILE, 50, old_ge == 0, dir_list[delete_index].c_str());
		} else {
			pCore->dspl.directoryShow(dir_list, old_ge);
		}
	} else if (status == FLASH_NO_FILESYSTEM) {
		if (!confirm_format) {
			pCore->g_enc.reset(1, 0, 1, 1, 1, true);			// Prepare to show confirmation dialog, "No" by default
			confirm_format = true;
			old_ge = 1;
		}
		pCore->dspl.showDialog(msg, 50, old_ge == 0);
	} else {
		pCore->dspl.errorMessage(msg, 50);
	}
	return this;
}

void FDEBUG::readDirectory(void) {
	DIR	dir;
	c_dir = "/";
	FRESULT res = f_opendir(&dir, c_dir.c_str());
	if (res != FR_OK) {
		msg 	= MSG_EEPROM_DIRECTORY;
		status	= FLASH_NO_DIRECTORY;
		old_ge	= 0;
		pCore->g_enc.reset(0, 0, 0, 0, 0, false);					// No encoder change
		return;
	}
	FILINFO file_info;
	dir_list.clear();
	for (;;) {
		res = f_readdir(&dir, &file_info);
	    if ((res != FR_OK) || (file_info.fname[0] == '\0')) {
	    	break;
	    }
	    std::string entry = std::string(file_info.fname);
	    dir_list.push_back(entry);
	}
	f_closedir(&dir);
	old_ge = dir_list.size();										// Force to redraw screen
	pCore->g_enc.reset(0, 0, old_ge-1, 1, 1, false);				// Select directory entry by the rotary encoder
}
