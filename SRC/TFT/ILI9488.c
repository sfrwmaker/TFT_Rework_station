/*
 * ILI9488.c
 *
 *  Created on: Nov 17, 2020
 *      Author: Alex
 */

#include <stdint.h>
#include "ILI9488.h"
#include "common.h"
#include "interface.h"

typedef enum {
	CMD_NOP_00			= 0x00,
	CMD_SWRESET_01		= 0x01,
	CMD_RDDIDIF_04		= 0x04,
	CMD_RDERR_DSI_05	= 0x05,
	CMD_RDDST_09		= 0x09,
	CMD_RDDPM_0A		= 0x0A,
	CMD_RDDMADCTL_0B	= 0x0B,
	CMD_RDDCOLMOD_0C	= 0x0C,
	CMD_RDDIM_0D		= 0x0D,
	CMD_RDDSM_0E		= 0x0E,
	CMD_RDDSDR_0F		= 0x0F,
	CMD_SPLIN_10		= 0x10,
	CMD_SLPOUT_11		= 0x11,
	CMD_PTLON_12		= 0x12,
	CMD_NORON_13		= 0x13,
	CMD_DINVOFF_20		= 0x20,
	CMD_DINVON_21		= 0x21,
	CMD_DPXOFF_22		= 0x22,
	CMD_DPXON_23		= 0x23,
	CMD_DISPOFF_28		= 0x28,
	CMD_DISPON_29		= 0x29,
	CMD_CASET_2A		= 0x2A,
	CMD_PASET_2B		= 0x2B,
	CMD_RAMWR_2C		= 0x2C,
	CMD_RAMRD_2E		= 0x2E,
	CMD_PLTAR_30		= 0x30,
	CMD_VSCRDEF_33		= 0x33,
	CMD_TEOFF_34		= 0x34,
	CMD_TEON_35			= 0x35,
	CMD_MADCTL_36		= 0x36,
	CMD_VSCRSADD_37		= 0x37,
	CMD_IDMOFF_38		= 0x38,
	CMD_IDMON_39		= 0x39,
	CMD_PIXSET_3A		= 0x3A,
	CMD_WRMEMCONT_3C	= 0x3C,
	CMD_RDMEMCONT_3E	= 0x3E,
	CMD_TESCANLN_44		= 0x44,
	CMD_RDTESCANLN_45	= 0x45,
	CMD_WRDISBV_51		= 0x51,
	CMD_RDDISBV_52		= 0x52,
	CMD_WRCTRLD_53		= 0x53,
	CMD_RDCTRLD_54		= 0x54,
	CMD_WRCABC_55		= 0x55,
	CMD_RDCABC_56		= 0x56,
	CMD_ADJCTRL2_58		= 0x58,
	CMD_WRCABC_5E		= 0x5E,
	CMD_RDCABC_5F		= 0x5F,
	CMD_RDABR_68		= 0x68,
	CMD_RDID1_DA		= 0xDA,
	CMD_RDID2_DB		= 0xDB,
	CMD_RDID3_DC		= 0xDC,
	CMD_IFMODE_B0		= 0xB0,
	CMD_FRMCTR1_B1		= 0xB1,
	CMD_FRMCTR2_B2		= 0xB2,
	CMD_FRMCTR3_B3		= 0xB3,
	CMD_INVTR_B4		= 0xB4,
	CMD_PRCTR_B5		= 0xB5,
	CMD_DISCTRL_B6		= 0xB6,
	CMD_ETMOD_B7		= 0xB7,
	CMD_CLRENHCR1_B9	= 0xB9,
	CMD_CLRENHCR2_BA	= 0xBA,
	CMD_HSLANESCR_BE	= 0xBE,
	CMD_PWCTRL1_C0		= 0xC0,
	CMD_PWCTRL2_C1		= 0xC1,
	CMD_PWCTRL3_C2		= 0xC2,
	CMD_PWCTRL4_C3		= 0xC3,
	CMD_PWCTRL5_C4		= 0xC4,
	CMD_VMCTRL1_C5		= 0xC5,
	CMD_CABC1_C6		= 0xC6,
	CMD_CABC2_C8		= 0xC8,
	CMD_CABC3_C9		= 0xc9,
	CMD_CABC4_CA		= 0xCA,
	CMD_CABC5_CB		= 0xCB,
	CMD_CABC6_CC		= 0xCC,
	CMD_CABC7_CD		= 0xCD,
	CMD_CABC8_CE		= 0xCE,
	CMD_CABC9_CF		= 0xCF,
	CMD_NVMWR_D0		= 0xD0,
	CMD_NVMPKEY_D1		= 0xD1,
	CMD_RDNVM_D2		= 0xD2,
	CMD_RDID4_D3		= 0xD3,
	CMD_ADJCTRL1_D7		= 0xD7,
	CDM_RDIDCK_D8		= 0xD8,
	CMD_PGAMCTRL_E0		= 0xE0,
	CMD_NGAMCTRL_E1		= 0xE1,
	CMD_DGAMCTRL_E2		= 0xE2,
	CMD_DGAMCTRL_E3		= 0xE3,
	CMD_SIMGF_E9		= 0xE9,
	
	CMD_IFCTL_0xF6		= 0xf6,
	CMD_ADJCTRL3_F7		= 0xf7,
	CMD_ADJCTRL4_F8		= 0xf8,
	CMD_ADJCTRL5_F9		= 0xf9,
	CMD_ADJ_CTRL6_FC	= 0xfc,
	CDM_ADJ_CTRL7_FF	= 0xff
	
} ILI9341_CMD;


