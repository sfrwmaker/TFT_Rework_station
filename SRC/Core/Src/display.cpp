/*
 * display.cpp
 *
 * 2022 Nov 6
 * 	Changed the timeToOff() function to use the bitmap to draw the remaining time value
 * 	Changed the drawFanPcnt() function to disable clearing bottom area
 *
 * 2022 Dec 2
 *    	Changed BRGT::adjust() to use the timer instance
 * 2022 DEC 26
 *    	Changed DSPL::debugShow() to show the GUN timer period
 * 2024 OCT 09, v.1.15
 * 		Added DSPL::encoderDebugShow()
 *		Added DSPL::drawButtonStatus()
 * 2024 OCT 12
 * 		Added IPS display support to DSPL::init()
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "display.h"
#include "tools.h"
#include "main.h"					// to ensure rebuild the creation date

static const uint8_t bmDegree[] = {
  0b00111000,
  0b01000100,
  0b01000100,
  0b01000100,
  0b00111000
};

static const uint8_t bmNotCalibrated[] = {
  0b11100000, 0b00000111,
  0b11100000, 0b00000111,
  0b11000011, 0b11000011,
  0b11000011, 0b11000011,
  0b11000011, 0b11000011,
  0b11000011, 0b11000011,
  0b11000011, 0b11000011,
  0b11000011, 0b11000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000000, 0b00000011,
  0b11000011, 0b11000011,
  0b11100011, 0b11000111,
  0b11100000, 0b00000111
};

static const uint8_t bmArrowH[] = {
  0b11100000,
  0b00111100,
  0b11111111,
  0b00111100,
  0b11100000
};

static const uint8_t bmArrowV[] = {
  0b00100000,
  0b00100000,
  0b01110000,
  0b01110000,
  0b01110000,
  0b11111000,
  0b10101000,
  0b10101000
};

static const uint8_t bmFan[4][288] = {
  {	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X7F,0X80,0X00,0X00,0X00,0X00,0XFF,0XC0,0X00,0X00,0X00,0X00,
	0XFF,0XC0,0X00,0X00,0X00,0X00,0X7F,0XC0,0X00,0X00,0X00,0X00,0X3F,0XC0,0X00,0X00,
	0X00,0X00,0X3F,0XC0,0X00,0X00,0X00,0X00,0X1F,0XC0,0X00,0X00,0X00,0X00,0X1F,0XC0,
	0X00,0X00,0X00,0X00,0X0F,0XC0,0X00,0X00,0X00,0X00,0X0F,0XC0,0X00,0X00,0X00,0X00,
	0X07,0XC0,0X00,0X00,0X00,0X00,0X07,0XC0,0X00,0X00,0X00,0X00,0X07,0XC0,0X00,0X00,
	0X00,0X00,0X03,0XC0,0X00,0X00,0X00,0X00,0X03,0XC0,0X00,0X00,0X00,0X00,0X03,0XC0,
	0X00,0X00,0X00,0X00,0X03,0XC0,0X00,0X00,0X00,0X00,0X0F,0XF0,0X00,0X00,0X00,0X00,
	0X0F,0XF0,0X00,0X00,0X00,0X00,0X1F,0XF8,0X00,0X00,0X00,0X00,0X1F,0XF8,0X00,0X00,
	0X00,0X00,0X1F,0XF8,0X00,0X00,0X00,0X00,0X3F,0XFC,0X00,0X00,0X00,0X00,0XEF,0XF6,
	0X00,0X00,0X00,0X03,0XFF,0XEF,0X80,0X00,0X00,0X07,0XFB,0XDF,0XF0,0X00,0X00,0X1F,
	0XF0,0X1F,0XFF,0XF8,0X00,0X7F,0XE0,0X07,0XFF,0XF8,0X00,0XFF,0XC0,0X03,0XFF,0XF8,
	0X03,0XFF,0X80,0X01,0XFF,0XF0,0X03,0XFF,0X00,0X00,0XFF,0XF0,0X03,0XFF,0X00,0X00,
	0X7F,0XE0,0X03,0XFE,0X00,0X00,0X1F,0XC0,0X01,0XFC,0X00,0X00,0X0F,0X80,0X01,0XFC,
	0X00,0X00,0X07,0X00,0X00,0XF8,0X00,0X00,0X00,0X00,0X00,0XF8,0X00,0X00,0X00,0X00,
	0X00,0X78,0X00,0X00,0X00,0X00,0X00,0X10,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00 },
  { 0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X07,0X00,0X00,0X00,0X00,0X00,0X07,0XC0,0X00,0X00,0X00,
	0X00,0X07,0XE0,0X00,0X00,0X00,0X00,0X07,0XF0,0X00,0X00,0X00,0X00,0X07,0XF8,0X00,
	0X00,0X00,0X00,0X07,0XFC,0X00,0X00,0X00,0X00,0X07,0XFC,0X00,0X00,0X00,0X00,0X07,
	0XFC,0X00,0X00,0X00,0X00,0X07,0XF8,0X00,0X00,0X00,0X00,0X0F,0XF0,0X00,0X00,0X00,
	0X00,0X0F,0XE0,0X00,0X00,0X00,0X00,0X0F,0XE0,0X00,0X00,0X00,0X00,0X1F,0XC0,0X00,
	0X00,0X00,0X00,0X1F,0X80,0X00,0X00,0X00,0X00,0X3F,0X00,0X00,0X00,0X00,0X00,0X7E,
	0X00,0X00,0X00,0X00,0X03,0XDC,0X00,0X00,0X00,0X00,0X0F,0XEC,0X00,0X00,0X00,0X00,
	0X0F,0XF0,0X00,0X00,0X0F,0XFF,0XFF,0XF8,0X00,0X00,0X1F,0XFF,0XFF,0XF8,0X00,0X00,
	0X1F,0XFF,0XFF,0XF8,0X00,0X00,0X1F,0XFF,0XFF,0XF8,0X00,0X00,0X1F,0XFF,0X0F,0XF0,
	0X00,0X00,0X1F,0XF8,0X0F,0XF8,0X00,0X00,0X1F,0XE0,0X03,0XDC,0X00,0X00,0X1F,0X80,
	0X00,0X7E,0X00,0X00,0X1E,0X00,0X00,0X3F,0X00,0X00,0X0C,0X00,0X00,0X3F,0X80,0X00,
	0X00,0X00,0X00,0X1F,0XE0,0X00,0X00,0X00,0X00,0X1F,0XF0,0X00,0X00,0X00,0X00,0X0F,
	0XFC,0X00,0X00,0X00,0X00,0X07,0XFF,0X80,0X00,0X00,0X00,0X07,0XFF,0XC0,0X00,0X00,
	0X00,0X03,0XFF,0X80,0X00,0X00,0X00,0X03,0XFF,0X80,0X00,0X00,0X00,0X01,0XFF,0X00,
	0X00,0X00,0X00,0X00,0XFC,0X00,0X00,0X00,0X00,0X00,0XF0,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00 },
  {	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X03,0X80,0X00,0X00,0X00,0X00,
	0X07,0XC0,0X00,0X00,0X00,0X00,0X07,0XC0,0X03,0XC0,0X00,0X00,0X0F,0XC0,0X03,0XE0,
	0X00,0X00,0X0F,0XE0,0X07,0XF8,0X00,0X00,0X1F,0XE0,0X0F,0XFE,0X00,0X00,0X3F,0XE0,
	0X0F,0XFF,0X00,0X00,0X7F,0XF0,0X1F,0XFF,0XC0,0X00,0XFF,0XE0,0X1F,0XFF,0XF0,0X03,
	0XFF,0XE0,0X1F,0XFF,0XFF,0XEF,0XFF,0X80,0X3F,0XFF,0XFF,0XFF,0XFE,0X00,0X18,0X07,
	0XFF,0XF7,0XF0,0X00,0X00,0X00,0X7F,0XFF,0XC0,0X00,0X00,0X00,0X1F,0XFE,0X00,0X00,
	0X00,0X00,0X1F,0XF8,0X00,0X00,0X00,0X00,0X1F,0XF8,0X00,0X00,0X00,0X00,0X0F,0XF0,
	0X00,0X00,0X00,0X00,0X07,0XE0,0X00,0X00,0X00,0X00,0X03,0X80,0X00,0X00,0X00,0X00,
	0X03,0XE0,0X00,0X00,0X00,0X00,0X03,0XE0,0X00,0X00,0X00,0X00,0X03,0XE0,0X00,0X00,
	0X00,0X00,0X03,0XE0,0X00,0X00,0X00,0X00,0X03,0XF0,0X00,0X00,0X00,0X00,0X03,0XF0,
	0X00,0X00,0X00,0X00,0X03,0XF8,0X00,0X00,0X00,0X00,0X03,0XF8,0X00,0X00,0X00,0X00,
	0X03,0XFC,0X00,0X00,0X00,0X00,0X03,0XFC,0X00,0X00,0X00,0X00,0X03,0XFE,0X00,0X00,
	0X00,0X00,0X03,0XFF,0X00,0X00,0X00,0X00,0X03,0XFF,0X00,0X00,0X00,0X00,0X01,0XFF,
	0XC0,0X00,0X00,0X00,0X01,0XFF,0X80,0X00,0X00,0X00,0X00,0XFF,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00 },
  { 0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X0F,0X80,0X00,0X00,0X00,0X00,0X3F,0XC0,0X00,0X00,0X00,
	0X00,0X7F,0XC0,0X00,0X00,0X00,0X00,0XFF,0XE0,0X00,0X00,0X00,0X01,0XFF,0XE0,0X00,
	0X00,0X00,0X01,0XFF,0XF0,0X00,0X00,0X00,0X00,0X7F,0XF0,0X00,0X00,0X00,0X00,0X1F,
	0XF8,0X00,0X00,0X00,0X00,0X07,0XFC,0X00,0X00,0X00,0X00,0X01,0XFC,0X00,0X00,0X00,
	0X00,0X00,0XFE,0X00,0X00,0X00,0X00,0X00,0X7E,0X00,0X00,0X00,0X00,0X00,0X3E,0X00,
	0X00,0X18,0X00,0X00,0X1F,0XE0,0X00,0X3C,0X00,0X00,0X1F,0XF0,0X00,0XFC,0X00,0X00,
	0X0F,0XF0,0X07,0XFC,0X00,0X00,0X1F,0XFC,0X7F,0XFC,0X00,0X00,0X1F,0XFF,0XFF,0XFC,
	0X00,0X00,0X1F,0XFF,0XFF,0XFC,0X00,0X00,0X1F,0XFF,0XFF,0XF8,0X00,0X00,0X0F,0XF7,
	0XFF,0XF8,0X00,0X00,0X0F,0XE0,0XFF,0XF0,0X00,0X00,0X1F,0XC0,0X03,0XF0,0X00,0X00,
	0X3F,0X00,0X00,0X00,0X00,0X00,0X3E,0X00,0X00,0X00,0X00,0X00,0X3E,0X00,0X00,0X00,
	0X00,0X00,0X7E,0X00,0X00,0X00,0X00,0X00,0X7E,0X00,0X00,0X00,0X00,0X00,0XFC,0X00,
	0X00,0X00,0X00,0X01,0XFC,0X00,0X00,0X00,0X00,0X01,0XFC,0X00,0X00,0X00,0X00,0X03,
	0XFC,0X00,0X00,0X00,0X00,0X03,0XFC,0X00,0X00,0X00,0X00,0X07,0XFC,0X00,0X00,0X00,
	0X00,0X07,0XFC,0X00,0X00,0X00,0X00,0X07,0XFC,0X00,0X00,0X00,0X00,0X03,0XFE,0X00,
	0X00,0X00,0X00,0X01,0XFE,0X00,0X00,0X00,0X00,0X00,0X7E,0X00,0X00,0X00,0X00,0X00,
	0X1C,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00 },
};

static const uint8_t bmSolder[288] = {
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0XC0,0X00,0X00,0X00,0X00,0X03,0XF0,
	0X00,0X00,0X00,0X00,0X07,0X18,0X00,0X00,0X00,0X00,0X3A,0X0C,0X00,0X00,0X00,0X00,
	0X7C,0X06,0X00,0X00,0X00,0X00,0XFC,0X02,0X00,0X00,0X00,0X03,0XFC,0X00,0X00,0X00,
	0X00,0X03,0XF8,0X00,0X00,0X00,0X00,0X07,0XF0,0X00,0X00,0X00,0X00,0X0F,0XF0,0X00,
	0X00,0X00,0X00,0X1F,0XE0,0X00,0X00,0X00,0X00,0X3F,0XC0,0X00,0X62,0X00,0X00,0XFF,
	0X00,0X00,0X62,0X00,0X00,0XFE,0X00,0X00,0X46,0X00,0X01,0XFC,0X00,0X00,0XC4,0X00,
	0X03,0XFC,0X00,0X00,0XC4,0X00,0X67,0XF8,0X00,0X00,0XC4,0X00,0X73,0XF0,0X00,0X00,
	0X66,0X00,0X3C,0XC0,0X00,0X00,0X62,0X00,0X1E,0X80,0X00,0X00,0X63,0X00,0X4F,0X00,
	0X00,0X00,0X62,0X00,0XE7,0X00,0X00,0X00,0X62,0X01,0XF3,0X80,0X00,0X00,0X46,0X03,
	0XF9,0X80,0X00,0X00,0XC4,0X0F,0XF0,0X00,0X00,0X00,0X00,0X0F,0XE0,0X00,0X00,0X00,
	0X00,0X1F,0XC0,0X00,0X00,0X00,0X00,0X3F,0XC0,0X00,0X00,0X00,0X00,0X7F,0X80,0X00,
	0X00,0X00,0X00,0XFF,0X00,0X00,0X00,0X00,0X01,0XFC,0X00,0X00,0X00,0X00,0X01,0XF8,
	0X00,0X00,0X00,0X00,0X00,0XF0,0X00,0X00,0X00,0X00,0X03,0X60,0X00,0X00,0X00,0X00,
	0X03,0X00,0X00,0X00,0X00,0X00,0X06,0X00,0X00,0X00,0X00,0X00,0X1C,0X00,0X00,0X00,
	0X00,0X00,0X38,0X00,0X00,0X00,0X00,0X00,0X30,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
	0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00
};

static const uint8_t bmPower[112] = {
	0X00,0X00,0X00,0X00,0X00,0X06,0X00,0X00,0X00,0X0F,0X00,0X00,0X00,0X0F,0X00,0X00,
	0X00,0X0F,0X00,0X00,0X01,0X8F,0X38,0X00,0X07,0XCF,0X3C,0X00,0X0F,0XCF,0X3E,0X00,
	0X0F,0X8F,0X3F,0X00,0X1F,0X0F,0X0F,0X00,0X1E,0X0F,0X0F,0X80,0X3E,0X0F,0X07,0X80,
	0X3E,0X0F,0X07,0XC0,0X3C,0X0F,0X03,0XC0,0X3C,0X06,0X03,0XC0,0X3C,0X00,0X03,0XC0,
	0X3C,0X00,0X03,0XC0,0X3E,0X00,0X07,0XC0,0X3E,0X00,0X07,0X80,0X1E,0X00,0X0F,0X80,
	0X1F,0X00,0X0F,0X00,0X0F,0X80,0X3F,0X00,0X0F,0XE0,0X7E,0X00,0X07,0XFF,0XFC,0X00,
	0X03,0XFF,0XF8,0X00,0X00,0XFF,0XF0,0X00,0X00,0X3F,0X80,0X00,0X00,0X00,0X00,0X00,
};

static const uint8_t bmReady[112] = {
	0X00,0X3F,0XC0,0X00,0X00,0XFF,0XF0,0X00,0X03,0XFF,0XFC,0X00,0X07,0XC0,0X3E,0X00,
	0X0F,0X00,0X0F,0X00,0X1E,0X00,0X07,0X80,0X3C,0X00,0X03,0XC0,0X38,0X00,0X01,0XC0,
	0X70,0X00,0X00,0XE0,0X70,0X00,0X00,0XE0,0XE0,0X00,0X30,0X70,0XE0,0X00,0X78,0X70,
	0XE0,0XC0,0XF0,0X70,0XE1,0XE1,0XE0,0X70,0XE1,0XF3,0XC0,0X70,0XE0,0XF7,0X80,0X70,
	0XE0,0X7F,0X00,0X70,0XE0,0X3E,0X00,0X70,0X70,0X1C,0X00,0XE0,0X70,0X08,0X00,0XE0,
	0X38,0X00,0X01,0XC0,0X3C,0X00,0X03,0XC0,0X1E,0X00,0X07,0X80,0X0F,0X00,0X0F,0X00,
	0X07,0XC0,0X3E,0X00,0X03,0XFF,0XFC,0X00,0X00,0XFF,0XF0,0X00,0X00,0X3F,0XC0,0X00,
};

static const uint8_t bmSafe[112] = {
	0X00,0X3F,0XC0,0X00,0X00,0XFF,0XF0,0X00,0X03,0XFF,0XFC,0X00,0X07,0XFF,0XFE,0X00,
	0X0F,0XFF,0XFF,0X00,0X1F,0XFF,0XFF,0X80,0X3F,0XFF,0XFF,0XC0,0X3F,0XFF,0XFF,0XE0,
	0X7F,0XF0,0XFF,0XE0,0X7F,0X00,0X0F,0XE0,0X7F,0X00,0X0F,0XF0,0XFF,0X00,0X0F,0XF0,
	0XFF,0X00,0XCF,0XF0,0XFF,0X01,0X8F,0XF0,0XFF,0X1B,0X0F,0XF0,0XFF,0X0E,0X0F,0XF0,
	0XFF,0X84,0X0F,0XF0,0X7F,0X80,0X1F,0XF0,0X7F,0XC0,0X3F,0XE0,0X7F,0XC0,0X3F,0XE0,
	0X3F,0XF0,0X7F,0XC0,0X1F,0XFD,0XFF,0XC0,0X1F,0XFF,0XFF,0X80,0X0F,0XFF,0XFF,0X00,
	0X07,0XFF,0XFE,0X00,0X03,0XFF,0XFC,0X00,0X00,0XFF,0XF0,0X00,0X00,0X1F,0X80,0X00,
};

static const uint8_t bmStandby[112] = {
	0X00,0X1F,0XC0,0X00,0X00,0XFF,0XF2,0X00,0X03,0XF0,0XFF,0X00,0X07,0X80,0X1F,0X00,
	0X0E,0X00,0X0F,0X00,0X1C,0X0C,0X0F,0X00,0X38,0X0C,0X00,0X00,0X30,0X0C,0X00,0X00,
	0X70,0X0C,0X00,0X60,0X60,0X0C,0X00,0X60,0XE0,0X0C,0X00,0X60,0XE0,0X0C,0X00,0X00,
	0XC0,0X0C,0X00,0X00,0XC0,0X0C,0X00,0X30,0XC0,0X0F,0X00,0X30,0XC0,0X07,0X80,0X00,
	0XE0,0X01,0XE0,0X00,0XE0,0X00,0X78,0X60,0X60,0X00,0X1C,0X60,0X70,0X00,0X00,0X60,
	0X70,0X00,0X00,0X00,0X38,0X00,0X01,0X80,0X1C,0X00,0X01,0X80,0X0E,0X00,0X01,0X00,
	0X07,0X80,0X0C,0X00,0X03,0XE0,0X1C,0X00,0X01,0XF9,0XC0,0X00,0X00,0X39,0XC0,0X00,
};

static const uint8_t bmBoost[112] = {
	0X00,0X00,0X00,0X00,0X00,0X00,0X10,0X00,0X00,0X00,0X30,0X00,0X00,0X7F,0X60,0X00,
	0X01,0XFE,0XE0,0X00,0X0F,0XC1,0XEC,0X00,0X7F,0X03,0XFE,0X00,0X3E,0X07,0XCF,0X00,
	0X1C,0X0F,0XC7,0X00,0X38,0X1F,0XC3,0X80,0X38,0X3F,0X81,0X80,0X70,0X7F,0X81,0XC0,
	0X70,0XFF,0XF9,0XC0,0X71,0XFF,0XF1,0XC0,0X73,0XFF,0XE1,0XC0,0X70,0X1F,0XC1,0XC0,
	0X70,0X3F,0X81,0XC0,0X38,0X3F,0X03,0X80,0X38,0X3E,0X03,0X80,0X1C,0X7C,0X07,0X00,
	0X0E,0X78,0X0F,0X80,0X07,0X70,0X3F,0X80,0X02,0XE0,0XFE,0X00,0X00,0XDF,0XF0,0X00,
	0X00,0XFF,0XC0,0X00,0X01,0X80,0X00,0X00,0X01,0X00,0X00,0X00,0X02,0X00,0X00,0X00,
};

static const char* k_proto[3] = {
	"Kp = %5d",
	"Ki = %5d",
	"Kd = %5d"
};

bool BRGT::adjust(void) {
	if (brightness > TFT_TIM.Instance->CCR1) {
		if (brightness - TFT_TIM.Instance->CCR1 > 8) {
			TFT_TIM.Instance->CCR1 += 8;
		} else {
			++TFT_TIM.Instance->CCR1;
		}
		return true;
	} else if (brightness < TFT_TIM.Instance->CCR1) {
		if (TFT_TIM.Instance->CCR1 - brightness > 8) {
			TFT_TIM.Instance->CCR1 -= 8;
		} else {
			--TFT_TIM.Instance->CCR1;
		}
		return true;
	}
	return false;
}

void DSPL::init(bool ips) {
	// Allocate Bitmap for temperature string (3 symbols)
	setFont(big_dgt_font);
	setFontScale(2);
	uint16_t h	= getMaxCharHeight();
	uint16_t w	= getStrWidth("000") + 2;
	bm_temp		= BITMAP(w, h);

	// Allocate BITMAP for unit power gauge
	bm_gauge = BITMAP(13, h);

	// Allocate BITMAP bm_adc_read to show the temperature in the internal units
	setFont(debug_font);
	w	= getStrWidth("0000") + 2;
	bm_adc_read	= BITMAP(w, h);

	if (ips) {
		tft_ILI9341::initIPS();								// Use IPS display type
	} else {
		tft_ILI9341::init();								// Also setup display rotation
	}
	BRGT::start();
	BRGT::set(0);											// Minimum brightness
	BRGT::on();												// Apply brightness value to the timer counter
	update();												// Update the icons and symbols position on the screen depending on orientation
}

void DSPL::rotate(tRotation rotation) {
	setRotation(rotation);
	update();												// Update the icons and symbols position on the screen depending on orientation
}

void DSPL::setLetterFont(uint8_t *font) {
	if (font) {
		letter_font = font;
	} else {
		letter_font	= (uint8_t *)def_font;
		NLS_MSG::use_nls = false;							// Cannot use NLS message with default font
	}

	// Allocate Bitmap for preset temperature (3 symbols)
	setFont(letter_font);
	uint16_t h	= getMaxCharHeight();
	uint16_t w	= getStrWidth("000") + 2;
	bm_preset	= BITMAP(w, h);
}

void DSPL::clear(void) {
	BRGT::off();											// Switch-off the display brightness
	pwr_pcnt	= 255;
	fillScreen(bg_color);
}

void DSPL::drawTempSet(uint16_t temp, tUnitPos pos) {
	if (pos != u_upper && pos != u_lower) return;
	drawValue(temp, 10, (pos == u_upper)?iron_temp_y:gun_temp_y, align_center, fg_color);
}

void DSPL::drawTempGauge(int16_t t, tUnitPos pos, bool on) {
	if (pos != u_upper && pos != u_lower) return;
	uint16_t y = (pos == u_upper)?iron_temp_y:gun_temp_y;
	uint16_t w = bm_preset.width() + 20;					// Left coordinate of the gauge
	uint8_t  h = bm_temp.height();
	drawCircle(w+3, y, 3, fg_color);						// top half-circle
	drawVLine(w,    y, h, fg_color);						// Left & right borders
	drawVLine(w+6,  y, h, fg_color);
	drawCircle(w+3, y+h+5, 8, fg_color);					// Bottom part
	drawFilledCircle(w+3, y+h+5, 7, on?gd_color:bg_color);	// Fill-up the bottom
	drawFilledRect(w-3, y+9, 3, 3, fg_color);				// Label
	drawFilledRect(w+1, y, 5, h, bg_color);					// clear-up the gauge

	t = constrain(t, -100, 10);								// Limit the gauge
	if (t > 0) {
		drawFilledRect(w+1, y+10-t, 5, t+h-10, on?gd_color:bg_color);
	} else {
		uint8_t s = map(t, -100, 0, 1, h-10);
		drawFilledRect(w+1, y+h-s, 5, s, on?gd_color:bg_color);
	}
}

void DSPL::drawTemp(uint16_t temp, tUnitPos pos, uint32_t color) {
	if (pos != u_upper && pos != u_lower) return;
	if (temp >= 1000) temp = 999;
	char b[6];
	sprintf(b, "%d", temp);
	setFont(big_dgt_font);
	setFontScale(2);
	bm_temp.clear();
	strToBitmap(bm_temp, b, align_center);
	uint16_t x = width() - bm_temp.width() - 20;			// Portrait display orientation
	if (width() > height()) {								// Landscape display orientation
		x = (width() - bm_temp.width()) >> 1;
		x += 50;
	}
	uint16_t y = (pos == u_upper)?iron_temp_y:gun_temp_y;
	drawBitmap(x, y, bm_temp, bg_color, (color <= 0xffff)?color:fg_color);
}

void DSPL::animateTempCooling(uint16_t t, bool celsius, tUnitPos pos) {
	const uint16_t t_low = celsius?40:80;
	const uint16_t t_hot = celsius?90:180;
	uint16_t t_clr = constrain(t, t_low, t_hot);
	uint8_t clr_indx = map(t_clr, t_low, t_hot, 85, 0);		// Green -> Red
	uint16_t clr = wheelColor(clr_indx);
	drawTemp(t, pos, clr);
}

void DSPL::drawTipName(std::string tip_name, bool calibrated, tUnitPos pos) {
	if (pos != u_upper && pos != u_lower) return;
	setFont(letter_font);
	uint16_t h	= getMaxCharHeight();
	uint16_t w	= getStrWidth(tip_name.c_str());
	drawFilledRect(0, (pos == u_upper)?0:height()-h, tip_name_width, h, bg_color);
	uint16_t x	= 0;
	if (w < tip_name_width)
		x = (tip_name_width - w) >> 1;
	drawStr(x, (pos == u_upper)?h:height(), tip_name.c_str(), fg_color);
	if (!calibrated) {
		drawIcon(x+w+10, h-14, 16, 14, bmNotCalibrated, 16, bg_color, fg_color);
	}
	uint16_t l_width = width()-20;
	drawHLine(10, iron_area_top, l_width, fg_color);
}

void DSPL::drawFanPcnt(uint8_t p, bool modify) {
	char fan[6];
	if (p > 100) p = 100;
	const char *msg = NLS_MSG::msg(MSG_FAN);
	sprintf(fan, "%3d%c", p, '%');
	setFont(letter_font);
	uint16_t h	= getMaxCharHeight();
	BITMAP bm(fan_pcnt_width, h);
	strToBitmap(bm, msg, align_left, 0, true);				// true means UTF8
	strToBitmap(bm, fan, align_right);
	uint16_t color = modify?gd_color:fg_color;
	drawBitmap(0, height()-h, bm, bg_color, color);
	// drawFilledRect(fan_pcnt_width, height()-h, width()-fan_pcnt_width, h, bg_color); clear rest bottom area
}

void DSPL::drawAmbient(int16_t t, bool celsius) {
	char buff[5];
	setFont(letter_font);
	uint16_t h	= getMaxCharHeight();
	if (t >= -9 && t < 100) {
		char sym[] = {'C', '\0'};
		if (!celsius)
			sym[0] = 'F';
		sprintf(buff, "%2d", t);
		uint16_t x_C = width() - getStrWidth(sym) - 30;		// Position of 'C' or 'F' symbol
		uint16_t x_t =  x_C - getStrWidth(buff) - 8;		// Position of the temperature value
		drawFilledRect(x_t, height()-h, width(), h, bg_color);
		drawStr(x_C, height(), sym, fg_color);
		drawIcon(x_C-8, height()-h+4, 8, 5, bmDegree, 8, bg_color, fg_color);
		drawStr(x_t, height(), buff, fg_color);

	}
	uint16_t l_width = width()-20;
	drawHLine(10, height()-gun_area_bott, l_width, fg_color);
}

void DSPL::drawAlternate(uint16_t t, bool active, tDevice dev_type) {
	if (t > 999) t = 999;
	uint16_t x = width() - 20;								// Portrait display orientation
	if (width() > height()) {								// Landscape display orientation
		x = (width() + bm_temp.width()) >> 1;
		x += 50;
		x -= bm_preset.width();
	}
	if (t == 0) {
		drawFilledRect(x, iron_temp_y + bm_temp.height() + 8, bm_preset.width(), bm_preset.height(), bg_color);
	} else {
		setFont(letter_font);
		drawValue(t, x, iron_temp_y + bm_temp.height() + 8, align_center, active?YELLOW:BLUE);
	}
}

void DSPL::drawPower(uint8_t p, tUnitPos pos) {
	if (pos != u_upper && pos != u_lower) return;
	if (p > 100) p = 100;
	uint8_t max_h		= bm_gauge.height();
	uint8_t p_height	= gauge(p, 3, max_h);				// Applied power triangle height
	uint16_t y			= (pos == u_upper)?iron_temp_y:gun_temp_y;
	bm_gauge.drawVGauge(p_height, false);					// Draw non-edged triangle
	drawBitmap(width()-5-bm_gauge.width(), y, bm_gauge, bg_color, fg_color);
}

/*
 * Icon color depends on temperature t = (t_current - t_set)
 * Wheel Color works like this:
 * Red -> Green -> Blue -> Red
 * 0......85......170......255
 */
