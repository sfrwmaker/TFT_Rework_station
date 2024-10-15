/*
 * pixmap.cpp
 *
 */

#include <string.h>
#include <stdlib.h>
#include "pixmap.h"
#include "common.h"

PIXMAP::PIXMAP(uint16_t width, uint16_t height, uint8_t depth) {
	ds = 0;
	if (width == 0 || height == 0 || depth == 0 || depth > 8) return;

	uint8_t colors = 1 << depth;
	uint16_t bytes_per_row = (depth*width+7) >> 3;
	// Allocate memory to store whole data structure, color patette and the pixmap data
	ds = (struct p_data *)calloc(sizeof(struct p_data) + colors * sizeof(uint16_t) + height * bytes_per_row, sizeof(uint8_t));
	if (!ds) return;
	ds->w 		= width;
	ds->h 		= height;
	ds->depth	= depth;
	ds->colors	= 0;											// No palette color defined yet
	ds->links 	= 1;
}

PIXMAP::PIXMAP(const PIXMAP &bm) {
	this->ds = bm.ds;
	++ds->links;
}

PIXMAP&	PIXMAP::operator=(const PIXMAP &pm) {
	if (this != &pm) {
		if (ds != 0 && --(ds->links) == 0) {
			free(ds);
		}
		this->ds = pm.ds;
		++ds->links;
	}
	return *this;
}

PIXMAP::~PIXMAP(void) {
	if (!ds) return;
	if (--ds->links > 0) return;								// Destroy yet another copy of the bitmap
	free(ds);
	ds = 0;
}

void PIXMAP::clear(void) {
	if (!ds) return;
	uint16_t bytes_per_row = (ds->w * ds->depth + 7) >> 3;
	uint32_t size = bytes_per_row * ds->h;
	// The first elements of data are pixmap itself (color codes), the palette is located after the pixmap data
	for (uint32_t i = 0; i < size; ++i)
		ds->data[i] = 0;
}

void PIXMAP::setupPalette(uint16_t p[], uint8_t colors) {
	if (!ds) return;
	uint16_t *pa = palette();
	if (colors > (1<<ds->depth)) colors = 1 << ds->depth;
	for (uint8_t i = 0; i < colors; ++i)
		pa[i] = p[i];
	ds->colors = colors;
}

uint8_t	PIXMAP::colorCode(uint16_t color) {
	uint16_t colors = 1 << ds->depth;
	uint16_t bytes_per_row = (ds->w*ds->depth + 7) >> 3;
	uint16_t *palette = (uint16_t *)ds->data + bytes_per_row * ds->h; // The palette is allocated after the pixmap data
	for (uint16_t i = 0; i < colors; ++i) {
		if (palette[i] == color)
			return i;
	}
	return 0;
}

void PIXMAP::drawPixelCode(uint16_t x, uint16_t y, uint8_t color_code) {
	if (!ds) return;
	if (x >= ds->w || y >= ds->h) return;
	uint16_t bytes_per_row = (ds->w*ds->depth + 7) >> 3;
	// locate byte of the first bit of the color code
	uint16_t bit	  = x * ds->depth;
	uint16_t in_byte  = y * bytes_per_row + (bit >> 3);
	uint16_t in_mask  = ((1 << ds->depth) - 1) << (16-ds->depth); // pixel bit mask shifted to the left position in the word
	uint16_t clr_mask = color_code << (16-ds->depth);
	in_mask  >>= (bit & 0x7);
	clr_mask >>= (bit & 0x7);
	ds->data[in_byte] &= ~(in_mask >> 8);
	ds->data[in_byte] |= (clr_mask >> 8);
	if (in_mask & 0xff) {
		ds->data[++in_byte] &= ~(in_mask & 0xff);
		ds->data[in_byte]   |= (clr_mask & 0xff);
	}
}

void PIXMAP::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
	if (!ds) return;
	uint16_t bytes_per_row = (ds->w*ds->depth + 7) >> 3;
	uint16_t *palette = (uint16_t *)ds->data + bytes_per_row * ds->h;	// The palette is allocated after the pixmap data
	uint8_t color_code = 0;										// Find out the required color in the palette
	for (; color_code < ds->colors; ++color_code) {
		if (palette[color_code] == color)
			break;
	}
	if (color_code > ds->colors) {								// Color not found in the palette, try to add new color to the palette
		if (color_code < (1 << ds->depth)) {
			palette[color_code] = color;
			ds->colors = color_code;
		} else {
			return;
		}
	}
	drawPixelCode(x, y, color_code);
}

