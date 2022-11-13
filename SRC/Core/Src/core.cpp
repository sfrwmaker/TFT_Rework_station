/*
 * core.cpp
 *
 *  Created on: 30 aug 2019
 *      Author: Alex
 */

#include "core.h"
#include "hw.h"
#include "mode.h"

#define ADC_CONV 	(5)										// Activated ADC Ranks Number (hadc2.Init.NbrOfConversion)
#define ADC_LOOPS	(4)										// Number of ADC conversion loops. Even value better.
#define ADC_BUFF_SZ	(ADC_CONV*ADC_LOOPS)

extern ADC_HandleTypeDef	hadc1;
extern TIM_HandleTypeDef	htim1;							// HOT AIR GUN + AC_Zero
extern TIM_HandleTypeDef	htim2;							// IRON POWER + HOT AIR GUN FAN POWER
extern TIM_HandleTypeDef	htim4;							// BUZZER

typedef enum { ADC_IDLE, ADC_CURRENT, ADC_TEMP } t_ADC_mode;
volatile static t_ADC_mode	adc_mode = ADC_IDLE;
volatile static uint16_t	buff[ADC_BUFF_SZ];
volatile static	uint32_t	tim1_cntr		= 0;			// Previous value of TIM1 counter. Using to check the TIM1 value changing
volatile static	bool		ac_sine			= false;		// Flag indicating that TIM1 is driven by AC power interrupts on AC_ZERO pin
const static uint16_t  		max_iron_pwm	= 1960;			// Max value should be less than TIM2.CHANNEL3 value by 20
const static uint16_t  		max_gun_pwm		= 99;			// TIM1 period. Full power can be applied to the HOT GUN
const static uint32_t		check_sw_period = 100;			// IRON switches check period, ms

static HW		core;										// Hardware core (including all device instances)

// MODE instances
static	MWORK			work(&core);
static	MSLCT			iselect(&core);
static	MTACT			activate(&core);
static	MCALIB			calib_auto(&core);
static	MCALIB_MANUAL	calib_manual(&core);
static	MCALMENU		calib_menu(&core, &calib_auto, &calib_manual);
static	MTUNE			tune(&core);
static	MFAIL			fail(&core);
static	MMBST			boost_setup(&core);
static	MTPID			pid_tune(&core);
static 	MAUTOPID		auto_pid(&core);
//static	MENU_GUN		gun_menu(&core, &calib_manual, &tune, &auto_pid);
static	MENU_GUN		gun_menu(&core, &calib_manual, &tune, &pid_tune);
static  MABOUT			about(&core);
static	FDEBUG			flash_debug(&core, &fail);
static  MDEBUG			debug(&core, &flash_debug);
static	MSETUP			param_menu(&core);
//static	MMENU			main_menu(&core, &boost_setup, &param_menu, &calib_menu, &activate, &tune, &auto_pid, &gun_menu, &about);
static	MMENU			main_menu(&core, &boost_setup, &iselect, &param_menu, &calib_menu, &activate, &tune, &pid_tune, &gun_menu, &about);
static	MODE*           pMode = &work;

bool isACsine(void) 	{ return ac_sine; }

// Synchronize TIM2 timer to AC power
uint16_t syncAC(void) {
	uint32_t to = HAL_GetTick() + 300;						// The timeout
	uint16_t nxt_tim1	= TIM1->CNT + 2;
	if (nxt_tim1 > 99) nxt_tim1 -= 99;						// TIM1 is clocked by AC zero crossing signal, period is 99.
	while (HAL_GetTick() < to) {							// Prevent hang
		if (TIM1->CNT == nxt_tim1) {
			TIM2->CNT = 0;									// Synchronize TIM2 to AC power zero crossing signal
			break;
		}
	}
	// Checking the TIM2 has been synchronized
	to = HAL_GetTick() + 300;
	nxt_tim1 = TIM1->CNT + 2;
	if (nxt_tim1 > 99) nxt_tim1 -= 99;
	while (HAL_GetTick() < to) {
		if (TIM1->CNT == nxt_tim1) {
			return TIM2->CNT;
		}
	}
	return TIM2->ARR+1;										// This value is bigger than TIM2 period, the TIM2 has not been synchronized
}

bool confirm(void) {
	uint8_t p = 2;											// Make sure the message sill be displayed for the first time in the loop
	core.g_enc.reset(1, 0, 1, 1, 1, true);
	core.dspl.clear();
	core.dspl.drawTitle(MSG_EEPROM_READ);
	core.dspl.BRGT::set(128);								// Turn on the display backlight
	core.dspl.BRGT::on();

	while (true) {
		uint8_t answer = core.g_enc.read();
		if (answer != p) {
			p = answer;
			core.dspl.showDialog(MSG_FORMAT_EEPROM, 100, answer == 0);
		}
		if (core.g_enc.buttonStatus() > 0)
			return answer == 0;
		if (core.dspl.BRGT::adjust()) {						// Adjust display brightness
			HAL_Delay(5);
		}
	}
	return false;
}