void DSPL::animateFan(int16_t t) {
	t = constrain(t, -1000, 0);
	uint8_t clr_indx = map(t, -1000, 0, 85, 0);				// Green-> Red
	uint16_t fan_clr = wheelColor(clr_indx);
	++fan_angle;
	fan_angle &= 0x3;										// Can be from 0 to 3
	drawIcon(fan_icon_x, fan_icon_y, 48, 48, bmFan[fan_angle], 48, bg_color, fan_clr);
}

void DSPL::stopFan(void) {
	drawIcon(fan_icon_x, fan_icon_y, 48, 48, bmFan[fan_angle], 48, bg_color, GREY);
}

void DSPL::noFan(void) {
	drawFilledRect(fan_icon_x, fan_icon_y, 48, 48, bg_color);
}

void DSPL::ironActive(bool active, tUnitPos pos) {
	if (pos != u_upper && pos != u_lower) return;
	uint16_t x = active_icon_x;
	uint16_t y = active_icon_y;
	if (pos == u_lower) {
		x = fan_icon_x;
		y = fan_icon_y;
	}
	if (active) {
		drawIcon(x, y, 48, 48, bmSolder, 48, bg_color, fg_color);
	} else {
		drawFilledRect(x, y, 48, 48, bg_color);
	}
}

