/*
 * hw.h
 *
 *  Created on: 12 july 2021
 *      Author: Alex
 */

#ifndef HW_H_
#define HW_H_

/*
 * This is a fusion file to join c++ project and main.c file generated by CubeMX
 */

#include "stat.h"
#include "iron.h"
#include "gun.h"
#include "encoder.h"
#include "display.h"
#include "config.h"
#include "buzzer.h"
#include "nls.h"
#include "nls_cfg.h"

class HW {
	public:
		HW(void) : cfg(),
			i_enc(I_ENC_R_GPIO_Port, I_ENC_R_Pin, I_ENC_L_GPIO_Port, I_ENC_L_Pin),
			g_enc(G_ENC_R_GPIO_Port, G_ENC_R_Pin, G_ENC_L_GPIO_Port, G_ENC_L_Pin)		{ }
		CFG_STATUS	init(void);
		uint16_t			ambientInternal(void)			{ return t_amb.read();							}
		bool				noAmbientSensor(void)			{ return t_amb.read() >= max_ambient_value;		}
		void				updateAmbient(uint32_t value)	{ t_amb.update(value);							}
		int32_t				ambientTemp(void);				// T12 IRON ambient temperature
		CFG			cfg;
		NLS			nls;
		DSPL		dspl;
		IRON		iron;
		RENC		i_enc, g_enc;
		HOTGUN		hotgun;
		BUZZER		buzz;
	private:
		EMP_AVERAGE 	t_amb;								// Exponential average of the ambient temperature
		const uint8_t	ambient_emp_coeff	= 30;			// Exponential average coefficient for ambient temperature
		const uint16_t	max_ambient_value	= 3900;			// About -30 degrees. If the soldering IRON disconnected completely, "ambient" value is greater than this
};

#endif