// Initialize the Display
void ILI9488_Init(void) {
	// Initialize display interface. By default the library guess the display interface
	TFT_InterfaceSetup(TFT_18bits, 0);
	// Reset display hardware
	TFT_DEF_Reset();
	// SOFTWARE RESET
	TFT_Command(CMD_SWRESET_01,	0, 0);
	TFT_Delay(1000);

	TFT_Command(CMD_PGAMCTRL_E0,	(uint8_t *)"\x00\x03\x09\x08\x16\x0A\x3F\x78\x4C\x09\x0A\x08\x16\x1a\x0f", 15);
	TFT_Command(CMD_NGAMCTRL_E1,	(uint8_t *)"\x00\x16\x19\x03\x0F\x05\x32\x45\x46\x04\x0E\x0D\x35\x37\x0F", 15);

	// Power Control 1: Vreg1out, Verg2out
	TFT_Command(CMD_PWCTRL1_C0,	(uint8_t *)"\x17\x15", 2);
	// Power Control 2: VGH, VGL
	TFT_Command(CMD_PWCTRL2_C1,	(uint8_t *)"\x41", 1);
	TFT_Delay(5);
	// Power Control 3: Vcom
	TFT_Command(CMD_VMCTRL1_C5,	(uint8_t *)"\x00\x12\x80", 3);
	TFT_Delay(5);
	// Interface Mode Control: DIN and SDO pins are used for 3/4 wire serial interface.
	TFT_Command(CMD_IFMODE_B0,		(uint8_t *)"\x00", 1);
	// Interface Pixel Format: 18 bits
	TFT_Command(CMD_PIXSET_3A,		(uint8_t *)"\x66", 1);

	// Frame rate: 60 Hz
	TFT_Command(CMD_FRMCTR1_B1,	(uint8_t *)"\xA0", 1);
	// Display Inversion Control: 2 dot inversion
	TFT_Command(CMD_INVTR_B4,		(uint8_t *)"\x02", 1);
	// Display function control
	TFT_Command(CMD_DISCTRL_B6,	(uint8_t *)"\x02\0x02", 2);
	// Set Image Function: Disable 24-bits Data Bus
	TFT_Command(CMD_SIMGF_E9,		(uint8_t *)"\x00", 1);
	// Adjust Control 3: DSI write DCS command, use loose packet RGB 666
	TFT_Command(CMD_ADJCTRL3_F7,	(uint8_t *)"\xA9\x51\x2C\x82", 4);
	TFT_Delay(5);

	// Exit sleep mode
	TFT_DEF_SleepOut();

	// Turn On Display
	TFT_Command(CMD_DISPON_29,		0, 0);
	TFT_Delay(5);
	// Memory access mode
	TFT_Command(CMD_MADCTL_36,		(uint8_t *)"\0x48", 1);

	// Setup display parameters
	uint8_t rot[4] = {0x40|0x08, 0x20|0x08, 0x80|0x08, 0x40|0x80|0x20|0x08};
	TFT_Setup(ILI9488_SCREEN_WIDTH, ILI9488_SCREEN_HEIGHT, rot);
}