void DSPL::drawTitle(t_msg_id msg_id) {
	const char *title = NLS_MSG::msg(msg_id);
	drawTitleString(title);
}

void DSPL::drawTitleString(const char *title) {
	setFont(letter_font);
	uint8_t h	= getMaxCharHeight();
	uint8_t w 	= getUTF8Width(title);
	drawUTF8((width()-w)/2, h, title, fg_color);
	drawHLine((width()-w)/2-10, h+3, w+20, fg_color);
}

void DSPL::statusIcon(const uint8_t* icon, uint16_t bg_color, uint16_t fg_color, tUnitPos pos) {
	if (pos != u_upper && pos != u_lower) return;
	uint16_t y  = (pos == u_upper)?0:gun_temp_y - 30;
	drawIcon(width()-40, y, 28, 28, icon, 28, bg_color, fg_color);
}

void DSPL::msgOFF(tUnitPos pos) {
	statusIcon(bmPower, bg_color, (uint16_t)GREY, pos);
}

void DSPL::msgON(tUnitPos pos) {
	statusIcon(bmPower, bg_color, (uint16_t)WHITE, pos);
}

void DSPL::msgNormal(tUnitPos pos) {
	statusIcon(bmPower, bg_color, (uint16_t)GREEN, pos);
}

void DSPL::msgCold(tUnitPos pos) {
	statusIcon(bmSafe, bg_color, (uint16_t)GREEN, pos);
}

