/*
 * ST7735.c
 *
 *  Created on: Nov 19 2020
 *      Author: Alex
 */

#include <stdint.h>
#include "ST7735.h"
#include "common.h"

typedef enum {
	CMD_NOP_00			= 0x00,
	CMD_SWRESET_01		= 0x01,
	CMD_RDDID_04		= 0x04,
	CMD_RDERR_DSI_05	= 0x05,
	CMD_RDDST_09		= 0x09,
	CMD_RDDPM_0A		= 0x0A,
	CMD_RDDMADCTL_0B	= 0x0B,
	CMD_RDDCOLMOD_0C	= 0x0C,
	CMD_RDDIM_0D		= 0x0D,
	CMD_RDDSM_0E		= 0x0E,
	CMD_RDDSDR_0F		= 0x0F,
	CMD_SLPIN_10		= 0x10,
	CMD_SLPOUT_11		= 0x11,
	CMD_PTLON_12		= 0x12,
	CMD_NORON_13		= 0x13,
	CMD_DINVOFF_20		= 0x20,
	CMD_DINVON_21		= 0x21,
	CMD_DISPOFF_28		= 0x28,
	CMD_DISPON_29		= 0x29,
	CMD_CASET_2A		= 0x2A,
	CMD_PASET_2B		= 0x2B,
	CMD_RAMWR_2C		= 0x2C,
	CMD_RAMRD_2E		= 0x2E,
	CMD_PTLAR_30		= 0x30,
	CMD_VSCRDEF_33		= 0x33,
	CMD_TEOFF_34		= 0x34,
	CMD_TEON_35			= 0x35,
	CMD_MADCTL_36		= 0x36,
	CMD_VSCRSADD_37		= 0x37,
	CMD_IDMOFF_38		= 0x38,
	CMD_IDMON_39		= 0x39,
	CMD_PIXSET_3A		= 0x3A,
	CMD_RDMEMCONT_3E	= 0x3E,
	CMD_TESCAN_44		= 0x44,
	CMD_RDTESCAN_45		= 0x45,
	CMD_WRDISBV_51		= 0x51,
	CMD_RDDISBV_52		= 0x52,
	CMD_WRCTRLD_53		= 0x53,
	CMD_RDCTRLD_54		= 0x54,
	CMD_WRCABC_55		= 0x55,
	CMD_RDCABC_56		= 0x56,
	CMD_WRCABCMB_5E		= 0x5E,
	CMD_RDCABCMB_5F		= 0x5F,
	CMD_RDFCHKSUM_AA	= 0xAA,
	CMD_RDCCHKSUM_AF	= 0xAF,
	CMD_RDID1_DA		= 0xDA,
	CMD_RDID2_DB		= 0xDB,
	CMD_RDID3_DC		= 0xDC,
	CMD_IFMODE_B0		= 0xB0,
	CMD_FRMCTR1_B1		= 0xB1,
	CMD_FRMCTR2_B2		= 0xB2,
	CMD_FRMCTR3_B3		= 0xB3,
	CMD_INVTR_B4		= 0xB4,
	CMD_BPC_B5			= 0xB5,
	CMD_DFC_B6			= 0xB6,
	CMD_EM_B7			= 0xB7,
	CMD_PWCTRL1_C0		= 0xC0,
	CMD_PWCTRL2_C1		= 0xC1,
	CMD_PWCTRL3_C2		= 0xC2,
	CMD_VMCTRL1_C5		= 0xC5,
	CMD_VCMOFFS_C6		= 0xC6,
	CMD_NVMADW_D0		= 0xD0,
	CMD_NVMBPROG_D1		= 0xD1,
	CMD_NVMSTRD_D2		= 0xD2,
	CMD_RDID4_D3		= 0xD3,
	CMD_PGAMCTRL_E0		= 0xE0,
	CMD_NGAMCTRL_E1		= 0xE1,
	CMD_DGAMCTRL_E2		= 0xE2,
	CMD_DGAMCTRL_E3		= 0xE3,
	CMD_DOCA_E8			= 0xE8,
	CMD_CSCON_F0		= 0xF0,
	CMD_SPIRC_FB		= 0xFB
} ST7796_CMD;

// Initialize the Display
void ST7735_Init(void) {
	// Initialize display interface. By default the library guess the display interface
	TFT_InterfaceSetup(TFT_16bits, 0);
	// Reset display hardware
	TFT_DEF_Reset();
	// SOFTWARE RESET
	TFT_Delay(1);
	TFT_Command(CMD_SWRESET_01,		0, 0);
	TFT_Delay(500);

	// positive gamma control
	TFT_Command(CMD_PGAMCTRL_E0,	(uint8_t *)"\x09\x16\x09\x20\x21\x1B\x13\x19\x17\x15\x1E\x2B\x04\x05\x02\x0E", 16);
	// negative gamma control
	TFT_Command(CMD_NGAMCTRL_E1,	(uint8_t *)"\x0B\x14\x08\x1E\x22\x1D\x18\x1E\x1B\x1A\x24\x2B\x06\x06\x02\x0F", 16);
	// Power Control 1 (Vreg1out, Verg2out)
	TFT_Command(CMD_PWCTRL1_C0,		(uint8_t *)"\x17\x15", 2);
	// Power Control 2 (VGH,VGL)
	TFT_Command(CMD_PWCTRL2_C1,		(uint8_t *)"\x41", 1);
	// Power Control 3 (Vcom)
	TFT_Command(CMD_VMCTRL1_C5,		(uint8_t *)"\x00\x12\x80", 3);
	// Interface Pixel Format (16 bit)
	TFT_Command(CMD_PIXSET_3A,		(uint8_t *)"\x55", 1);
	// Interface Mode Control (SDO NOT USE)
	TFT_Command(CMD_IFMODE_B0,		(uint8_t *)"\x80", 1);
	// Frame rate (60Hz)
	TFT_Command(CMD_FRMCTR1_B1,		(uint8_t *)"\xA0", 1);
	// Display Inversion Control (2-dot)
	TFT_Command(CMD_INVTR_B4,		(uint8_t *)"\x02", 1);
	// Display Function Control RGB/MCU Interface Control
	TFT_Command(CMD_DFC_B6,			(uint8_t *)"\x02\x02", 2);
	// Set Image Function (Disable 24 bit data)
	TFT_Command(0xE9,				(uint8_t *)"\x00", 1);
	// Adjust Control (D7 stream, loose)
	TFT_Command(0xF7,				(uint8_t *)"\xA9\x51\x2C\x82", 4);

	TFT_Command(CMD_MADCTL_36,		(uint8_t *)"\x00", 1);

	// Exit sleep mode
	TFT_DEF_SleepOut();

	// Turn On Display
	TFT_Command(CMD_DISPON_29,		0, 0);
	TFT_Delay(5);

	// Setup display parameters
	uint8_t rot[4] = {0x00, 0x40|0x20, 0x80|0x40, 0x80|0x20};
	TFT_Setup(ST7735_SCREEN_WIDTH, ST7735_SCREEN_HEIGHT, rot);
}
