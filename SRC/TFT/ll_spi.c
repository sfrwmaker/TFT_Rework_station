/*
 * low_level.c
 *
 *  Created on: Nov 16 2020
 *      Author: Alex
 */
#include "ll_spi.h"

#ifdef TFT_SPI_PORT

#define BURST_HALF_SIZE 		(768)			// Divided by 2 and 3 for any pixel format: 3 bytes or 2 bytes per pixel
static uint8_t	buff[BURST_HALF_SIZE*2];		// Buffer to be send via SPI
static uint16_t index = 0;						// Buffer index to put new data

// Activates process of sending a command to the display, chip select goes to low state
static void TFT_SPI_Select(void) {
	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

// Deactivates process of sending/receiving data to the display, chip select goes high state
static void TFT_SPI_Unselect(void) {
	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
}

// Hard reset pin management
static void TFT_SPI_RST(GPIO_PinState state) {
	HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, state);
}

// Activates command mode: CS and DC pins should share the same GPIO port!
static void TFT_SPI_COMMAND_MODE(void) {
	// If CS and DC pins are sharing the same port
//	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin | TFT_DC_Pin, GPIO_PIN_RESET);
	// Otherwise manage the pins separately
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

// HARDWARE RESET
void TFT_SPI_Reset(void) {
	TFT_SPI_RST(GPIO_PIN_RESET);
	TFT_Delay(200);
	TFT_SPI_Select();
	TFT_Delay(200);
	TFT_SPI_Unselect();
	TFT_SPI_RST(GPIO_PIN_SET);
}

// Activates data mode
void TFT_SPI_DATA_MODE(void) {
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
}

// Send command (byte) to the Display followed by the command arguments
void TFT_SPI_Command(uint8_t cmd, const uint8_t* buff, size_t buff_size) {
	TFT_SPI_COMMAND_MODE();
	HAL_SPI_Transmit(&TFT_SPI_PORT, &cmd, 1, 10);
	TFT_SPI_Unselect();
	if (buff && buff_size > 0) {
		TFT_SPI_DATA_MODE();
		while (buff_size > 0) {
			uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
			HAL_SPI_Transmit(&TFT_SPI_PORT, (uint8_t *)buff, chunk_size, HAL_MAX_DELAY);
			buff 		+= chunk_size;
			buff_size 	-= chunk_size;
		}
		TFT_SPI_Unselect();
	}
}

bool TFT_SPI_ReadData(uint8_t cmd, uint8_t *data, uint16_t size) {
	if (!data || size == 0) return 0;

	TFT_SPI_COMMAND_MODE();
	HAL_SPI_Transmit(&TFT_SPI_PORT, &cmd, 1, 10);
	bool ret = (HAL_OK == HAL_SPI_Receive(&TFT_SPI_PORT, data, size, 100));
	TFT_SPI_Unselect();
	return ret;
}

// Last section of this file dedicated to send block of color(s) to the display via SPI bus

#ifdef TFT_USE_DMA
/*
 * Send color buffer with DMA support enabled via SPI bus
 * Split buff into two pieces but use normal DMA buffer
 * Each half-buffer sending separately using DMA
 * Active sending half-buffer indexed by buff_sending variable
 * 0 or 1 means low or upper half-buffer is sending via DMA, 2 means no active buffer sending
 * buff_border variable indicating the top limit of the current half-buffer
 * that is filling by new data to be sent.
 * When low half-buffer is filling, the buff_border is equal to BURST_HALF_SIZE,
 * when upper half-buffer if fiiling, the buff_border is equal to BURST_HALF_SIZE*2
 */

static volatile uint8_t 	buff_sending	= 2;// Index of the sending via DMA buffer (0 or 1). If Greater than 1, DMA is in idle state.
static uint16_t	buff_border					= BURST_HALF_SIZE; // We can write data to the buffer until this boundary

// Complete buffer sent callback procedure
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
		buff_sending = 2;						// DMA buffer sending is complete
}

static uint8_t waitHalfBuffer(uint32_t to) {
	uint32_t finish_ms = HAL_GetTick() + to;
	while (HAL_GetTick() < finish_ms) {
		if (buff_sending > 1)					// The half-buffer was successfully sent
			return 1;
	}
	return 0;
}

// Prepare to send new data block
void TFT_SPI_ColorBlockInit(void) {
	if (buff_sending <= 1) {					// There is an active DMA transfer
		waitHalfBuffer(2000);
		HAL_SPI_DMAStop(&TFT_SPI_PORT);
	}
	buff_border		= BURST_HALF_SIZE;
	index			= 0;
	buff_sending	= 2;
}

