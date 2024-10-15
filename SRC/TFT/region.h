/*
 * region.h
 *
 *  Created on: 2023 MAR 12
 *      Author: Alex
 */

#ifndef _REGION_H_
#define _REGION_H_

#include "config.h"
#include <string>
#include "bitmap.h"
#include "u8g_font.h"

#ifdef TFT_BMP_JPEG_ENABLE

/*
 * Region is a full-color rectangular area to be drawn in the screen.
 * Region data structure is allocated in memory by malloc() call.
 * Each Region instance is allocated just once
 * All other copies are just "links" to the main instance
 */
struct r_data {
	uint16_t	w;												// Region active width
	uint16_t	h;												// Region active height
	uint16_t	links;											// Number of the links to allocated area
	uint32_t	size;											// Region full size (16-bits WORDS)
	uint16_t	data[0];										// Data array
};

class REGION {
	public:
		REGION(void)											{ }
		REGION(uint32_t size);
		REGION(uint16_t width, uint16_t height);
		REGION(uint8_t* addr, uint16_t width, uint16_t height);
		REGION(const REGION &rg);
		REGION&		operator=(const REGION &rg);
		~REGION(void);
		uint32_t	totalSize(void);
		uint16_t*	region(void)								{ return (ds)?ds->data:0;	}
		uint16_t	width(void)									{ return (ds)?ds->w:0;		}
		uint16_t	height(void)								{ return (ds)?ds->h:0;		}
		uint32_t	size(void)									{ return (ds)?ds->size:0;	}
		void		clear();
		bool		setSize(uint16_t width, uint16_t height);	// Setup picture size inside the region, cannot be greater than actual size
		void		draw(uint16_t x, uint16_t y);
		void		fill(uint16_t color);
		void		drawPixel(uint16_t x, uint16_t y, uint16_t color);
		uint16_t	pixel(uint16_t x, uint16_t y);
		void		drawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
		void		drawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color);
		bool 		loadBMP(const char *filename, int16_t bmp_x, int16_t bmp_y, uint16_t transparent = 0, bool use_transparent = false);
		void		drawIcon(uint16_t x, uint16_t y, BITMAP &bm, uint16_t color, BM_ALIGN x_align = align_left, BM_ALIGN y_align = align_left);
		void		drawIcon(uint16_t x, uint16_t y, uint16_t y_offset,	BITMAP &bm, uint16_t color);
		void		scrollBitmap(uint16_t x, uint16_t y, uint16_t y_offset,	BITMAP &bm, int16_t offset, uint8_t gap, uint16_t color);
	private:
		struct r_data	*ds	= 0;
};


#endif

#endif