void DSPL::msgReady(tUnitPos pos) {
	statusIcon(bmReady, bg_color, (uint16_t)GREEN, pos);
}

void DSPL::msgIdle(tUnitPos pos) {
	statusIcon(bmStandby, bg_color, (uint16_t)GREEN, pos);
}

void DSPL::msgStandby(tUnitPos pos) {
	statusIcon(bmStandby, bg_color, (uint16_t)BLUE, pos);
}

void DSPL::msgBoost(tUnitPos pos) {
	statusIcon(bmBoost, bg_color, (uint16_t)RED, pos);
}

void DSPL::timeToOff(uint8_t time) {
	static char msg[4];
	sprintf(msg, "%2d", time);
	setFont(letter_font);
	bm_preset.clear();
	strToBitmap(bm_preset, msg, align_left);
	drawBitmap(width()-40, 0, bm_preset, bg_color, fg_color);		// See x-coordinate in statusIcon()
	int8_t extra_y = 28 - bm_preset.height();
	if (extra_y > 0)
		drawFilledRect(width()-40, bm_preset.height(), 40, extra_y, bg_color);
}

/*
 * Icon color depends on temperature t = (t_current - t_set)
 * Wheel Color works like this:
 * Red -> Green -> Blue -> Red
 * 0......85......170......255
 */
void DSPL::animatePower(tUnitPos pos, int16_t t) {
	uint8_t clr_indx = 0;
	t = constrain(t, -50, 10);
	if (t < 0) {											// Blue -> Green
		clr_indx = map(t, -50, 0, 170, 85);
	} else {
		clr_indx = map(t, 0, 10, 85, 0);					// Green -> Red
	}
	uint16_t clr = wheelColor(clr_indx);
	statusIcon(bmPower, bg_color, clr, pos);
}

