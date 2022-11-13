/* vim: set ai et ts=4 sw=4: */

#include "sdspi.h"

#define BLOCK_SIZE_HC	(512)

static void SD_Select() {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

static void SD_Unselect() {
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

// Wait SD card ready
static uint8_t SD_NotReady(void) {
	uint8_t busy	= 0;
	uint8_t tx		= 0xFF;
	for (uint32_t i = 0; busy != 0xFF; ++i) {
		if (HAL_OK != HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, &busy, 1, HAL_MAX_DELAY)) {
			SD_Unselect();
			return 1;
		}
		if (i >= 100000)
			return 2;										// Timed out
	}
	return 0;
}

static uint8_t SD_ReadBytes(uint8_t* buff, size_t buff_size) {
    uint8_t tx = 0xFF;
    for ( ; buff_size > 0; --buff_size) {
        if (HAL_OK != HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, buff, 1, HAL_MAX_DELAY))
        	return 1;
        ++buff;
    }
    return 0;
}

static uint32_t ext_bits(uint8_t *data, int msb, int lsb) {
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = 15 - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}

/*
R1: 0abcdefg
     ||||||`- 1th bit (g): card is in idle state
     |||||`-- 2th bit (f): erase sequence cleared
     ||||`--- 3th bit (e): illegal command detected
     |||`---- 4th bit (d): CRC check error
     ||`----- 5th bit (c): error in the sequence of erase commands
     |`------ 6th bit (b): misaligned address used in command
     `------- 7th bit (a): command argument outside allowed range
             (8th bit is always zero)
*/
static uint8_t SD_CMD(SPISD_CMD cmd, uint32_t arg) {
	static uint8_t cmd_buff[6];
	cmd_buff[0] = (uint8_t)cmd | 0x40;
	for (uint8_t i = 0; i < 4; ++i) {
		cmd_buff[4-i] = arg & 0xFF;
		arg >>= 8;
	}
	switch (cmd & 0x3F) {
		case CMD0_GO_IDLE_STATE:
			cmd_buff[5] = 0x95;								// CRC of CMD0: (0x4A << 1) | 1 - CRC 7 + end bit
			break;
		case CMD8_SEND_IF_COND:
			cmd_buff[5] = 0x87;								// CRC of CMD8(0x1AA): (0x43 << 1) | 1 - CRC7 + end bit
			break;
		default:
			cmd_buff[5] = 0xff; 							// Dummy CRC + end bit
			break;
	}

	SD_Select();											// Make sure SD card is selected
	if (SD_NotReady()) return 0;

	// Transmit the command
	HAL_SPI_Transmit(&SD_SPI_PORT, cmd_buff, sizeof(cmd_buff), HAL_MAX_DELAY);

	// Receive the answer
	uint8_t r1		= 0x80;
	uint8_t tx		= 0xFF;
	for (uint8_t i = 0; i < 10; ++i ) {
        HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, &r1, sizeof(r1), HAL_MAX_DELAY);
        if ((r1 & 0x80) == 0)	break;						// When 8th bit is zero, r1 received
    }
    return r1;
}

static uint8_t SD_DATA_CMD(SPISD_CMD cmd, uint32_t arg, uint8_t token, uint8_t *data, uint16_t size) {
	uint8_t crc[2];

	if (0x00 != SD_CMD(cmd, arg)) {
	    SD_Unselect();
	    return 1;
	}

    uint8_t fb;
    uint8_t tx = 0xFF;
    while (1) {
        HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, &fb, sizeof(fb), HAL_MAX_DELAY);
        if (fb == token)
            break;
        if (fb != 0xFF) {
        	SD_Unselect();
            return 2;
        }
    }

    if (SD_ReadBytes(data, size) != 0) {
        SD_Unselect();
        return 3;
    }

    if (SD_ReadBytes(crc, sizeof(crc)) != 0) {
        SD_Unselect();
        return 4;
    }

    SD_Unselect();
    return 0;
}