void TFT_SPI_ColorBlockSend_18bits(uint16_t color, uint32_t size) {
	// Convert to 18-bits color
	uint8_t r = (color & 0xF800) >> 8;
	uint8_t g = (color & 0x7E0)  >> 3;
	uint8_t b = (color & 0x1F)   << 3;
	for (uint32_t i = 0; i < size; ++i) {
		buff[index++] = r;
		buff[index++] = g;
		buff[index++] = b;
		if (index >= buff_border) {				// The half-buffer filled completely
			if (0 == waitHalfBuffer(2000)) {	// Timed out
				TFT_SPI_ColorBlockInit();
				return;							// Transfer failed
			}
			if (buff_border <= BURST_HALF_SIZE) {
				buff_border <<= 1;				// BURST_HALF_SIZE*2
				buff_sending = 0;				// First half buffer start sending data
				HAL_SPI_Transmit_DMA(&TFT_SPI_PORT, (uint8_t *)buff, BURST_HALF_SIZE);
			} else {
				buff_border = BURST_HALF_SIZE;
				index = 0;
				buff_sending = 1;				// Second half buffer start sending data
				HAL_SPI_Transmit_DMA(&TFT_SPI_PORT, (uint8_t *)(buff+BURST_HALF_SIZE), BURST_HALF_SIZE);
			}
		}
	}
}

void TFT_SPI_ColorBlockSend_16bits(uint16_t color, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i) {
		buff[index++] = (color >> 8 ) & 0xFF;
		buff[index++] = color & 0xFF;
		if (index >= buff_border) {				// The half-buffer filled completely
			if (0 == waitHalfBuffer(2000)) {	// Timed out
				TFT_SPI_ColorBlockInit();
				return;							// Transfer failed
			}
			if (buff_border <= BURST_HALF_SIZE) {
				buff_border <<= 1;
				buff_sending = 0;				// First half buffer start sending data
				HAL_SPI_Transmit_DMA(&TFT_SPI_PORT, (uint8_t *)buff, BURST_HALF_SIZE);
			} else {
				buff_border = BURST_HALF_SIZE;
				index = 0;
				buff_sending = 1;				// Second half buffer start sending data
				HAL_SPI_Transmit_DMA(&TFT_SPI_PORT, (uint8_t *)(buff+BURST_HALF_SIZE), BURST_HALF_SIZE);
			}
		}
	}
}

//	No more data, flush the buffer
void TFT_SPI_ColorBlockFlush(void) {
	if (0 == waitHalfBuffer(2000)) {			// Timed out
		TFT_SPI_ColorBlockInit();
		return;									// Transfer failed
	}
	HAL_SPI_DMAStop(&TFT_SPI_PORT);
	// Check the 'tail' bytes are in the buffer
	uint8_t *hb = buff;
	uint16_t bytes_to_send = index;
	if (buff_border > BURST_HALF_SIZE) {
		bytes_to_send -= BURST_HALF_SIZE;
		hb += BURST_HALF_SIZE;
	}
	if (bytes_to_send > 0) {
		buff_sending = 0;
		HAL_SPI_Transmit_DMA(&TFT_SPI_PORT, hb, bytes_to_send);
		waitHalfBuffer(2000);
	}
	TFT_SPI_ColorBlockInit();
	TFT_SPI_Unselect();
}


#else 			// TFT_USE_DMA not defined
// Send color buffer without DMA support via SPI bus

void TFT_SPI_ColorBlockSend_18bits(uint16_t color, uint32_t size) {
	// Convert to 18-bits color
	uint8_t r = (color & 0xF800) >> 8;
	uint8_t g = (color & 0x7E0)  >> 3;
	uint8_t b = (color & 0x1F)   << 3;
	for (uint32_t i = 0; i < size; ++i) {
		buff[index++] = r;
		buff[index++] = g;
		buff[index++] = b;

		if (index >= BURST_HALF_SIZE*2) {
			HAL_SPI_Transmit(&TFT_SPI_PORT, (uint8_t *)buff, BURST_HALF_SIZE*2, 100);
			index = 0;
		}
	}
}

void TFT_SPI_ColorBlockSend_16bits(uint16_t color, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i) {
		buff[index++] = (color >> 8) & 0xFF;
		buff[index++] = color & 0xFF;

		if (index >= BURST_HALF_SIZE*2) {
			HAL_SPI_Transmit(&TFT_SPI_PORT, (uint8_t *)buff, BURST_HALF_SIZE*2, 100);
			index = 0;
		}
	}
}

void TFT_SPI_ColorBlockFlush(void) {
	if (index > 0)
		HAL_SPI_Transmit(&TFT_SPI_PORT, (uint8_t *)buff, index, 100);
	index = 0;
	TFT_SPI_Unselect();
}

void TFT_SPI_ColorBlockInit(void) { }					// Not extra initialization required without DMA support

#endif			// TFT_USE_DMA

#endif			// TFT_SPI_PORT