uint8_t	PIXMAP::pixelCode(uint16_t x, uint16_t y) {
	if (!ds) return 0;
	if (x >= ds->w || y >= ds->h) return 0;
	uint16_t bytes_per_row = (ds->w*ds->depth + 7) >> 3;
	// locate byte of the first bit of the color code
	uint16_t bit 	  = x * ds->depth;
	uint16_t in_byte  = y * bytes_per_row + (bit >> 3);
	uint16_t in_mask  = ((1 << ds->depth) - 1) << (16-ds->depth); // pixel bit mask shifted to the left position in the word
	in_mask  >>= (bit & 0x7);
	uint16_t code = ds->data[in_byte] & (in_mask >> 8);
	code <<= 8;
	code |= ds->data[in_byte+1] & (in_mask & 0xff);
	code >>= 16 - ds->depth - (bit & 0x7);
	return code;
}

uint16_t PIXMAP::pixel(uint16_t x, uint16_t y) {
	if (!ds) return 0;
	uint8_t code = pixelCode(x, y);
	if (code >= ds->colors) return 0;
	uint16_t *pa = palette(); 									// The palette is allocated after the pixmap data
	return pa[code];
}

uint16_t*  PIXMAP::palette(void) {
	if (!ds) return 0;
	uint16_t bytes_per_row = (ds->w*ds->depth + 7) >> 3;
	uint8_t *p_addr = ds->data + bytes_per_row * ds->h;
	return (uint16_t *)p_addr;
}

void PIXMAP::drawHLineCode(uint16_t x, uint16_t y, uint16_t length, uint8_t color_code) {
	for (uint16_t i = 0; i < length; ++i) {
		drawPixelCode(x+i, y, color_code);
	}
}

void PIXMAP::drawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	drawPixelCode(x, y, color);
	if (length > 1) {
		uint8_t color_code = colorCode(color);
		drawHLineCode(x+1, y, length -1, color_code);
	}
}

void PIXMAP::drawVLineCode(uint16_t x, uint16_t y, uint16_t length, uint8_t color_code) {
	if (!ds) return;
	if (length == 0 || x > ds->w || y > ds->h) return;
	if (y+length >= ds->h) length = ds->h - y;
	uint16_t bytes_per_row = (ds->w*ds->depth + 7) >> 3;
	uint16_t bit 	  = x * ds->depth;
	uint16_t in_byte  = y * bytes_per_row + (bit >> 3);
	uint16_t in_mask  = ((1 << ds->depth) - 1) << (16-ds->depth); // pixel bit mask shifted to the left position in the word
	uint16_t clr_mask = color_code << (16-ds->depth);
	in_mask  >>= (bit & 0x7);
	clr_mask >>= (bit & 0x7);
	for (uint16_t i = 0; i < length; ++i) {
		ds->data[in_byte] &= ~(in_mask >> 8);
		ds->data[in_byte] |= (clr_mask >> 8);
		if (in_mask & 0xff) {
			ds->data[++in_byte] &= ~(in_mask & 0xff);
			ds->data[in_byte]   |= (clr_mask & 0xff);
		}
		in_byte += bytes_per_row;
	}
}

void PIXMAP::drawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	drawPixelCode(x, y, color);
	if (length > 1) {
		uint8_t color_code = colorCode(color);
		drawVLineCode(x, y+1, length -1, color_code);
	}
}

void PIXMAP::draw(uint16_t x, uint16_t y, uint16_t area_width, uint16_t area_height) {
	if (ds == 0) return;
	TFT_DrawPixmap(x, y, area_width, area_height, ds->data, ds->w, ds->depth, palette());
}

void PIXMAP::draw(uint16_t x, uint16_t y) {
	TFT_DrawPixmap(x, y, ds->w, ds->h, ds->data, ds->w, ds->depth, palette());
}