extern "C" void setup(void) {
	CFG_STATUS cfg_init = core.init();						// Initialize the hardware structure before start timers
	core.iron.setCheckPeriod(3);							// Start checking the T12 IRON connectivity
	HAL_TIM_PWM_Start(&htim1,	TIM_CHANNEL_4);				// PWM signal of Hot Air Gun
	HAL_TIM_OC_Start_IT(&htim1, TIM_CHANNEL_3);				// Calculate power of Hot Air Gun interrupt
	HAL_TIM_PWM_Start(&htim2, 	TIM_CHANNEL_1);				// PWM signal of the IRON
	HAL_TIM_PWM_Start(&htim2, 	TIM_CHANNEL_2);				// PWM signal of FAN (Hot Air Gun)
	HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_3);				// Check the current through the IRON and FAN, also check ambient temperature
	HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_4);				// Calculate power of the IRON
	HAL_TIM_PWM_Start(&htim4,   TIM_CHANNEL_4);				// PWM signal for the buzzer

	// Setup main mode parameters: return mode, short press mode, long press mode
	work.setup(&main_menu, &iselect, &main_menu);
	iselect.setup(&work, &activate, &main_menu);
	activate.setup(&work, &work, &main_menu);
	calib_auto.setup(&work, &work, &work);
	calib_manual.setup(&calib_menu, &work, &work);
	calib_menu.setup(&work, &work, &work);
	tune.setup(&work, &work, &work);
	fail.setup(&work, &work, &work);
	boost_setup.setup(&main_menu, &main_menu, &work);
	pid_tune.setup(&work, &work, &work);
	gun_menu.setup(&main_menu, &work, &work);
	param_menu.setup(&main_menu, &work, &work);
	main_menu.setup(&work, &work, &work);
	about.setup(&work, &work, &debug);
	debug.setup(&work, &work, &work);
	flash_debug.setup(&fail, &work, &work);
	auto_pid.setup(&work, &pid_tune, &pid_tune);

	core.dspl.clear();
	switch (cfg_init) {
		case CFG_NO_TIP:
			pMode	= &activate;							// No tip configured, run tip activation menu
			break;
		case CFG_READ_ERROR:								// Failed to read EEPROM
			fail.setMessage(MSG_EEPROM_READ);
			pMode	= &fail;
			break;
		case CFG_NO_FILESYSTEM:
			if (confirm()) { 								// Yes
				if (!core.cfg.formatFlashDrive()) {
					fail.setMessage(MSG_FORMAT_FAILED);
					pMode	= &fail;
				}
			}
			break;
		default:
			break;
	}

	syncAC();												// Synchronize TIM2 timer to AC power
	HAL_Delay(1500);										// Wait till hardware status updated, for instance, the ambient temperature
	uint8_t br = core.cfg.getDsplBrightness();
	core.dspl.BRGT::set(br);
	//core.dspl.BRGT::on()								 	// Tuurn-on the display backlight immediately. Also, comment-out the BRGT::off(); line in void DSPL::clear()
	pMode->init();
}

extern "C" void loop(void) {
	static uint32_t AC_check_time = 0;						// Time in ms when to check TIM1 is running
	static uint32_t	check_sw	  = 0;						// Time when check iron switches status (ms)
	if (HAL_GetTick() > check_sw) {
		check_sw = HAL_GetTick() + check_sw_period;
		GPIO_PinState pin = HAL_GPIO_ReadPin(TILT_SW_GPIO_Port, TILT_SW_Pin);
		core.iron.updateReedStatus(GPIO_PIN_SET == pin);		// Update T12 TILT switch status
		pin = HAL_GPIO_ReadPin(REED_SW_GPIO_Port, REED_SW_Pin);
		core.hotgun.updateReedStatus(GPIO_PIN_SET == pin);	// Switch active when the Hot Air Gun handle is off-hook
	}
	MODE* new_mode = pMode->returnToMain();
	if (new_mode && new_mode != pMode) {
		core.buzz.doubleBeep();
		core.iron.switchPower(false);
		TIM2->CCR1	= 0;									// Switch-off the IRON power immediately
		pMode = new_mode;
		pMode->init();
		return;
	}
	new_mode = pMode->loop();
	if (new_mode != pMode) {
		if (new_mode == 0) new_mode = &fail;				// Mode Failed
		core.iron.switchPower(false);
		core.hotgun.switchPower(false);
		core.iron.setCheckPeriod(0);						// Stop checking t12 IRON
		TIM2->CCR1	= 0;									// Switch-off the IRON power immediately
		pMode = new_mode;
		pMode->init();
	}

	// If TIM1 counter has been changed since last check, we received AC_ZERO events from AC power
	if (HAL_GetTick() >= AC_check_time) {
		ac_sine		= (TIM1->CNT != tim1_cntr);
		tim1_cntr	= TIM1->CNT;
		AC_check_time = HAL_GetTick() + 41;					// 50Hz AC line generates 100Hz events. The pulse period is 10 ms
	}

	// Adjust display brightness
	if (core.dspl.BRGT::adjust()) {
		HAL_Delay(5);
	}
}