//---------------------- The Menu list display functions -------------------------
void DSPL::drawTipList(TIP_ITEM list[], uint8_t list_len, uint8_t index, bool name_only) {
	setFont(letter_font);
	uint8_t  h	= getMaxCharHeight() + 5;					// Extra space between lines
	uint16_t top = h+12;

	// Create bitmap to display tip name and activated mark
	char tip_buff[tip_name_sz+5];							// Text buffer simulating tip name
	for (uint8_t i = 0; i < tip_name_sz+4; ++i)				// Fill-up it with X (to calculate required bitmap size)
		tip_buff[i] = 'X';
	tip_buff[tip_name_sz+4] = '\0';
	uint16_t w = getStrWidth(tip_buff)+10+10+20+16;			// left margin = 10, right margin = 10 20 is space between the tip name and calibrated icon, 16 is the size of the calibrated icon
	if (!name_only)											// The checkbox size is 16 and space between checkbox and tip name is 10
		w += 16+10;
	uint8_t bm_height = getMaxCharHeight();
	BITMAP tip_entry(w, bm_height);				// The bitmap for tip name and activated mark
	uint16_t left = 0;
	if (w < width())										// Tip list aligned center
		 left = (width() - w)>>1;

	uint8_t not_calibrated_icon_y = 0;						// The row of the tip_entry bitmap to start drawing bmNotCalibrated icon
	if (bm_height > 14)										// 14 is the bmNotCalibrated height
		not_calibrated_icon_y = (bm_height - 14) >> 1;
	uint8_t max_entries = (height() - top) / h;				// Maximum entries that fits on the display
	if (list_len > max_entries) list_len = max_entries;
	// Show the tip list
	for (uint8_t i = 0; i <  list_len; ++i) {
		uint16_t fg = fg_color;
		uint16_t bg = bg_color;
		if (list[i].name[0]) {
			if (list[i].tip_index == index) {
				fg = bg_color;
				bg = fg_color;
			}
			tip_entry.clear();
			uint8_t margin = 10;
			if (!name_only) {								// Draw calibrated check box
				margin += 10+16;
				checkBox(tip_entry, 10, 16, list[i].mask & 1);
			}
			strToBitmap(tip_entry, list[i].name, align_left, margin);
			if (!(list[i].mask & 2)) {						// The tip is not calibrated
				tip_entry.drawIcon(w-16-10, not_calibrated_icon_y, bmNotCalibrated, 16, 14);
			}
			//drawScrolledBitmap(left, h*i+top, tip_entry.width(), tip_entry, 0, 0, bg, fg);
			drawBitmap(left, h*i+top, tip_entry, bg, fg);
		}
	}
	if (list_len < max_entries) {							// Clear bottom part of the display
		uint16_t y = top + h * list_len;
		drawFilledRect(left, y, tip_entry.width(), height()-y, bg_color);
	}
}

void DSPL::menuShow(t_msg_id menu_id, uint8_t item, const char* value, bool modify) {
	setFont(letter_font);
	uint8_t  h		= getMaxCharHeight() + 10;				// Extra space between menu lines
	uint16_t top	= h+12;
	uint8_t  max_items = (height() - top) / h;
	uint8_t left = (width() > height())?50:20;

	BITMAP bm_menu(width()-2*left, getMaxCharHeight());		// Bitmap to show the menu item (about full screen width)
	// Show the menu list
	uint8_t menu_size = NLS_MSG::menuSize(menu_id);
	uint8_t items = (menu_size > max_items)?max_items:menu_size; // max(max_items, menu_size)
	bool is_value = value && value[0];
	if (is_value && items == max_items) --items;
	int8_t first = item - items/2+1;
	if (menu_size > 1 && first == item) first = item - 1;
	if (first < 0) first += menu_size;
	uint16_t param_y = 0;
	for (uint8_t i = 0; i < items; ++i) {
		uint8_t menu_index = first + i;
		if (menu_index >= menu_size) menu_index -= menu_size;
		t_msg_id msg_id	= (t_msg_id)((uint8_t)menu_id+menu_index+1); // The first menu item is menu title
		const char* msg = NLS_MSG::msg(msg_id);
		bm_menu.clear();
		strToBitmap(bm_menu, msg, align_left, 10, true);	// True means UTF8
		uint16_t y = h*i+top;
		if (is_value && (param_y > 0)) y = h*i+top+h;		// Reserve extra line for menu parameter
		uint16_t bg = bg_color;
		uint16_t fg = fg_color;
		if (menu_index == item) {							// Mark active menu item
			bg = fg_color;									// Inverse colors
			fg = bg_color;
			param_y = y+h;
		}
		drawBitmap(left, y, bm_menu, bg, fg);
	}
	// Show menu item data
	if (is_value) {											// Show in-place changed menu item
		bm_menu.clear();
		strToBitmap(bm_menu, value, align_right, 0, true);	// True means UTF8
		if (!modify) {
			drawBitmap(left, param_y, bm_menu, bg_color, fg_color);
		} else {
			drawBitmap(left, param_y, bm_menu, bg_color, RED);
		}
	}
	// Clear extra line under menu list
	if (!is_value && items < max_items) {
		drawFilledRect(left, h*items+top, bm_menu.width(), h, bg_color);
	}
}

void DSPL::directoryShow(const std::vector<std::string> &dir_list, uint16_t item) {
	static const uint8_t left  = 50;
	static const uint8_t max_items = 6;

	uint16_t dir_size = dir_list.size();
	if (dir_size < item) return;
	setFont(letter_font);
	uint8_t  h		= getMaxCharHeight() + 10;				// Extra space between menu lines
	uint16_t top	= h+12;
	BITMAP bm_menu(width()-2*left, getMaxCharHeight());		// Bitmap to show the menu item (about full screen)
	// Show the menu list
	int8_t first = item;
	uint16_t remains = dir_size - item;
	if (remains < max_items)
		first = (int16_t)dir_size - (int16_t)max_items;
	if (first < 0) first = 0;
	uint8_t items = ((dir_size - first) >= max_items)?max_items:(dir_size-first);
	for (uint8_t i = 0; i < items; ++i) {
		uint8_t menu_index = first + i;
		bm_menu.clear();
		uint16_t y = h*i+top;
		uint16_t bg = bg_color;
		uint16_t fg = fg_color;
		if (menu_index <= dir_size) {
			strToBitmap(bm_menu, dir_list[menu_index].c_str(), align_left, 10, true);
			if (menu_index == item) {						// Mark active menu item
				bg = fg_color;								// Inverse colors
				fg = bg_color;
			}
		}
		drawBitmap(left, y, bm_menu, bg, fg);
	}
	// Clear extra line under menu list
	if (items < max_items) {
		drawFilledRect(left, h*items+top, bm_menu.width(), h, bg_color);
	}
}

void DSPL::tuneShow(uint16_t tune_temp, uint16_t temp, uint8_t pwr_pcnt) {
	setFont(letter_font);
	uint8_t  h		= getMaxCharHeight() + 5;				// Extra space between menu lines
	if (this->pwr_pcnt != pwr_pcnt) {						// Redraw power information
		this->pwr_pcnt = pwr_pcnt;
		uint16_t top	= h+60;
		// Show the power supplied (power triangle)
		uint16_t p_len		= map(pwr_pcnt, 0, 100, 0, width()-20);
		uint8_t p_height	= map(pwr_pcnt, 0, 100, 0, 40);
		drawFilledTriangle(10, top, width()-10, top, width()-10, top-40, bg_color); // Clear area first
		drawTriangle(10, top, width()-10, top, width()-10, top-40, fg_color); // Draw round area
		if (pwr_pcnt > 0)
			drawFilledTriangle(10, top, 10+p_len, top, 10+p_len, top-p_height, fg_color);
		// Show the power value
		char p_buff[10];
		if (pwr_pcnt > 0) {
			const char *msg = NLS_MSG::msg(MSG_PWR);
			uint8_t i = strlen(msg);						// Limit the message by 4 characters
			if (i > 4) i = 4;
			strncpy(p_buff, msg, i);
			sprintf(&p_buff[i], " %3d%c", pwr_pcnt, '%');
		} else {
			sprintf(p_buff, NLS_MSG::msg(MSG_OFF));
		}
		bm_temp.clear();									// Use bm_temp to draw the power supplied value
		strToBitmap(bm_temp, p_buff, align_center);
		drawScrolledBitmap((width()-bm_temp.width())/2, top + h + 10, bm_temp.width(), bm_temp, 0, 0, bg_color, fg_color);
	}
	// Show the temperature gauge.
	// Left and right margins are 32. The gauge starts at 32 and can reach maximum value, width()-64
	if (temp > 4095) temp = 4095;
	const uint16_t max_temp_y = height() - h;				// Y-coordinate of the maximum temp text
	const uint16_t bar_width = width()- 64;					// The temperature gauge bar width
	uint16_t t_len		= 0;								// The temperature bar length
	if (temp <= 2048) {
		t_len			= map(temp, 0, 2048, 0, 40);		// cold interval maps to 0-40
	} else {
		t_len			= map(temp, 2049, 4095, 40, bar_width);	// rest interval maps to maximum
	}
	uint8_t pos_450	= map(3600, 2049, 4095, 40, bar_width) + 32; // Also map the maximum possible temperature mark to the gauge
	// Show temperature bar frame
	drawRoundRect(32-5, max_temp_y-h-15, bar_width+10, 10, 5, fg_color); // The bar gauge
	drawFilledCircle(32-4, max_temp_y-h-10, 10, gd_color);
	drawCircle(32-4, max_temp_y-h-10, 10, fg_color);
	drawFilledRect(32, max_temp_y-h-14, t_len, 8, gd_color);// Fill-up the the bar itself
	if (t_len < bar_width-1)
		drawFilledRect(32+t_len, max_temp_y-h-14, bar_width-t_len, 8, bg_color); // Rest of bar fill with the background color
	drawFilledRect(pos_450-1, max_temp_y-h-5, 3, 6, fg_color); // Vertical thick line (3 pixel width) - max temperature label
	// Show max temperature text
	if (temp > 4095) temp = 4095;
	char mtemp_buff[6];
	sprintf(mtemp_buff, "%3d", tune_temp);
	char sym[2]			= "C";
	uint16_t w = getStrWidth(mtemp_buff);
	const uint16_t max_temp_x = pos_450 - w / 2;
	drawStr(max_temp_x, max_temp_y, mtemp_buff, fg_color);
	drawIcon(max_temp_x + w + 6, max_temp_y-h+4, 8, 5, bmDegree, 8, bg_color, fg_color);
	drawStr(max_temp_x + w + 6 + 8, max_temp_y, sym, fg_color);
}


