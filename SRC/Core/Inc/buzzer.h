/*
 * buzzer.h
 *
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#ifndef __BUZZ_H
#define __BUZZ_H
#include "main.h"

class BUZZER {
	public:
		BUZZER(void);
		void		activate(bool e)						{ enabled = e; }
		void		lowBeep(void);
		void		shortBeep(void);
		void		doubleBeep(void);
		void		failedBeep(void);
	private:
		void		playTone(uint16_t period_mks, uint16_t duration_ms);
		bool		enabled = true;
};

#endif

#endif /* BUZZER_H_ */
