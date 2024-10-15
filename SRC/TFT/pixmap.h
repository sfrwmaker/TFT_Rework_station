/*
 * pixmap.h
 *
 *  Created on: Sep 09 2021
 *      Author: Alex
 */

#ifndef _PIXMAP_H_
#define _PIXMAP_H_

#include "main.h"
#include <stdint.h>

/*
 * Pixmap is a multi-color icon with predefined palette of colors.
 * Supported depth: 1-8, number of colors 2, 4, 8, 16, ... 256
 * Each pixel coded by several bits in the pixmap-array.
 * Pixmap data structure is allocated in memory by malloc() call.
 * The space to store pixmap, pallete, width and height is allocated together
 * with the space for pixmap itself
 * Each pixmap instance is allocated just once
 * All other copies are just "links" to the main instance
 */
struct p_data {
	uint16_t	w;												// Pixmap width
	uint16_t	h;												// Pixmap height
	uint8_t		links;											// Number of the links to allocated area
	uint8_t		depth;											// The number of bits per pixel. Colors supported by pixmap is 2^depth
	uint8_t		colors;											// The number of colors actually allocated in the palette
	uint8_t		data[0];										// Data array. The Last elements are palette colors (uint16_t per color code)
};

class PIXMAP {
	public:
		PIXMAP(void)											{ }
		PIXMAP(uint16_t width, uint16_t height, uint8_t depth);
		PIXMAP(const PIXMAP &bm);
		PIXMAP&		operator=(const PIXMAP &pm);
		~PIXMAP(void);
		uint8_t*	pixmap(void)								{ return (ds)?ds->data:0;	}
		uint8_t		depth(void)									{ return (ds)?ds->depth:0;	}
		uint16_t	width(void)									{ return (ds)?ds->w:0;		}
		uint16_t	height(void)								{ return (ds)?ds->h:0;		}
		void		clear(void);
		void		setupPalette(uint16_t p[], uint8_t colors);
		uint8_t		colorCode(uint16_t color);
		void		drawPixelCode(uint16_t x, uint16_t y, uint8_t color_code);
		void		drawPixel(uint16_t x, uint16_t y, uint16_t color);
		uint8_t		pixelCode(uint16_t x, uint16_t y);
		uint16_t	pixel(uint16_t x, uint16_t y);
		uint16_t* 	palette(void);
		void		drawHLineCode(uint16_t x, uint16_t y, uint16_t length, uint8_t color_code);
		void		drawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
		void		drawVLineCode(uint16_t x, uint16_t y, uint16_t length, uint8_t color_code);
		void		drawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
		void		draw(uint16_t x, uint16_t y, uint16_t area_width, uint16_t area_height);
		void		draw(uint16_t x, uint16_t y);
	private:
		struct p_data	*ds	= 0;
};

#endif
