/*
 * display.h
 *
 *
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <vector>
#include <string>
#include "tft.h"
#include "cfgtypes.h"
#include "graph.h"
#include "font.h"
#include "nls.h"
#include "tools.h"

// TFT brightness control class
extern TIM_HandleTypeDef htim3;

class BRGT {
	public:
		BRGT(void)								{ }
		void		start(void)								{ HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);	}
		void		stop(void)								{ HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);	}
		void		set(uint8_t brightness)					{ this->brightness = brightness;			}
		uint8_t		get(void)								{ return TIM3->CCR1 & 0xff;					}
		void		off(void)								{ TIM3->CCR1 = 0;							}
		void		dim(uint8_t br)							{ TIM3->CCR1 = br;							}
		void		on(void)								{ TIM3->CCR1 = brightness;					}
		bool		adjust(void);
	private:
		uint8_t	brightness			= 0;					// Setup display brightness
};

typedef enum { u_lower = 0, u_upper = 1, u_extra = 2, u_none = 3 } tUnitPos;

class DSPL : public tft_ILI9341, public BRGT, public GRAPH, public NLS_MSG {
	public:
					DSPL(void) : tft_ILI9341()				{ }
		virtual		~DSPL()									{ }
		void		init();
		void		rotate(tRotation rotation);
		void		setLetterFont(uint8_t *font);
		void		clear(void);
		void		drawTemp(uint16_t temp, tUnitPos pos, uint32_t color = 0xFF0000);
		void		animateTempCooling(uint16_t t, bool celsius, tUnitPos pos);
		void		drawTempSet(uint16_t temp, tUnitPos pos);
		void		drawTempGauge(int16_t t, tUnitPos pos, bool on);	// temp - temp_set
		void		drawTipName(std::string tip_name, bool calibrated, tUnitPos pos);
		void		drawFanPcnt(uint8_t p, bool modify = false);
		void		drawAmbient(int16_t t, bool celsius);
		void		drawAlternate(uint16_t t, bool active, tDevice dev_type); // Alternative unit temperature (JBC or T12)
		void		drawPower(uint8_t p, tUnitPos pos);
		void		animateFan(int16_t t);
		void		stopFan(void);
		void 		noFan(void);
		void		ironActive(bool active, tUnitPos pos);
		void		drawTitle(t_msg_id msg_id);
		void		drawTitleString(const char *title);
		void 		statusIcon(const uint8_t* icon, uint16_t bg_color, uint16_t fg_color, tUnitPos pos);
		void 		msgOFF(tUnitPos pos);
		void		msgON(tUnitPos pos);
		void		msgNormal(tUnitPos pos);
		void 		msgCold(tUnitPos pos);
		void 		msgReady(tUnitPos pos);
		void 		msgIdle(tUnitPos pos);
		void 		msgStandby(tUnitPos pos);
		void 		msgBoost(tUnitPos pos);
		void 		timeToOff(uint8_t time);
		void 		animatePower(tUnitPos pos, int16_t t);
		void		drawTipList(TIP_ITEM list[], uint8_t list_len, uint8_t index, bool name_only);
		void		menuShow(t_msg_id menu_id, uint8_t item, const char* value, bool modify);
		void		directoryShow(const std::vector<std::string> &dir_list, uint16_t item);
		void		tuneShow(uint16_t tune_temp, uint16_t temp, uint8_t pwr_pcnt);
		void 		calibShow(uint8_t ref_point, uint16_t current_temp, uint16_t real_temp, bool celsius, uint8_t power, bool on, bool ready, uint8_t int_temp_pcnt);
		void		calibManualShow(uint16_t ref_temp, uint16_t current_temp, uint16_t setup_temp, bool celsius, uint8_t power, bool on, bool ready);
		void		endCalibration(void);
		bool		pidStart(void);
		void		pidAxis(const char *title, const char *temp, const char *disp);
		void		pidModify(uint8_t index, uint16_t value);
		void		pidShowGraph(void);
		void		pidShowMenu(uint16_t pid_k[3], uint8_t index);
		void		pidShowMsg(const char *msg);
		void		pidShowInfo(uint16_t period, uint16_t loops);
		void		pidDestroyData(void);
		void		errorMessage(t_msg_id err_id, uint16_t y);
		void		showDialog(t_msg_id msg_id, uint16_t y, bool yes, const char *parameter = 0);
		void 		showVersion(void);
		void 		debugShow(uint16_t data[9], bool iron_on, bool gun_on, bool iron_connected, bool gun_connected);
		void		debugMessage(const char *msg, uint16_t x, uint16_t y, uint16_t len);
	private:
		void		checkBox(BITMAP &bm, uint16_t x, uint8_t size, bool checked);
		void		drawTemp(uint16_t temp, uint16_t x, uint16_t y, bool celsius);
		void		drawPowerTriangle(uint8_t power, uint16_t x0, uint16_t p_top);
		void		drawHGauge(uint16_t len, uint16_t g_width, uint16_t x, uint16_t y, int16_t label = -1);
		void		drawValue(uint16_t value, uint16_t x, uint16_t y, BM_ALIGN align, uint16_t color);
		void		update(void);
		uint8_t*	letter_font			= (uint8_t*)u8g_font_profont22r;
		uint16_t	bg_color			= 0;
		uint16_t	fg_color			= 0xFFFF;
		uint16_t	gd_color			= RED;				// Temperature gauge color
		uint16_t	dp_color			= BLUE;				// Dispersion graph color
		uint16_t	pid_color			= YELLOW;			// Pid information color
		uint16_t	dim_color			= LIGHTGREY;		// Not active item color
		uint8_t		fan_angle			= 0;
		BITMAP		bm_temp, bm_preset, bm_adc_read, bm_gauge;
		BITMAP		bm_calib_power;							// Used to draw the applied power during calibration procedure
		PIXMAP		pm_graph;
		uint8_t		pwr_pcnt			= 255;				// The power percent applied
		uint16_t	gun_temp_y			= 150;				// Y coordinate of hot air gun coordinate (depends on screen orientation)
		uint16_t	fan_icon_x			= 0;				// Fan animated icon coordinates
		uint16_t	fan_icon_y			= 0;
		uint16_t	active_icon_x		= 0;				// Iron active icon coordinates
		uint16_t	active_icon_y		= 0;
		const uint16_t	tip_name_width	= 200;
		const uint16_t	fan_pcnt_width	= 110;
		const uint16_t	ref_point_width	= 90;
		const uint16_t	iron_temp_y		= 60;				// Y coordinate of iron temperature
		const uint16_t	gun_temp_y_off	= 90;				// Y coordinate of gun temperature, bottom offset
		const uint16_t	iron_area_top	= 32;
		const uint16_t  gun_area_bott	= 25;				// Gun area bottom offset
		const uint8_t*	big_dgt_font	= u8g2_font_kam28n;
		const uint8_t*	debug_font		= u8g_font_profont22r;
		const uint8_t*	def_font		= u8g_font_profont22r;
};


#endif
