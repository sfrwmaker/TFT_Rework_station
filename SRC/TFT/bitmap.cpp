/*
 * bitpam.cpp
 *
 *  Created on: May 23 2020
 *      Author: Alex
 */

#include <string.h>
#include <stdlib.h>
#include "bitmap.h"
#include "common.h"

BITMAP::BITMAP(uint16_t width, uint16_t height) {
	ds = 0;
	if (width == 0 || height == 0) return;

	uint8_t	bytes_per_row = (width+7) >> 3;
	ds = (struct data *)calloc(sizeof(struct data) + height * bytes_per_row, sizeof(uint8_t));
	if (!ds) return;
	ds->w 		= width;
	ds->h 		= height;
	ds->links 	= 1;
}

// Allocate the memory manually, in the CCM region for instance
// Please, ensure the data fits to the memory region
BITMAP::BITMAP(uint8_t* addr, uint32_t max_size, uint16_t width, uint16_t height) {
	if (addr == 0 || width == 0 || height == 0) return;
	// Check the required memory size
	uint8_t	bytes_per_row = (width+7) >> 3;
	uint32_t b_size	= sizeof(struct data) + height * bytes_per_row;
	if (b_size > max_size) {							// ccm buffer is about to overflow
		b_size = max_size - sizeof(struct data);		// Remaining bytes available for the forecast string
		b_size /= height;								// Maximum bytes per horizontal line in the forecast string
		width = b_size << 3;							// Cut-off the BITMAP width
	}
	ds = (struct data *)addr;
	ds->w 		= width;
	ds->h 		= height;
	ds->links 	= 255;									// Do not free the memory
	clear();
}

BITMAP::BITMAP(const BITMAP &bm) {
	this->ds = bm.ds;
	++ds->links;
}

BITMAP&	BITMAP::operator=(const BITMAP &bm) {
	if (this != &bm) {
		if (ds != 0 && --(this->ds->links) == 0) {
			free(ds);
		}
		this->ds = bm.ds;
		++ds->links;
	}
	return *this;
}

BITMAP::~BITMAP(void) {
	if (!ds) return;
	if (--ds->links > 0) return;						// Destroy yet another copy of the bitmap
	free(ds);
	ds = 0;
}

uint32_t BITMAP::totalSize(void) {
	if (!ds) return 0;
	uint8_t	bytes_per_row = (ds->w+7) >> 3;
	return sizeof(struct data) + ds->h * bytes_per_row;
}

void BITMAP::clear(void) {
	if (!ds) return;
	uint8_t  bytes_per_row = (ds->w + 7) >> 3;
	uint32_t size = bytes_per_row * ds->h;
	TFT_BM_Clear(ds->data, size);
}

void BITMAP::drawPixel(uint16_t x, uint16_t y) {
	if (!ds) return;
	TFT_BM_DrawPixel(ds->data, ds->w, ds->h, x, y);
}

bool BITMAP::pixel(uint16_t x, uint16_t y) {
	if (!ds) return false;
	return TFT_BM_Pixel(ds->data, ds->w, ds->h, x, y) != 0;
}

void BITMAP::drawHLine(uint16_t x, uint16_t y, uint16_t length) {
	if (!ds) return;
	TFT_BM_DrawHLine(ds->data, ds->w, ds->h, x, y, length);
}

void BITMAP::drawVLine(uint16_t x, uint16_t y, uint16_t length) {
	if (!ds) return;
	TFT_BM_DrawVLine(ds->data, ds->w, ds->h, x, y, length);
}

void BITMAP::drawIcon(uint16_t x, uint16_t y, const uint8_t *icon, uint16_t ic_width, uint16_t ic_height) {
	if (!ds || !icon) return;
	TFT_BM_JoinIcon(ds->data, ds->w, ds->h, x, y, icon, ic_width, ic_height);
}

void BITMAP::drawVGauge(uint16_t gauge, bool edged) {
	if (!ds) return;
	TFT_BM_DrawVGauge(ds->data, ds->w, ds->h, gauge, edged);
}

void BITMAP::draw(uint16_t x, uint16_t y, uint16_t bg_color, uint16_t fg_color) {
	TFT_DrawBitmap(x, y, ds->w, ds->h, ds->data, ds->w, bg_color, fg_color);
}

void BITMAP::drawInsideArea(uint16_t x, uint16_t y, uint16_t area_width, uint16_t area_height, uint16_t bg_color, uint16_t fg_color) {
	TFT_DrawBitmap(x, y, area_width, area_height, ds->data, ds->w, bg_color, fg_color);
}

void BITMAP::scroll(uint16_t x, uint16_t y, uint16_t area_width, int16_t offset, uint8_t gap, uint16_t bg_color, uint16_t fg_color) {
	TFT_DrawScrolledBitmap(x, y, area_width, ds->h, ds->data, ds->w, offset, gap, bg_color, fg_color);
}