void DSPL::calibShow(uint8_t ref_point, uint16_t current_temp, uint16_t real_temp, bool celsius, uint8_t power, bool on, bool ready, uint8_t int_temp_pcnt) {
	setFont(letter_font);
	uint8_t fo 	= getFontTopOffset();
	uint8_t h	= getFontHeight() + 5;						// Extra space between lines
	uint16_t top = h+12;

	// Show reference point number
	BITMAP bm(ref_point_width, h);
	const char *msg = NLS_MSG::msg(MSG_REF_POINT);
	char ref_buff[6];
	sprintf(ref_buff, "%d", ref_point);
	strToBitmap(bm, msg, align_left, 0, true);				// True means UTF8
	strToBitmap(bm, ref_buff, align_right);
	drawBitmap(20, top, bm, bg_color, fg_color);
	// Show power status icon
	drawIcon(width()-40, top, 28, 28, bmPower, 28, bg_color, on?WHITE:GREY);
	uint16_t x = (width() - bm_temp.width())/2 - 40;
	top += h+15;											// Line under "On/OFF" line
	drawTemp(current_temp, x, top, celsius);				// Show current temperature
	uint16_t p_top = top;									// The power triangle top
	top += bm_temp.height() + h;							// under current temperature

	if (ready) {											// Show real temperature
		drawFilledTriangle(20, top, 20, top+fo, 30, top+fo/2, fg_color);
		char temp_buff[6];
		sprintf(temp_buff, "%3d", real_temp);
		bm_preset.clear();
		strToBitmap(bm_preset, temp_buff, align_left);
		drawBitmap(40, top, bm_preset, bg_color, fg_color);
	} else {
		drawFilledRect(20, top, width()-70, bm_preset.height(), bg_color);
	}
	drawPowerTriangle(power, width()-20, p_top);			// Show the power applied
	// Show internal temperature bar
	int_temp_pcnt = constrain(int_temp_pcnt, 0, 100);
	uint16_t len = map(int_temp_pcnt, 0, 100, 0, width()-64);
	drawHGauge(len, width()-64, 32, height()-30);
}

void DSPL::calibManualShow(uint16_t ref_temp, uint16_t current_temp, uint16_t setup_temp, bool celsius, uint8_t power, bool on, bool ready) {
	setFont(letter_font);
	uint8_t h	= getMaxCharHeight() + 5;					// Extra space between lines
	uint16_t top = h+12;

	// Show power status icon
	drawIcon(width()-40, top, 28, 28, bmPower, 28, bg_color, on?WHITE:GREY);
	// Show ready sign
	if (ready) {
		drawIcon(40, top, 28, 28, bmReady, 28, bg_color, (uint16_t)GREEN);
	} else {
		drawFilledRect(40, top, 28, 28, bg_color);
	}
	top += h + 20;
	// Show reference temperature
	uint16_t x = 90;										// left point of temperature digit
	const char *msg = NLS_MSG::msg(MSG_SET);
	bm_temp.clear();
	uint16_t w = strToBitmap(bm_temp, msg, align_left, 0, true); // True means UTF8
	uint16_t x1 = (w+10 < x)?x-w-10:0;
	drawBitmapArea(x1, top+bm_temp.height()-h+5, w, bm_temp.height(), bm_temp, bg_color, fg_color);
	drawTemp(ref_temp, x, top, celsius);
	uint16_t power_top = (width() > height())?top:top+bm_temp.height()+20;
	drawPowerTriangle(power, width()-20, power_top);		// Show the power applied
	top += bm_temp.height();
	// Show internal temperature bar
	const uint16_t bar_length = width()-64;					// Left & right margins are 32 pixels
	const uint16_t detail_area = 3*bar_length/8;			// Detailed area is 3/4 of the bar length
	uint16_t g_diff = 0;									// Gauge difference between current and setup temperatures
	uint16_t t_diff = abs((int)current_temp - (int)setup_temp) / 4;
	if (t_diff <= detail_area) {
		g_diff = t_diff;
	} else {
		t_diff -= detail_area;
		t_diff = constrain(t_diff, 0, 1000);
		g_diff = detail_area + map(t_diff, 0, 1000, 0, bar_length/8);
	}
	// Calculate temperature gauge length
	uint16_t len = 0;
	if (current_temp >= setup_temp) {
		len = bar_length/2 + g_diff;						// Add the difference to the middle point (middle point is a setup temperature)
	} else {
		len = bar_length/2 - g_diff;						// Substract the difference from the middle point
	}
	len = constrain(len, 0, bar_length);
	top = height()-30;
	drawHGauge(len, bar_length, 32, top, bar_length/2);
	// Draw the temperature in the internal units
	w = bm_adc_read.width();
	if (w > 0) {
		char buff[6];
		sprintf(buff, "%4d", setup_temp);
		bm_adc_read.clear();
		strToBitmap(bm_adc_read, buff, align_center);
		uint8_t h = bm_adc_read.height();
		drawBitmapArea((width()-w)/2, top-h-2, w, bm_adc_read.height(), bm_adc_read, bg_color, fg_color);
	}
}

void DSPL::endCalibration(void) {
	bm_calib_power.~BITMAP();
}

//---------------------- The PID calibration process functions -------------------
bool DSPL::pidStart(void) {
	setFont(letter_font);
	uint8_t h = getMaxCharHeight() + 5;
	uint16_t top = h + 30;
	uint16_t t_height = height()-top-5;						// The temperature graph height, leave 5 bottom lines free
	if ((t_height & 1) == 0) t_height--;					// Ensure the graph height is odd to draw abscissa coordinate axis

	uint16_t data_size = width() - bm_preset.width() - 54;
	if (GRAPH::allocate(data_size)) {
		// Allocate space for graph pixmap
		if (pm_graph.width() == 0) {
			pm_graph = PIXMAP(data_size, t_height, 2);		// Depth is 2 bits, 4-color graph
			uint16_t g_colors[4] = { bg_color, fg_color, gd_color, dp_color};
			pm_graph.setupPalette(g_colors, 4);
		}
		return true;
	}
	return false;
}

void DSPL::pidAxis(const char *title, const char *temp, const char *disp) {
	setFont(letter_font);
	uint8_t h = getMaxCharHeight() + 5;
	uint16_t top = h + 30;
	uint16_t t_height = height()-top-5;						// The temperature graph height, leave 5 bottom lines free
	if ((t_height & 1) == 0) t_height--;					// Ensure the graph height is odd to draw abscissa coordinate axis
	uint8_t temp_zero = t_height/2;

	// Show the axis of coordinates
	const uint16_t ord = bm_preset.width()+19;				// The graph ordinate axis X coordinate
	drawHLine(bm_preset.width()+5, temp_zero+top, width()-70, fg_color); // The abscissa axis (time)
	drawIcon(width()-70+bm_preset.width()+5, temp_zero+top-2, 8, 5, bmArrowH, 8, bg_color, fg_color); // abscissa axis arrow
	drawStr(width()-20, temp_zero+top-5, "t", fg_color);	// The axis label, time
	drawVLine(ord, 10, height()-12, fg_color);				// The ordinate axis (temperature/dispersion)
	drawIcon(ord-2, 5, 5, 8, bmArrowV, 8, bg_color, fg_color);	// The ordinate axis arrow
	drawHLine(ord-4, top, 9, gd_color);						// Maximum temperature label
	drawHLine(ord-4, top+h, 9, dp_color);					// Maximum dispersion label
	// Show graph title and axis labels
	drawStr(bm_preset.width()-15, h, temp, gd_color);
	uint16_t hp = drawStr(bm_preset.width()+30, h, disp, dp_color) + bm_preset.width()+30;
	uint16_t w = getStrWidth(title);
	drawStr(hp+(width()-hp-w)/2, h, title, fg_color);
}