static bool adcStart(t_ADC_mode mode) {
    if (adc_mode != ADC_IDLE) {								// Not ready to check analog data; Something is wrong!!!
    	TIM2->CCR1 = 0;										// Switch off the IRON
    	TIM1->CCR4 = 0;										// Switch off the Hot Air Gun
		return false;
    }
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)buff, ADC_CONV*ADC_LOOPS);
	adc_mode = mode;
	return true;
}

/*
 * IRQ handler
 * on TIM1 Output channel #3 to calculate required power for Hot Air Gun
 * on TIM2 Output channel #3 to read the current through the IRON and Fan of Hot Air Gun
 * also check that TIM1 counter changed driven by AC_ZERO interrupt
 * on TIM2 Output channel #4 to read the IRON, HOt Air Gun and ambient temperatures
 */
extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM1 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
		uint16_t gun_power	= core.hotgun.power();
		TIM1->CCR4	= constrain(gun_power, 0, max_gun_pwm);	// Apply Hot Air Gun power
	} else if (htim->Instance == TIM2) {
		if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
			if (TIM2->CCR1 || TIM2->CCR2)					// If IRON of Hot Air Gun has been powered
				adcStart(ADC_CURRENT);
		} else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
			adcStart(ADC_TEMP);
		}
	}
}

/*
 * IRQ handler of ADC complete request. The data is in the ADC buffer (buff)
 * Data read by 5 slots: adc1-rank1, adc1-rank2, ..., adc1-rank5
 * The ADC buffer would have the following fields (see MX_ADC1_Init() in main.c)
 * 0: iron_current
 * 1: fan_current
 * 2: iron_temp
 * 3: gun_temp
 * 4: ambient
 */
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	if (hadc->Instance != ADC1) return;
	HAL_ADC_Stop_DMA(&hadc1);
	if (adc_mode == ADC_TEMP) {								// Read the temperatures only, the current should be ignored
		volatile uint32_t iron_temp	= 0;
		volatile uint32_t gun_temp	= 0;
		volatile uint32_t ambient 	= 0;
		for (uint8_t i = 0; i < ADC_BUFF_SZ; i += ADC_CONV) {
			iron_temp	+= buff[i+2];
			gun_temp	+= buff[i+3];
			ambient		+= buff[i+4];
		}
		iron_temp 	+= ADC_LOOPS/2;							// Round the result
		iron_temp 	/= ADC_LOOPS;
		gun_temp 	+= ADC_LOOPS/2;							// Round the result
		gun_temp  	/= ADC_LOOPS;
		ambient 	+= ADC_LOOPS/2;							// Round the result
		ambient  	/= ADC_LOOPS;
		core.updateAmbient(ambient);

		// Apply power to iron
		uint16_t iron_power = core.iron.power(iron_temp);
		TIM2->CCR1	= iron_power;
		core.hotgun.updateTemp(gun_temp);					// Update average Hot Air Gun temperature. Apply the power by TIM1.CNANNEL3 interrupt
	} else if (adc_mode == ADC_CURRENT) {					// Read the currents, the temperatures should be ignored
		volatile uint32_t iron_curr	= 0;
		volatile uint32_t fan_curr 	= 0;
		for (uint8_t i = 0; i < ADC_BUFF_SZ; i += ADC_CONV) {
			iron_curr	+= buff[i];
			fan_curr	+= buff[i+1];
		}
		iron_curr	+= ADC_LOOPS/2;							// Round the result
		iron_curr	/= ADC_LOOPS;
		fan_curr	+= ADC_LOOPS/2;							// Round the result
		fan_curr	/= ADC_LOOPS;

		if (TIM2->CCR1)										// If IRON has been powered
			core.iron.updateCurrent(iron_curr);
		if (TIM2->CCR2)										// If Hot Air Gun Fan has been powered
			core.hotgun.updateCurrent(fan_curr);
	}
	adc_mode = ADC_IDLE;
}

extern "C" void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) 				{ }
extern "C" void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc) 	{ }

// Iron Encoder Rotated
extern "C" void EXTI0_IRQHandler(void) {
	if(__HAL_GPIO_EXTI_GET_IT(I_ENC_L_Pin) != RESET) {
	    core.i_enc.encoderIntr();
		__HAL_GPIO_EXTI_CLEAR_IT(I_ENC_L_Pin);
	}
}

// Hot Air Gun Encoder Rotated
extern "C" void EXTI1_IRQHandler(void) {
	if(__HAL_GPIO_EXTI_GET_IT(G_ENC_L_Pin) != RESET) {
		core.g_enc.encoderIntr();
		__HAL_GPIO_EXTI_CLEAR_IT(G_ENC_L_Pin);
	}
}