// Initialize the Display
void ILI9488_IPS_Init(void) {
	// Initialize display interface. By default the library guess the display interface
	TFT_InterfaceSetup(TFT_18bits, 0);
	// Reset display hardware
	TFT_DEF_Reset();
	// SOFTWARE RESET
	TFT_Command(CMD_SWRESET_01,	0, 0);
	TFT_Delay(1000);

	TFT_Command(CMD_PGAMCTRL_E0,	(uint8_t *)"\x00\x03\x09\x08\x16\x0A\x3F\x78\x4C\x09\x0A\x08\x16\x1a\x0f", 15);
	TFT_Command(CMD_NGAMCTRL_E1,	(uint8_t *)"\x00\x16\x19\x03\x0F\x05\x32\x45\x46\x04\x0E\x0D\x35\x37\x0F", 15);

	// Turn-on inversion
	TFT_Command(CMD_INVTR_B4, (uint8_t *)"\x00", 1);
	TFT_Command(CMD_DINVON_21, 0, 0);

	// Power Control 1: Vreg1out, Verg2out
	TFT_Command(CMD_PWCTRL1_C0,	(uint8_t *)"\x17\x15", 2);
	// Power Control 2: VGH, VGL
	TFT_Command(CMD_PWCTRL2_C1,	(uint8_t *)"\x41", 1);
	TFT_Delay(5);
	// Power Control 3: Vcom
	TFT_Command(CMD_VMCTRL1_C5,	(uint8_t *)"\x00\x12\x80", 3);
	TFT_Delay(5);
	// Interface Mode Control: DIN and SDO pins are used for 3/4 wire serial interface.
	TFT_Command(CMD_IFMODE_B0,		(uint8_t *)"\x00", 1);
	// Interface Pixel Format: 18 bits
	TFT_Command(CMD_PIXSET_3A,		(uint8_t *)"\x66", 1);

	// Frame rate: 60 Hz
	TFT_Command(CMD_FRMCTR1_B1,	(uint8_t *)"\xA0", 1);
	// Display Inversion Control: 2 dot inversion
	TFT_Command(CMD_INVTR_B4,		(uint8_t *)"\x02", 1);
	// Display function control
	TFT_Command(CMD_DISCTRL_B6,	(uint8_t *)"\x02\0x02", 2);
	// Set Image Function: Disable 24-bits Data Bus
	TFT_Command(CMD_SIMGF_E9,		(uint8_t *)"\x00", 1);
	// Adjust Control 3: DSI write DCS command, use loose packet RGB 666
	TFT_Command(CMD_ADJCTRL3_F7,	(uint8_t *)"\xA9\x51\x2C\x82", 4);
	TFT_Delay(5);

	// Exit sleep mode
	TFT_DEF_SleepOut();

	// Turn On Display
	TFT_Command(CMD_DISPON_29,		0, 0);
	TFT_Delay(5);
	// Memory access mode
	TFT_Command(CMD_MADCTL_36,		(uint8_t *)"\0x48", 1);
	
	// Setup display parameters
	uint8_t rot[4] = {0x40|0x08, 0x20|0x08, 0x80|0x08, 0x40|0x80|0x20|0x08};
	TFT_Setup(ILI9488_SCREEN_WIDTH, ILI9488_SCREEN_HEIGHT, rot);
}