static void SD_GetParameters(SDCARD *sd) {
	uint8_t csd[16];

	if (0 > SD_DATA_CMD(CMD9_SEND_CSD, 0, 0xFE, csd, sizeof(csd))) {
		sd->type		= TYPE_NOT_READY;
		sd->blocks		= 0;
		sd->erase_size	= 0;
		return;
	}

    uint8_t csd_structure = ext_bits(csd, 127, 126);		// csd_structure : csd[127:126]
    switch (csd_structure) {
        case 0:
          	if (sd->type == TYPE_SDSC) {
          		uint32_t c_size		= ext_bits(csd, 73, 62); // c_size        : csd[73:62]
          		uint8_t	 c_size_m	= ext_bits(csd, 49, 47); // c_size_mult   : csd[49:47]
           		uint8_t read_bl_len	= ext_bits(csd, 83, 80); // read_bl_len   : csd[83:80] - the *maximum* read block length
           		uint32_t block_len 	= 1 << read_bl_len;  	// BLOCK_LEN = 2^READ_BL_LEN
           		uint32_t mult		= 1 << (c_size_m + 2); 	// MULT = 2^C_SIZE_MULT+2 (C_SIZE_MULT < 8)
           		uint32_t blocknr	= (c_size + 1) * mult;	// BLOCKNR = (C_SIZE+1) * MULT
           		uint32_t capacity	= blocknr * block_len;	// memory capacity = BLOCKNR * BLOCK_LEN
           		sd->blocks			= capacity / BLOCK_SIZE_HC;

           		// ERASE_BLK_EN = 1: Erase in multiple of 512 bytes supported
           		if (ext_bits(csd, 46, 46)) {
           			sd->erase_size = BLOCK_SIZE_HC;
           		} else {
           			// ERASE_BLK_EN = 1: Erase in multiple of SECTOR_SIZE supported
           			sd->erase_size = BLOCK_SIZE_HC * (ext_bits(csd, 45, 39) + 1);
           		}
           	} else {
           		sd->type = TYPE_NOT_READY;
           	}
          	break;
        case 1:
            if (sd->type == TYPE_SDHC) {
            	uint32_t hc_c_size	= ext_bits(csd, 69, 48); // device size : C_SIZE : [69:48]
            	sd->blocks	= (hc_c_size + 1) << 10;	// block count = C_SIZE+1) * 1K byte (512B is block size)
            	// ERASE_BLK_EN is fixed to 1, which means host can erase one or multiple of 512 bytes.
            	sd->erase_size = BLOCK_SIZE_HC;
            } else {
            	sd->type = TYPE_NOT_READY;
            }
            break;
        default:
          	sd->type = TYPE_NOT_READY;
           	break;
    };
}

uint8_t SD_Init(SDCARD *sd) {
	sd->type		= TYPE_NOT_READY;
	sd->blocks		= 0;
	sd->erase_size	= 0;
	sd->init_status = 12;


	uint32_t spi_speed = SD_SPI_PORT.Init.BaudRatePrescaler;
	SD_SPI_PORT.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; // 164 kbbs
	HAL_SPI_Init(&SD_SPI_PORT);
    SD_Unselect();
    uint8_t high = 0xFF;
    for (uint8_t i = 0; i < 10; ++i) {
        HAL_SPI_Transmit(&SD_SPI_PORT, &high, sizeof(high), HAL_MAX_DELAY);
    }
    SD_SPI_PORT.Init.BaudRatePrescaler = spi_speed;
    HAL_SPI_Init(&SD_SPI_PORT);

    if (0x01 != SD_CMD(CMD0_GO_IDLE_STATE, 0x0)) {
    	SD_Unselect();
    	sd->init_status = 1;
    	return sd->init_status;
    }

    if (0x01 != SD_CMD(CMD8_SEND_IF_COND, 0x1AA)) {
        SD_Unselect();
        sd->init_status = 2;
        return sd->init_status;								// not an SDHC/SDXC/SDSC card
    }
    // Read R7 trailing 32-bits data from SD card
    uint8_t resp[4];
    if (SD_ReadBytes(resp, sizeof(resp)) != 0) {
    	SD_Unselect();
    	sd->init_status = 3;
        return sd->init_status;
    }

    if (((resp[2] & 0x01) != 1) || (resp[3] != 0xAA)) {
    	SD_Unselect();
    	sd->init_status = 4;
        return sd->init_status;
    }

    for (uint32_t i = 0; ; ++i) {
        if (0x01 != SD_CMD(CMD55_APP_CMD, 0)) {
            SD_Unselect();
            sd->init_status = 5;
            return sd->init_status;
        }

        uint8_t r1 = SD_CMD(ACMD41_APP_SEND_OP_COND, 0x40000000);
        if (r1 == 0)
        	break;

        if (r1 != 0x01){
            SD_Unselect();
            sd->init_status = 6;
            return sd->init_status;
        }

        HAL_Delay(1);
        if (i > 30000) {
        	sd->init_status = 11;
        	return sd->init_status;							// Timed out
        }
    }

    if (0x00 != SD_CMD(CMD58_READ_OCR, 0)) {
        SD_Unselect();
        sd->init_status = 7;
        return sd->init_status;
    }
    if (SD_ReadBytes(resp, sizeof(resp)) != 0) {
    	SD_Unselect();
    	sd->init_status = 8;
        return sd->init_status;
    }

    sd->type = TYPE_SDHC;
    if ((resp[0] & 0xC0) != 0xC0) {							// SDSC card
    	sd->type = TYPE_SDSC;
    	if (0x00 != SD_CMD(CMD16_SET_BLOCKLEN, BLOCK_SIZE_HC)) {
    		SD_Unselect();
    		sd->init_status = 9;
    		return sd->init_status;
    	}
    }

    SD_GetParameters(sd);
    SD_Unselect();

    if (sd->type == TYPE_NOT_READY) {
    	sd->init_status = 10;
    } else {
    	sd->init_status = 0;
    }
    return sd->init_status;
}

