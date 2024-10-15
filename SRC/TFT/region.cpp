/*
 * region.cpp
 *
 *  Created on: 2023 MAR 12
 *      Author: Alex
 */
#ifdef TFT_BMP_JPEG_ENABLE

#include "region.h"
#include "ff.h"
#include "picture.h"


REGION::REGION(uint32_t size) {
	if (size == 0) return;
	// Allocate memory to store whole data structure and the pixmap data
	ds = (struct r_data *)malloc(sizeof(struct r_data) + size * 2); // Size is the number of 16-bits words
	if (!ds) return;
	ds->w 		= 1;							// Non-zero means the data allocated successfully
	ds->h 		= 1;
	ds->links 	= 1;
	ds->size 	= size;							// Size is the number of 16-bits words
}

REGION::REGION(uint16_t width, uint16_t height) {
	if (width == 0 || height == 0) return;

	// Allocate memory to store whole data structure and the pixmap data
	ds = (struct r_data *)calloc(sizeof(struct r_data) + width * height, sizeof(uint16_t));
	if (!ds) return;
	ds->w 		= width;
	ds->h 		= height;
	ds->links 	= 1;
	ds->size	= (uint32_t)height * width;		// Size is the number of 16-bits words
}

// Allocate the memory manually, in the CCM region for instance
REGION::REGION(uint8_t* addr, uint16_t width, uint16_t height) {
	if (addr == 0 || width == 0 || height == 0) return;
	ds = (struct r_data *)addr;
	ds->w 		= width;
	ds->h 		= height;
	ds->links 	= 255;							// Do not free the memory
	ds->size	= (uint32_t)height * width;		// Size is the number of 16-bits words
}

REGION::REGION(const REGION &rn) {
	this->ds = rn.ds;
	++ds->links;
}

REGION&	REGION::operator=(const REGION &rg) {
	if (this != &rg) {
		if (ds != 0 && --(ds->links) == 0) {
			free(ds);
		}
		this->ds = rg.ds;
		++ds->links;
	}
	return *this;
}

REGION::~REGION(void) {
	if (!ds) return;
	if (--ds->links > 0) return;				// Destroy yet another copy of the region
	free(ds);
	ds = 0;
}

uint32_t REGION::totalSize(void) {
	if (!ds) return 0;
	return sizeof(struct r_data) + ds->h * ds->w * 2;
}

void REGION::clear(void) {
	if (!ds) return;
	for (uint32_t i = 0; i < ds->size; ++i)
		ds->data[i] = 0;
}

// Setup picture size inside the region, cannot be greater than actual size
bool REGION::setSize(uint16_t width, uint16_t height) {
	if (!ds) return false;
	if (ds->size < (uint32_t)width * height) return false;
	ds->w 	= width;
	ds->h	= height;
	return true;
}

void REGION::draw(uint16_t x, uint16_t y) {
	if (!ds) return;
	TFT_DrawRegion(x, y, ds->data, ds->w, ds->h);
}

void REGION::fill(uint16_t color) {
	if (!ds) return;
	for (uint32_t i = 0; i < (ds->w * ds->h); ++i)
		ds->data[i] = color;
}

void REGION::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
	if (!ds) return;
	if (x >= ds->w || y >= ds->h) return;
	uint32_t pos 	= ds->w * y + x;			// pixel position in the data array
	ds->data[pos]	= color;
}

uint16_t REGION::pixel(uint16_t x, uint16_t y) {
	if (!ds) return 0;
	if (x >= ds->w || y >= ds->h) return 0;
	uint32_t pos = ds->w * y + x;				// pixel position in the data array
	return ds->data[pos];
}

void REGION::drawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	if (!ds) return;
	if (x >= ds->w || y >= ds->h) return;
	if (x + length >= ds->w)
		length = ds->w - x - 1;
	uint32_t pos = ds->w * y + x;
	for (uint16_t i = 0; i < length; ++i)
		ds->data[pos+i] = color;
}

void REGION::drawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	if (!ds) return;
	if (x >= ds->w || y >= ds->h) return;
	if (y + length >= ds->h)
		length = ds->h - y - 1;
	uint32_t pos = ds->w * y + x;
	for (uint16_t i = 0; i < length; ++i)
		ds->data[pos + i*ds->w] = color;
}

bool REGION::loadBMP(const char *filename, int16_t bmp_x, int16_t bmp_y, uint16_t transparent, bool use_transparent) {
	if (!ds) return false;
	return TFT_LoadBMP(ds->data, ds->w, ds->h, filename, bmp_x, bmp_y, transparent, use_transparent);
}

void REGION::scrollBitmap(uint16_t x, uint16_t y, uint16_t y_offset, BITMAP &bm, int16_t offset, uint8_t gap, uint16_t color) {
	if (!ds) return;
	TFT_ScrollBitmapOverRegion(x, y, ds->data, ds->w, ds->h, y_offset, bm.bitmap(), bm.width(), bm.height(), offset, gap, color);
}

void REGION::drawIcon(uint16_t x, uint16_t y, BITMAP &bm, uint16_t color, BM_ALIGN x_align, BM_ALIGN y_align) {
	if (!ds) return;
	uint16_t y_offset = 0;
	if (ds->h > bm.height()) {
		if (y_align != align_left)
			y_offset = ds->h - bm.height();
		if (y_align == align_center)
			y_offset >>= 1;
	}
	int16_t offset = 0;
	if (ds->w > bm.width()) {
		if (y_align != align_left)
			offset = ds->w - bm.width();
		if (y_align == align_center)
			offset >>= 1;
	}
    // Negative offset means shift bitmap right
	TFT_ScrollBitmapOverRegion(x, y, ds->data, ds->w, ds->h, y_offset, bm.bitmap(), bm.width(), bm.height(), -offset, 0, color);
}

void REGION::drawIcon(uint16_t x, uint16_t y, uint16_t y_offset,	BITMAP &bm, uint16_t color) {
	if (!ds) return;
	TFT_ScrollBitmapOverRegion(x, y, ds->data, ds->w, ds->h, y_offset, bm.bitmap(), bm.width(), bm.height(), 0, 0, color);
}

#endif