void DSPL::pidModify(uint8_t index, uint16_t value) {
	char modified_value[25];								// The buffer to show current value of being modified coefficient
	if (index < 3) {
		sprintf(modified_value, k_proto[index], value);
	}
	setFont(letter_font);
	uint8_t  h = getMaxCharHeight();
	uint16_t w = getStrWidth(modified_value);
	uint16_t x = (width()-w) >> 1;
	uint16_t y = height()*3/4;
	uint8_t  o = getFontTopOffset();
	drawFilledRect(x-10, y-o-10, w+20, h+20, bg_color);
	drawStr(x, y, modified_value, fg_color);
}

void DSPL::pidShowGraph(void) {
	setFont(letter_font);
	uint8_t h	= getMaxCharHeight() + 5;					// Extra space between lines
	uint16_t top = h+30;

	// Check both bitmaps allocated successfully
	if (pm_graph.width() == 0) return;
	const uint16_t t_height  = pm_graph.height();			// The temperature graph height, leave 5 bottom lines free
	const uint8_t  temp_zero = t_height/2;					// The temperature abscissa axis vertical coordinate inside bitmap
    const uint8_t  disp_zero = t_height-1;					// The dispersion  abscissa axis vertical coordinate

	// Calculate the transition coefficient for the temperature, dispersion and applied power
	int16_t	 min_t = 32767;
	int16_t  max_t = -32767;
	uint16_t max_d = 0;										// Maximum value for dispersion
	uint16_t till  = GRAPH::dataSize();
	for (uint8_t i = 0; i < till; ++i) {
		int16_t  t = GRAPH::temp(i);
		uint16_t d = GRAPH::disp(i);
		if (min_t > t) min_t = t;							// Here h_temp is average_temp - preset_temp
		if (max_t < t) max_t = t;
		if (max_d < d) max_d = d;
	}
	if (min_t < 0)		min_t *= -1;						// If graph under zero is bigger (the temperature is lower than preset one)
	if (max_t < min_t)	max_t = min_t;						// normalize graph by its lower part
	uint16_t d_height = t_height - h;						// Dispersion graph height is lower because we should write max dispersion value

	// Normalize the graph data and fill-up the graph pixmap
	pm_graph.clear();
	int16_t g0[2]	= {0};									// Previous value of graph: temperature, dispersion
	int16_t g1[2]	= {0};									// Current  value of graph: temperature, dispersion
	for (uint16_t i = 0; i < till-1; ++i) {					// fill-up the graph bitmaps
		// Normalize the data to be plotted. Current points are g1[x]
		g0[0] = g1[0];										// Temperature graph. Save previous data
		if (max_t == 0) {
			g1[0] = temp_zero;
		} else {
			int16_t t = GRAPH::temp(i);
			if (t > 0) {
				g1[0] = temp_zero - round((float)t * (float)temp_zero / (float)max_t);
				if (g1[0] < 1) g1[0] = 1;
			} else {
				int16_t neg = t * (-1);
				g1[0] = temp_zero + round((float)neg * (float)temp_zero / (float)max_t);
				if (g1[0] >= t_height) g1[0] = t_height - 1;
			}
		}
		g0[1] = g1[1];										// Dispersion graph. Save previous data
		if (max_d == 0) {
			g1[1] = disp_zero;
		} else {
			int16_t d = round((float)GRAPH::disp(i) * (float)d_height / (float)max_d);
			if (d >= d_height) d = d_height-1;
			g1[1] = disp_zero - d;
		}
		// draw line between nearby points from t0 to t1, from d0 to d1, from p0 and p1
		if (i > 0) {										// we need two points to draw the line
			for (int8_t gr = 1; gr >= 0; --gr) {			// Through the graphs
				uint16_t top_dot = g1[gr];					// draw vertical line from top_dot and length is len
				uint16_t len = 0;
				if (g1[gr] <= g0[gr]) {
					len = g0[gr] - g1[gr] + 1;
				} else {
					top_dot = g0[gr];
					len = g1[gr] - g0[gr] + 1;
				}
				pm_graph.drawVLineCode(i-1, top_dot, len, gr+2);
			}
		}
	}
	pm_graph.drawHLineCode(0, temp_zero, pm_graph.width(), 1);	// Draw the temperature abscissa axis
	drawPixmap(bm_preset.width()+20, top, pm_graph.width(), t_height, pm_graph);

	// draw graph maximum value labels and applied power
	drawValue(max_t, 0, top-h/2, align_right, gd_color);	// Show maximum value of temperature
	drawValue(max_d, 0, top+h/2, align_right, dp_color);	// Show maximum value of dispersion
}

void DSPL::pidShowMenu(uint16_t pid_k[3], uint8_t index) {
	static const uint8_t left  = 50;
	char buff[12];

	setFont(letter_font);
	drawTitle(MSG_TUNE_PID);
	uint8_t  h		= getMaxCharHeight() + 5;				// Extra space between menu lines
	uint16_t top	= h+12;
	BITMAP bm_menu(width()-2*left, getMaxCharHeight());		// Bitmap to show the menu item (about full screen)
	// Show the Coefficient values
	for (uint8_t i = 0; i < 3; ++i) {
		sprintf(buff, k_proto[i], pid_k[i]);
		bm_menu.clear();
		strToBitmap(bm_menu, buff, align_left, 5);
		uint16_t fg = fg_color;
		uint16_t bg = bg_color;
		if (index == i) {
			fg = bg_color;
			bg = fg_color;
		}
		//drawScrolledBitmap(left, top+i*h, bm_menu.width(), bm_menu, 0, 0, bg, fg);
		drawBitmap(left, top+i*h, bm_menu, bg, fg);
	}
}

void DSPL::pidShowMsg(const char *msg) {
	setFont(letter_font);
	drawStr(100, height()-50, msg, pid_color);
}

void DSPL::pidShowInfo(uint16_t period, uint16_t loops) {
	drawValue(period, 0, height()-50, align_right, pid_color);
	drawValue(loops,  0, height()-70, align_right, pid_color);
}

void DSPL::pidDestroyData(void) {
	if (pm_graph.width() > 0)
		pm_graph.~PIXMAP();
	GRAPH::freeData();
}

void DSPL::errorMessage(t_msg_id err_id, uint16_t y) {
	const char *err = NLS_MSG::msg(MSG_ERROR);
	fillScreen(bg_color);
	const char *msg = NLS_MSG::msg(err_id);
	if (msg[0] == 0) {													// No error message specified, show big "ERROR"
		setFont(letter_font);
		setFontScale(3);
		uint16_t w = getUTF8Width(err);
		uint16_t x = (width() - w) >>1;
		drawUTF8(x, height()/2, err, fg_color);
		setFontScale(1);
	} else {
		setFont(letter_font);
		uint16_t h	= getMaxCharHeight() + 5;
		uint8_t len = strlen(msg);
		char *err_msg = (char*)malloc(len+1);
		if (!err_msg)
			return;
		strcpy(err_msg, msg);
		uint8_t line = 1;
		for (uint8_t start = 0; start < len; ) {
			uint8_t finish = start+1;
			while (++finish < len) {
				if (err_msg[finish] == '\n')
					break;
			}
			bool not_end = (finish < len);
			if (not_end)
				err_msg[finish] = '\0';
			uint8_t wid = getUTF8Width(&err_msg[start]);
			if (wid < width()) {
				drawUTF8((width()-wid)/2, y+line*h, &err_msg[start], fg_color);
			} else {
				drawUTF8(0, y+line*h, &err_msg[start], fg_color);
			}
			if (++line > 7)												// Only 7 lines display can fit the display
				break;
			if (not_end)
				err_msg[finish] = '\n';
			start = finish + 1;
		}
		free(err_msg);
	}
}

void DSPL::showDialog(t_msg_id msg_id, uint16_t y, bool yes, const char *parameter) {
	setFont(letter_font);
	const char *m = NLS_MSG::msg(msg_id);
	uint16_t h = getMaxCharHeight() + 5;
	uint16_t w = getUTF8Width(m);
	uint16_t pos = (w < width())?((width()-w) >> 1):0;
	drawUTF8(pos, y, m, fg_color);
	if (parameter) {
		w = getUTF8Width(parameter);
		pos = (w < width())?((width()-w) >> 1):0;
		y += h;
		drawUTF8(pos, y, parameter, fg_color);
	}
	uint16_t space = (width() - bm_preset.width())/3;
	for (uint8_t i = 0; i < 2; ++i) {
		uint16_t bg = (i == (uint8_t)yes)?bg_color:fg_color;
		uint16_t fg = (i == (uint8_t)yes)?fg_color:bg_color;
		m = (i==0)?NLS_MSG::msg(MSG_YES):NLS_MSG::msg(MSG_NO);
		bm_preset.clear();
		strToBitmap(bm_preset, m, align_center, 0, true);				// True means UTF8
		drawBitmap(space + i * (bm_preset.width() + space), y+h, bm_preset, bg, fg);
	}
}

