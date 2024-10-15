/*
 * ILI9341.c
 *
 *  Created on: May 20, 2020
 *      Author: Alex
 *
 *  2024 AUG 02
 *  	Added ILI9341v support, i.e. ILI9341v_Init() function
 */

#include "ILI9341.h"

typedef enum {
	CMD_NOP_00			= 0x00,
	CMD_SWRESET_01		= 0x01,
	CMD_RDDIDIF_04		= 0x04,
	CMD_RDDST_09		= 0x09,
	CMD_RDDPM_0A		= 0x0A,
	CMD_RDDMADCTL_0B	= 0x0B,
	CMD_RDDCOLMOD_0C	= 0x0C,
	CMD_RDDIM_0D		= 0x0D,
	CMD_RDDSM_0E		= 0x0E,
	CMD_RDDSDR_OF		= 0x0F,
	CMD_SPLIN_10		= 0x10,
	CMD_SLPOUT_11		= 0x11,
	CMD_PTLON_12		= 0x12,
	CMD_NORON_13		= 0x13,
	CMD_DINVOFF_20		= 0x20,
	CMD_DINVON_21		= 0x21,
	CMD_GAMSET_26		= 0x26,
	CMD_DISPOFF_28		= 0x28,
	CMD_DISPON_29		= 0x29,
	CMD_CASET_2A		= 0x2A,
	CMD_PASET_2B		= 0x2B,
	CMD_RAMWR_2C		= 0x2C,
	CMD_RGBSET_2D		= 0x2D,
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
	CMD_WRCABC_5E		= 0x5E,
	CMD_RDCABC_5F		= 0x5F,
	CMD_XX_80			= 0x80,
	CMD_FRMCTR1_B1		= 0xB1,
	CMD_FRMCTR2_B2		= 0xB2,
	CMD_FRMCTR3_B3		= 0xB3,
	CMD_INVTR_B4		= 0xB4,
	CMD_PRCTR_B5		= 0xB5,
	CMD_DISCTRL_B6		= 0xB6,
	CMD_ETMOD_B7		= 0xB7,
	CMD_BLCTRL1_B8		= 0xB8,
	CMD_BLCTRL2_B9		= 0xB9,
	CMD_BLCTRL3_BA		= 0xBA,
	CMD_BLCTRL4_BB		= 0xBB,
	CMD_BLCTRL5_BC		= 0xBC,
	CMD_BLCTRL7_BE		= 0xBE,
	CMD_BLCTRL8_BF		= 0xBF,
	CMD_PWRCTRL1_C0		= 0xC0,
	CMD_PWRCTRL2_C1		= 0xC1,
	CMD_VCOMCTRL1_C5	= 0xC5,
	CMD_VCOMCTRL2_C7	= 0xC7,
	CMD_PWRCTRLA_CB		= 0xCB,
	CMD_PWRCTRLB_CF		= 0xCF,
	CMD_NVMWR_D0		= 0xD0,
	CMD_NVMPKEY_D1		= 0xD1,
	CMD_RDNVM_D2		= 0xD2,
	CMD_RDID4_D3		= 0xD3,
	CMD_RDID1_DA		= 0xDA,
	CMD_RDID2_DB		= 0xDB,
	CMD_RDID3_DC		= 0xDC,
	CMD_PGAMCTRL_E0		= 0xE0,
	CMD_NGAMCTRL_E1		= 0xE1,
	CMD_DGAMCTRL_E2		= 0xE2,
	CMD_DGAMCTRL_E3		= 0xE3,
	CMD_TIMCTRLA_E8		= 0xE8,
	CMD_TIMCTRLC_EA		= 0xEA,
	CMD_PWRSEC_ED		= 0xED,
	CMD_EN3G_F2			= 0xF2,
	CMD_IFCTL_F6		= 0xF6,
	CMD_PRATIOC_F7		= 0xF7
} ILI9341_CMD;