uint8_t	SD_Read(SDCARD *sd, uint32_t start_block, uint8_t count, uint8_t* data) {
	if (count == 1) {
		return SD_ReadSingleBlock(sd, start_block, data);
	}
	return SD_ReadBlocks(sd, start_block, count, data);
}

uint8_t	SD_Write(SDCARD *sd, uint32_t start_block, uint8_t count, const uint8_t* data) {
	if (count == 1) {
		return SD_WriteSingleBlock(sd, start_block, data);
	}
	return SD_WriteSBlocks(sd, start_block, count, data);
}

uint8_t SD_ReadSingleBlock(SDCARD *sd, uint32_t block_num, uint8_t* data) {
	if (sd->type == TYPE_NOT_READY)
		return 1;
	if (sd->blocks <= block_num)
		return 2;
	if (sd->type == TYPE_SDSC)
		block_num *= BLOCK_SIZE_HC;

    if (0 != SD_DATA_CMD(CMD17_READ_SINGLE_BLOCK, block_num, 0xFE, data, BLOCK_SIZE_HC)) {
    	SD_Unselect();
    	return 3;
    }
    SD_Unselect();
    return 0;
}


uint8_t SD_WriteSingleBlock(SDCARD *sd, uint32_t block_num, const uint8_t* data) {
	if (sd->type == TYPE_NOT_READY)
		return 1;
	if (sd->blocks <= block_num)
		return 2;
	if (sd->type == TYPE_SDSC)
		block_num *= BLOCK_SIZE_HC;

	if (0 != SD_CMD(CMD24_WRITE_BLOCK, block_num)) {
		SD_Unselect();
		return 3;
	}

    uint8_t data_token = 0xFE;								// Data start token
    uint8_t crc[2] = { 0xFF, 0xFF };
    HAL_SPI_Transmit(&SD_SPI_PORT, &data_token, sizeof(data_token), HAL_MAX_DELAY);
    HAL_SPI_Transmit(&SD_SPI_PORT, (uint8_t*)data, BLOCK_SIZE_HC, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&SD_SPI_PORT, crc, sizeof(crc), HAL_MAX_DELAY);

    /*
        dataResp:
        xxx0abc1
            010 - Data accepted
            101 - Data rejected due to CRC error
            110 - Data rejected due to write error
    */
    uint8_t data_resp;
    SD_ReadBytes(&data_resp, sizeof(data_resp));
    if ((data_resp & 0x1F) != 0x05) {						// data rejected
        SD_Unselect();
        return 4;
    }

    if (SD_NotReady()) return 5;

    SD_Unselect();
    return 0;
}