void DSPL::showVersion(void) {
	static const char *name  = "IRONs & Hot Air Gun";
	char buff[30];
	setFont(letter_font);
	uint16_t h	= getMaxCharHeight() + 5;
	fillScreen(bg_color);
	// Show title
	drawTitle(MSG_ABOUT);
	// Show name
	uint16_t top = h + 12;
	uint16_t w = getStrWidth(name);
	uint16_t x = (width() - w ) >> 1;
	drawStr(x, top+h, name, fg_color);
	// Show software version
	sprintf(buff, "Controller v.%s", FW_VERSION);
	w = getStrWidth(buff);
	x = (width() - w ) >> 1;
	drawStr(x, h*2+top, buff, fg_color);
	// Print date of compilation
	sprintf(buff, "%s", __DATE__);
	w = getStrWidth(buff);
	x = (width() - w ) >> 1;
	drawStr(x, h*3+top, buff, fg_color);
}

void DSPL::debugShow(uint16_t data[9], bool iron_on, bool gun_on, bool iron_connected, bool gun_connected, bool is_ac_ok) {
	static const char *item_name[9] = {
			"irnC:",
			"funC:",
			"irnP:",
			"irnT:",
			"tilt:",
			"fanP:",
			"gunT:",
			"TIM1:",
			"amb.:"
	};
	char buff[10];
	setFont(debug_font);
	uint8_t  h		= getMaxCharHeight() + 5;							// Extra space between menu lines
	uint16_t top	= h+12;
	BITMAP bm(width()/2-40, getMaxCharHeight());
	strToBitmap(bm, "iron:", align_left);								// The IRON power status
	strToBitmap(bm, iron_on?" ON":"OFF", align_right);
	drawScrolledBitmap(10, top, bm.width(), bm, 0, 0, bg_color, fg_color);
	bm.clear();
	sprintf(buff, "%5d", data[0]);										// The current through IRON
	strToBitmap(bm, item_name[0], align_left);
	strToBitmap(bm, buff, align_right);
	drawScrolledBitmap(10, top+h, bm.width(), bm, 0, 0, bg_color, fg_color);
	bm.clear();
	strToBitmap(bm, "gun:", align_left);								// The GUN power status
	strToBitmap(bm, gun_on?" ON":"OFF", align_right);
	drawScrolledBitmap(10, top+3*h, bm.width(), bm, 0, 0, bg_color, fg_color);
	bm.clear();
	sprintf(buff, "%5d", data[1]);										// Fan current
	strToBitmap(bm, item_name[1], align_left);
	strToBitmap(bm, buff, align_right);
	drawScrolledBitmap(10, top+4*h, bm.width(), bm, 0, 0, bg_color, fg_color);
	for (uint8_t i = 0; i < 7; ++i){
		bm.clear();
		sprintf(buff, "%5d", data[i+2]);
		strToBitmap(bm, item_name[i+2], align_left);
		strToBitmap(bm, buff, align_right);
		uint16_t clr = fg_color;
		if (i == 5 && !is_ac_ok) clr = gd_color;						// GUN_TIM status
		drawScrolledBitmap(width()/2+10, top+i*h, bm.width(), bm, 0, 0, bg_color, clr);
	}
	sprintf(buff, "(%c-%c)", iron_connected?'i':' ', gun_connected?'g':' ');
	bm.clear();
	strToBitmap(bm, buff);
	drawScrolledBitmap(10, top+6*h, bm.width(), bm, 0, 0, bg_color, fg_color);
}

void DSPL::debugMessage(const char *msg, uint16_t x, uint16_t y, uint16_t len) {
	setFont(letter_font);
	uint8_t  h	= getMaxCharHeight();
	drawFilledRect(x, y, len, h, bg_color);
	drawStr(x, y+h, msg, fg_color);
}

void DSPL::encoderDebugShow(uint16_t i_enc, uint32_t i_ints, uint8_t i_b, uint16_t g_enc, uint32_t g_ints, uint8_t g_b, uint8_t ret) {
	if (i_ints > 999) i_ints = 999;
	if (g_ints > 999) g_ints = 999;
	drawTemp(i_enc, u_upper, fg_color);
	drawTempSet(i_ints, u_upper);
	drawButtonStatus(i_b, 10, iron_temp_y + 30, fg_color);

	drawTemp(g_enc, u_lower, fg_color);
	drawTempSet(g_ints, u_lower);
	drawButtonStatus(g_b, 10, gun_temp_y  + 30, fg_color);

	char b[20];
	sprintf(b, "return in %2d sec", ret);
	setFont(letter_font);
	uint16_t h	= getMaxCharHeight();
	BITMAP bm(200, h);
	strToBitmap(bm, b, align_center);
	drawBitmap((width() - 200)>>1, height()-h, bm, bg_color, dim_color);
}

void DSPL::checkBox(BITMAP &bm, uint16_t x, uint8_t size, bool checked) {
	uint16_t w = bm.width();
	uint8_t  h = bm.height();
	if (w == 0  || size == 0 || x+size > w || size > h) return;
	uint8_t y = (h - size) >> 1;
	bm.drawHLine(x, y, size);											// Top horizontal line
	bm.drawHLine(x, y+size-1, size);									// Bottom horizontal line
	bm.drawVLine(x, y+1, size-2);										// Left vertical line
	bm.drawVLine(x+size-1, y+1, size-2);								// Right vertical line
	if (checked && size > 6) {
		for (uint16_t i = 0; i < size - 6; ++i) {						// Fill-up the square box
			bm.drawHLine(x+3, y+3+i, size-6);
		}
	}
}

void DSPL::drawTemp(uint16_t temp, uint16_t x, uint16_t y, bool celsius) {
	char sym[] = {'C', '\0'};
	if (!celsius)
		sym[0] = 'F';
	setFont(big_dgt_font);
	setFontScale(2);
	char buff[6];
	sprintf(buff, "%3d", temp);
	bm_temp.clear();
	strToBitmap(bm_temp, buff, align_center);
	drawBitmap(x, y, bm_temp, bg_color, fg_color);
	x+= bm_temp.width() + 8;
	drawIcon(x, y, 8, 5, bmDegree, 8, bg_color, fg_color);
	setFont(letter_font);
	uint8_t h = getMaxCharHeight();
	drawStr(x+8, y+h-1, sym, fg_color);
}

void DSPL::drawPowerTriangle(uint8_t power, uint16_t x, uint16_t p_top) {
	uint16_t bm_w = bm_calib_power.width();		// 20x110
	if (bm_w < 20)
		bm_calib_power = BITMAP(20, 110);							// Create the power gauge bitmap
	power = gauge(power, 4, 100);									// Increase low power values
	uint16_t p_height = map(power, 0, 100, 0, bm_calib_power.height());
	bm_calib_power.drawVGauge(p_height, true);
	drawBitmap(x-20, p_top, bm_calib_power, bg_color, fg_color);
}

void DSPL::drawHGauge(uint16_t len, uint16_t g_width, uint16_t x, uint16_t y, int16_t label) {
	drawFilledCircle(x, y+5, 4, gd_color);								// Fill-up the left corner of the gauge
	drawRoundRect(x-5, y, g_width+10, 10, 5, fg_color);
	if (len > 0)
		drawFilledRect(x, y+1, len, 8, gd_color);
	drawFilledRect(x+len, y+1, g_width-len, 8, bg_color);
	if (label > 0)
		drawFilledRect(x+label-1, y-4, 3, 18, fg_color);
}

// Draw three digits value 0-999
void DSPL::drawValue(uint16_t value, uint16_t x, uint16_t y, BM_ALIGN align, uint16_t color) {
	if (value > 999) value = 999;
	char b[6];
	sprintf(b, "%d", value);
	setFont(letter_font);
	bm_preset.clear();
	strToBitmap(bm_preset, b, align);
	drawBitmap(x, y, bm_preset, bg_color, color);
}

void DSPL::drawButtonStatus(uint8_t button, uint16_t x, uint16_t y, uint16_t color) {
	char b[6];
	if (button == 1) {
		sprintf(b, "sht");												// Short pressed
	} else if (button == 2) {											// Long pressed
		sprintf(b, "lng");
	} else {
		b[0] = '-'; b[1] = 0;
	}
	setFont(letter_font);
	bm_preset.clear();
	strToBitmap(bm_preset, b, align_center);
	drawBitmap(x, y, bm_preset, bg_color, color);
}

// Update icons and symbols coordinate
void DSPL::update(void) {
	gun_temp_y = height() - gun_temp_y_off;								// Y-coordinate of the gun temperature depends on display orientation
	if (width() > height()) {											// Landscape display orientation
		fan_icon_x		= bm_preset.width() + 50;
		fan_icon_y		= gun_temp_y;
		active_icon_x	= bm_preset.width() + 50;
		active_icon_y	= iron_temp_y;
	} else {															// Portrait display orientation
		fan_icon_x		= width() - 100;
		fan_icon_y		= (height() >> 1) - 24;
		active_icon_x	= 40;
		active_icon_y	= fan_icon_y;
	}
}