// Initialize the Display
void ILI9341_Init(void) {
	// Initialize display interface. By default the library guess the display interface
	TFT_InterfaceSetup(TFT_16bits, 0);

	// Reset display hardware
	TFT_DEF_Reset();
	// SOFTWARE RESET
	TFT_Command(CMD_SWRESET_01,		0, 0);
	TFT_Delay(1000);

	// Power Control A
	TFT_Command(0xCB,				(uint8_t *)"\x39\x2C\x00\x34\x002", 5);
	// Power Control B
	TFT_Command(0xCF,				(uint8_t *)"\x00\xC1\x30", 3);
	// Driver timing control A
	TFT_Command(0xE8,				(uint8_t *)"\x85\x00\x78", 3);
	// Driver timing control B
	TFT_Command(0xEA,				(uint8_t *)"\x00\x00", 2);
	// Power on Sequence control
	TFT_Command(0xCF,				(uint8_t *)"\x64\x03\x12\x81", 4);
	// Pump ratio control
	TFT_Command(0xF7,				(uint8_t *)"\x20", 1);
	// Power Control,VRH[5:0]
	TFT_Command(CMD_PWRCTRL1_C0,	(uint8_t *)"\x23", 1);
	// Power Control,SAP[2:0];BT[3:0]
	TFT_Command(CMD_PWRCTRL2_C1,	(uint8_t *)"\x10", 1);
	// VCOM Control 1
	TFT_Command(CMD_VCOMCTRL1_C5,	(uint8_t *)"\x3E\x28", 2);
	// VCOM Control 2
	TFT_Command(CMD_VCOMCTRL2_C7,	(uint8_t *)"\x86", 1);
	// Memory Access Control
	TFT_Command(CMD_MADCTL_36,		(uint8_t *)"\x48", 1);
	// Pixel Format Set
	TFT_Command(CMD_PIXSET_3A,		(uint8_t *)"\x55", 1);
	// Frame Rratio Control, Standard RGB Color
	TFT_Command(CMD_FRMCTR1_B1,		(uint8_t *)"\x00\x18", 2);
	// Display Function Control
	TFT_Command(CMD_DISCTRL_B6,		(uint8_t *)"\x08\x82\x27", 3);
	//3Gammf function disable
	TFT_Command(0xF2,		(uint8_t *)"\x00", 1);
	// Gamma set
	TFT_Command(CMD_GAMSET_26,		(uint8_t *)"\x01", 1);
	// Positive Gamma  Correction
	TFT_Command(CMD_PGAMCTRL_E0,	(uint8_t *)"\x0F\x31\x2B\x0C\x0E\x08\x4E\xF1\x37\x07\x10\x03\x0E\x09\x00", 15);
	// Negative Gamma  Correction
	TFT_Command(CMD_NGAMCTRL_E1,	(uint8_t *)"\x00\x0E\x14\x03\x11\x07\x31\xC1\x48\x08\x0F\x0C\x31\x36\x0F", 15);
	// Exit sleep mode
	TFT_DEF_SleepOut();

	// Turn On Display
	TFT_Command(CMD_DISPON_29,		0, 0);

	// Setup display parameters
	uint8_t rot[4] = {0x40|0x08, 0x20|0x08, 0x80|0x08, 0x40|0x80|0x20|0x08};
	TFT_Setup(ILI9341_SCREEN_WIDTH, ILI9341_SCREEN_HEIGHT, rot);
}

// Initialize the Display
void ILI9341v_Init(void) {
	// Initialize display interface. By default the library guess the display interface
	TFT_InterfaceSetup(TFT_16bits, 0);

	// Reset display hardware
	TFT_DEF_Reset();
	// SOFTWARE RESET
	TFT_Command(CMD_SWRESET_01,		0, 0);
	TFT_Delay(200);

	TFT_Command(CMD_XX_80,			(uint8_t *)"\xFA", 1);
	TFT_Command(CMD_PWRCTRLB_CF,	(uint8_t *)"\x00\xC1\x30", 3);
	TFT_Command(CMD_PWRSEC_ED,		(uint8_t *)"\x64\x03\x12\x81", 4);
	TFT_Command(CMD_TIMCTRLA_E8,	(uint8_t *)"\x85\x00\x78", 3);
	TFT_Command(CMD_PWRCTRLA_CB,	(uint8_t *)"\x39\x2C\x00\x34\x02", 5);
	TFT_Command(CMD_PRATIOC_F7,		(uint8_t *)"\x20", 1);
	TFT_Command(CMD_TIMCTRLC_EA,	(uint8_t *)"\x00\x00", 2);
	TFT_Command(CMD_PWRCTRL1_C0,	(uint8_t *)"\x13", 1);
	TFT_Command(CMD_PWRCTRL2_C1,	(uint8_t *)"\x13", 1);
	TFT_Command(CMD_VCOMCTRL1_C5,	(uint8_t *)"\x22\x35", 2);
	TFT_Command(CMD_VCOMCTRL2_C7,	(uint8_t *)"\xBD", 1);

	TFT_Command(CMD_MADCTL_36,		(uint8_t *)"\x28", 1);			// LCD_CMD_MV_BIT | 0x08
	TFT_Command(CMD_PIXSET_3A,		(uint8_t *)"\x55", 1);			// INTERFACE PIXEL FORMAT: 0x66 -> 18 bit; 0x55 -> 16 bit
	TFT_Command(CMD_IFCTL_F6,		(uint8_t *)"\x01\x30", 2);
	TFT_Command(CMD_FRMCTR1_B1,		(uint8_t *)"\x00\x1B", 2);
	TFT_Command(CMD_EN3G_F2,		(uint8_t *)"\x00", 1);
	TFT_Command(CMD_GAMSET_26,		(uint8_t *)"\x01", 1);
	// Positive Gamma  Correction
	TFT_Command(CMD_PGAMCTRL_E0,	(uint8_t *)"\x0F\x35\x31\x0B\x0E\x06\x49\xA7\x33\x07\x0F\x03\x0C\x0A\x00", 15);
	// Negative Gamma  Correction
	TFT_Command(CMD_NGAMCTRL_E1,	(uint8_t *)"\x00\x0A\x0F\x04\x11\x08\x36\x58\x4D\x07\x10\x0C\x32\x34\x0F", 15);

	TFT_Command(CMD_ETMOD_B7,		(uint8_t *)"\x07", 1);
	TFT_Command(CMD_DISCTRL_B6,		(uint8_t *)"\x08\x82\x27", 3);
	TFT_DEF_SleepOut();
	TFT_Command(CMD_DINVON_21,		0, 0);
	// Turn On Display
	TFT_Command(CMD_DISPON_29,		0, 0);

	// Setup display parameters
	uint8_t rot[4] = {0x40|0x08, 0x20|0x08, 0x80|0x08, 0x40|0x80|0x20|0x08};
	TFT_Setup(ILI9341_SCREEN_WIDTH, ILI9341_SCREEN_HEIGHT, rot);
}