uint8_t SD_ReadBlocks(SDCARD *sd, uint32_t start_block, uint8_t count, uint8_t* data) {
	uint8_t crc[2];

	if (count == 0)
		return 0;
	if (sd->type == TYPE_NOT_READY)
		return 1;
	if (sd->blocks <= start_block)
		return 2;
	if (sd->blocks < start_block + count) {
		count = sd->blocks - start_block;
	}
	if (sd->type == TYPE_SDSC)
		start_block *= BLOCK_SIZE_HC;

	// Initialize multiple blocks reading
	if (0x00 != SD_CMD(CMD18_READ_MULTIPLE_BLOCK, start_block)) {
		SD_Unselect();
		return 3;
	}

	// Read data stream by blocks (512 bytes)
	for (uint8_t i = 0; i < count; ++i) {
		// Each data packet starts with 'start token', 0xFE
		uint8_t fb;
		uint8_t tx = 0xFF;
		while (1) {
			HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, &fb, sizeof(fb), HAL_MAX_DELAY);
			if (fb == 0xFE)
				break;
			if (fb != 0xFF) {
				SD_Unselect();
				return 4;
			}
		}

		// Read data packet
		if (SD_ReadBytes(data+i*BLOCK_SIZE_HC, BLOCK_SIZE_HC) != 0) {
			SD_Unselect();
			return 5;
		}
		// Read CRC data
		if (SD_ReadBytes(crc, sizeof(crc)) != 0) {
			SD_Unselect();
			return 6;
		}
	}

	// End of the data, stop transmission
    static const uint8_t cmd[] = { CMD12_STOP_TRANSMISSION | 0x40 , 0x00, 0x00, 0x00, 0x00 , 0xFF };
    HAL_SPI_Transmit(&SD_SPI_PORT, (uint8_t*)cmd, sizeof(cmd), HAL_MAX_DELAY);

    // Ignore staff byte
    uint8_t stuff_byte;
    if (SD_ReadBytes(&stuff_byte, sizeof(stuff_byte)) < 0) {
        SD_Unselect();
        return 7;
    }
	// Receive the answer
	uint8_t r1		= 0x80;
	uint8_t tx		= 0xFF;
	for (uint8_t i = 0; i < 10; ++i ) {
        HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, &r1, sizeof(r1), HAL_MAX_DELAY);
        if ((r1 & 0x80) == 0)	break;						// When 8th bit is zero, r1 received
    }
    // Check r1 status
	if (r1 != 0) {
		SD_Unselect();
		return 8;
	}

	// End of multiple blocks reading
    SD_Unselect();
    return 0;
}

uint8_t SD_WriteSBlocks(SDCARD *sd, uint32_t start_block, uint8_t count, const uint8_t* data) {
	if (count == 0)
		return 0;
	if (sd->type == TYPE_NOT_READY)
		return 1;
	if (sd->blocks <= start_block)
		return 2;
	if (sd->blocks < start_block + count) {
		count = sd->blocks - start_block;
	}
	if (sd->type == TYPE_SDSC)
		count *= BLOCK_SIZE_HC;

	if (0 != SD_CMD(CMD25_WRITE_MULTIPLE_BLOCK, start_block)) {
		SD_Unselect();
		return 3;
	}

	// Write data stream by blocks (512 bytes)
	for (uint8_t i = 0; i < count; ++i) {
		// First send starting token, then data buffer and CRC
		uint8_t data_token = 0xFC;							// Data start token
		uint8_t crc[2] = { 0xFF, 0xFF };
		HAL_SPI_Transmit(&SD_SPI_PORT, &data_token, sizeof(data_token), HAL_MAX_DELAY);
		HAL_SPI_Transmit(&SD_SPI_PORT, (uint8_t *)data+i*BLOCK_SIZE_HC, BLOCK_SIZE_HC, HAL_MAX_DELAY);
		HAL_SPI_Transmit(&SD_SPI_PORT, crc, sizeof(crc), HAL_MAX_DELAY);

		/*
        	dataResp:
        	xxx0abc1
            	010 - Data accepted
            	101 - Data rejected due to CRC error
            	110 - Data rejected due to write error
		 */
		uint8_t data_resp;
		SD_ReadBytes(&data_resp, sizeof(data_resp));
		if ((data_resp & 0x1F) != 0x05) {					// data rejected
			SD_Unselect();
			return 4;
		}

		if (SD_NotReady()) return 5;
	}

	// Stop sending data
    uint8_t stop_tran = 0xFD;								// stop transaction token for CMD25
    HAL_SPI_Transmit(&SD_SPI_PORT, &stop_tran, sizeof(stop_tran), HAL_MAX_DELAY);

    // skip one byte before readyng "busy"
    // this is required by the spec and is necessary for some real SD-cards!
    uint8_t skip_byte;
    SD_ReadBytes(&skip_byte, sizeof(skip_byte));

	// Wait SD card ready
	uint8_t busy	= 0;
    uint8_t tx		= 0xFF;
	do {
		if (HAL_OK != HAL_SPI_TransmitReceive(&SD_SPI_PORT, &tx, &busy, 1, HAL_MAX_DELAY)) {
			SD_Unselect();
			return 6;
		}
	} while (busy != 0xFF);

    SD_Unselect();
    return 0;
}
