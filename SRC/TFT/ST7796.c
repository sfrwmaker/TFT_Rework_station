/*
 * ILI9341.c
 *
 *  Created on: May 20, 2020
 *      Author: Alex
 */

#include <stdint.h>
#include "ST7796.h"
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
void ST7796_Init(void) {
	// Initialize display interface. By default the library guess the display interface
	TFT_InterfaceSetup(TFT_16bits, 0);
	// Reset display hardware
	TFT_DEF_Reset();
	// SOFTWARE RESET
	TFT_Command(CMD_SWRESET_01,	0, 0);
	TFT_Delay(1000);
	// Exit sleep mode
	TFT_DEF_SleepOut();
	
	// Command Set control: Enable extension command 2 partI
	TFT_Command(CMD_CSCON_F0,		(uint8_t *)"\xC3", 1);
	// Command Set control: Enable extension command 2 partII
	TFT_Command(CMD_CSCON_F0,		(uint8_t *)"\x96", 1);
	// Memory access mode: X-Mirror, Top-Left to right-Buttom, RGB
	TFT_Command(CMD_MADCTL_36,		(uint8_t *)"\x48", 1);
	// Interface Pixel Format: 16 bits
	TFT_Command(CMD_PIXSET_3A,		(uint8_t *)"\x55", 1);
	//Display Inversion Control: 1-dot inversion
	TFT_Command(CMD_INVTR_B4,		(uint8_t *)"\x01", 1);
	// Display function control: Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2; LCD Drive Line=8*(59+1)
	TFT_Command(CMD_DFC_B6,			(uint8_t *)"\x80\x02\x3B", 3);
	// Display Output Ctrl Adjust: Source eqaulizing period time= 22.5 us; Timing for "Gate start"=25; Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
	TFT_Command(CMD_DOCA_E8,		(uint8_t *)"\x40\x8A\x00\x00\x29\x19\xA5\x33", 8);
	// Power Control 2: VGH, VGL
	TFT_Command(CMD_PWCTRL2_C1,		(uint8_t *)"\x06", 1);
	TFT_Delay(5);
	// Power Control 3
	TFT_Command(CMD_PWCTRL3_C2,		(uint8_t *)"\xA7", 1);
	// Power Control 3: Vcom 0.9
	TFT_Command(CMD_VMCTRL1_C5,		(uint8_t *)"\x18", 1);
	TFT_Delay(120);
	
	// Gamma"+"
	TFT_Command(CMD_PGAMCTRL_E0,	(uint8_t *)"\xF0\x09\x0B\x06\x04\x15\x2F\x54\x42\x3C\x17\x14\x16\x1B", 14);
	// Gamma"-"
	TFT_Command(CMD_NGAMCTRL_E1,	(uint8_t *)"\xE0\x09\x0B\x06\x04\x03\x2B\x43\x42\x3B\x16\x14\x17\x1B", 14);
	TFT_Delay(120);
	// Command Set control: Disable extension command 2 partI
	TFT_Command(CMD_CSCON_F0,		(uint8_t *)"\x3C", 1);
	// Command Set control: Disable extension command 2 partII
	TFT_Command(CMD_CSCON_F0,		(uint8_t *)"\x69", 1);
	
	// Turn On Display
	TFT_Command(CMD_DISPON_29,		0, 0);
	TFT_Delay(5);
	
	// Setup display parameters
	uint8_t rot[4] = {0x40|0x08, 0x20|0x08, 0x80|0x08, 0x40|0x80|0x20|0x08};
	TFT_Setup(ST7796_SCREEN_WIDTH, ST7796_SCREEN_HEIGHT, rot);
}
