/*
 * common.c
 *
 *  Created on: Nov 4, 2022
 *      Author: Alex
 *
 *  Modified May 22, 2024
 */

#include "ll_spi.h"
#include <stdlib.h>
#include "common.h"

// Forward function declarations
static uint32_t	TFT_Read(uint8_t cmd, uint8_t length);
static void		TFT_DrawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t color);
static void 	TFT_DrawFilledCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t delta, uint16_t color);
static void 	swap16(uint16_t* a, uint16_t* b);
static void 	swap32(int32_t*  a, int32_t* b);

// Width & Height of the display used to draw elements (with rotation)
static uint16_t				TFT_WIDTH		= 0;
static uint16_t				TFT_HEIGHT		= 0;
// Screen rotation attributes
static tRotation			angle			= TFT_ROTATION_0;
static uint8_t				madctl_arg[4]	= {0x40|0x08, 0x20|0x08, 0x80|0x08, 0x40|0x80|0x20|0x08};
static uint16_t				TFT_WIDTH_0		= 0;	// Generic display width  (without rotation)
static uint16_t				TFT_HEIGHT_0	= 0;	// Generic display height (without rotation)


uint16_t TFT_Width(void) {
	return TFT_WIDTH;
}

uint16_t TFT_Height(void) {
	return TFT_HEIGHT;
}

tRotation TFT_Rotation(void) {
	return angle;
}

void TFT_Setup(uint16_t generic_width, uint16_t generic_height, uint8_t madctl[4]) {
	TFT_WIDTH_0		= generic_width;
	TFT_HEIGHT_0	= generic_height;
	if (madctl) {
		for (uint8_t i = 0; i < 4; ++i)
			madctl_arg[i] = madctl[i];
	}
	TFT_SetRotation(TFT_ROTATION_0);
	// Initialize transfer infrastructure for the display SPI bus (With DMA)
	IFACE_ColorBlockInit();
}

void TFT_SetRotation(tRotation rotation)  {
	TFT_Delay(1);

	switch (rotation)  {
		case TFT_ROTATION_90:
		case TFT_ROTATION_270:
			TFT_WIDTH	= TFT_HEIGHT_0;
			TFT_HEIGHT	= TFT_WIDTH_0;
			break;
		case TFT_ROTATION_0:
		case TFT_ROTATION_180:
		default:
			TFT_WIDTH	= TFT_WIDTH_0;
			TFT_HEIGHT	= TFT_HEIGHT_0;
			break;
	}
	// Perform CMD_MADCTL_36 command
	TFT_Command(0x36, &madctl_arg[(uint8_t)rotation], 1);
	TFT_Delay(10);
	angle = rotation;
}

uint16_t TFT_WheelColor(uint8_t wheel_pos) {
	wheel_pos = 255 - wheel_pos;
	if (wheel_pos < 85) {
		return TFT_Color(255 - wheel_pos * 3, 0, wheel_pos * 3);
	}
	if (wheel_pos < 170) {
		wheel_pos -= 85;
		return TFT_Color(0, wheel_pos * 3, 255 - wheel_pos * 3);
	}
	wheel_pos -= 170;
	return TFT_Color(wheel_pos * 3, 255 - wheel_pos * 3, 0);
}

uint16_t TFT_Color(uint8_t red, uint8_t green, uint8_t blue) {
	return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

void TFT_SetAttrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	uint8_t data[4];

	// column address set
	data[0] = (x0 >> 8) & 0xFF;
	data[1] = x0 & 0xFF;
	data[2] = (x1 >> 8) & 0xFF;
	data[3] = x1 & 0xFF;
	TFT_Command(0x2A, data, 4);

	// row address set
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;
    TFT_Command(0x2B, data, 4);
}

// Initializes the rectangular area for new data
void TFT_StartDrawArea(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height) {
	TFT_SetAttrWindow(x0, y0, x0 + width - 1, y0 + height -1);
	TFT_Command(0x2C, 0, 0);							// write to RAM
	IFACE_DataMode();
}

// Fills the rectangular area with a single color
void TFT_DrawFilledRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
	if ((x > TFT_WIDTH) || (y > TFT_HEIGHT) || (width < 1) || (height < 1)) return;
	if ((x + width  - 1) > TFT_WIDTH)  width  = TFT_WIDTH  - x;
	if ((y + height - 1) > TFT_HEIGHT) height = TFT_HEIGHT - y;
	TFT_SetAttrWindow(x, y, x + width - 1, y + height - 1);
	TFT_Command(0x2C, 0, 0);							// write to RAM
	IFACE_DataMode();
	TFT_ColorBlockSend(color, width*height);
	TFT_FinishDrawArea();
}

// Sets address (entire screen) and Sends Height*Width amount of color information to Display
void TFT_FillScreen(uint16_t color) {
	TFT_DrawFilledRect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}

void TFT_DrawPixel(uint16_t x,  uint16_t y, uint16_t color) {
	if ((x >=TFT_WIDTH) || (y >=TFT_HEIGHT)) return;
	IFACE_DrawPixel(x, y, color);
}

void TFT_DrawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	TFT_DrawFilledRect(x, y, length, 1, color);
}

void TFT_DrawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
	TFT_DrawFilledRect(x, y, 1, length, color);
}

// Draw a hollow rectangle between positions (x1,y1) and (x2,y2) with specified color
void TFT_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
	if ((x >= TFT_WIDTH) || (y >= TFT_HEIGHT) || (x + width >= TFT_WIDTH) || (y + height >= TFT_HEIGHT)) return;

	TFT_DrawHLine(x, y, width, color);
	TFT_DrawHLine(x, y + height, width, color);
	TFT_DrawVLine(x, y, height, color);
	TFT_DrawVLine(x + width, y,  height, color);

	if ((width) || (height))  {
		TFT_DrawPixel(x + width, y + height, color);
	}
}

void TFT_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color) {
	// Draw edges
	TFT_DrawHLine(x + r  , y    , w - r - r, color);			// Top
	TFT_DrawHLine(x + r  , y + h - 1, w - r - r, color);		// Bottom
	TFT_DrawVLine(x    , y + r  , h - r - r, color);			// Left
	TFT_DrawVLine(x + w - 1, y + r  , h - r - r, color);		// Right
	// Draw four corners
	TFT_DrawCircleHelper(x + r,					y + r, 	r, 1, color);
	TFT_DrawCircleHelper(x + w - r - 1,			y + r, 	r, 2, color);
	TFT_DrawCircleHelper(x + w - r - 1, y + h - r - 1,	r, 4, color);
	TFT_DrawCircleHelper(x + r, 		y + h - r - 1,	r, 8, color);

}

void TFT_DrawFilledRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color) {
	TFT_DrawFilledRect(x + r, y, w - r, h, color);
	// Draw right vertical bar with round corners
	TFT_DrawFilledCircleHelper(x + w - 1,	y + r, r, 1, h - r - r - 1, color);
	// Draw left vertical bar with round corners
	TFT_DrawFilledCircleHelper(x + r,		y + r, r, 2, h - r - r - 1, color);
}

// Draw thin line
void TFT_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
	uint8_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap16(&x0, &y0);
		swap16(&x1, &y1);
	}

	if (x0 > x1) {
		swap16(&x0, &x1);
		swap16(&y0, &y1);
	}

	int32_t dx = x1 - x0, dy = abs(y1 - y0);;
	int32_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

	if (y0 < y1) ystep = 1;

	// Split into steep and not steep for FastH/V separation
	if (steep) {
		for (; x0 <= x1; x0++) {
			dlen++;
			err -= dy;
			if (err < 0) {
				err += dx;
				if (dlen == 1)
					TFT_DrawPixel(y0, xs, color);
				else TFT_DrawVLine(y0, xs, dlen, color);
				dlen = 0; y0 += ystep; xs = x0 + 1;
			}
		}
		if (dlen) TFT_DrawVLine(y0, xs, dlen, color);
	} else {
		for (; x0 <= x1; x0++) {
			dlen++;
			err -= dy;
			if (err < 0) {
				err += dx;
				if (dlen == 1)
					TFT_DrawPixel(xs, y0, color);
				else TFT_DrawHLine(xs, y0, dlen, color);
				dlen = 0; y0 += ystep; xs = x0 + 1;
			}
		}
		if (dlen) TFT_DrawHLine(xs, y0, dlen, color);
	}
}

// Draw circle
void TFT_DrawCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color) {
    // Bresenham algorithm
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
    	TFT_DrawPixel(x - x_pos, y + y_pos, color);
    	TFT_DrawPixel(x + x_pos, y + y_pos, color);
    	TFT_DrawPixel(x + x_pos, y - y_pos, color);
    	TFT_DrawPixel(x - x_pos, y - y_pos, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
              e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}

void TFT_DrawFilledCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color) {
    // Bresenham algorithm
    int x_pos = -radius;
    int y_pos = 0;
    int err = 2 - 2 * radius;
    int e2;

    do {
    	TFT_DrawPixel(x - x_pos, y + y_pos, color);
    	TFT_DrawPixel(x + x_pos, y + y_pos, color);
    	TFT_DrawPixel(x + x_pos, y - y_pos, color);
    	TFT_DrawPixel(x - x_pos, y - y_pos, color);
    	TFT_DrawHLine(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, color);
    	TFT_DrawHLine(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, color);
        e2 = err;
        if (e2 <= y_pos) {
            err += ++y_pos * 2 + 1;
            if(-x_pos == y_pos && e2 <= x_pos) {
                e2 = 0;
            }
        }
        if (e2 > x_pos) {
            err += ++x_pos * 2 + 1;
        }
    } while (x_pos <= 0);
}


void TFT_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	TFT_DrawLine(x0, y0, x1, y1, color);
	TFT_DrawLine(x1, y1, x2, y2, color);
	TFT_DrawLine(x2, y2, x0, y0, color);
}

// Draw filled triangle
void TFT_DrawfilledTriangle (uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	int32_t a, b, y, last;
	// Sort coordinates by Y order (y2 >= y1 >= y0)
	if (y0 > y1) {
		swap16(&y0, &y1);
		swap16(&x0, &x1);
	}
	if (y1 > y2) {
		swap16(&y2, &y1);
		swap16(&x2, &x1);
	}
	if (y0 > y1) {
		swap16(&y0, &y1);
		swap16(&x0, &x1);
	}

	if (y0 == y2) {								// Handle awkward all-on-same-line case as its own thing
		a = b = x0;
		if (x1 < a)      a = x1;
		else if (x1 > b) b = x1;
		if (x2 < a)      a = x2;
		else if (x2 > b) b = x2;
		TFT_DrawHLine(a, y0, b - a + 1, color);
		return;
	}

	int32_t
		dx01 = x1 - x0,
		dy01 = y1 - y0,
		dx02 = x2 - x0,
		dy02 = y2 - y0,
		dx12 = x2 - x1,
		dy12 = y2 - y1,
		sa   = 0,
		sb   = 0;

	// For upper part of triangle, find scanline crossings for segments
	// 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
	// is included here (and second loop will be skipped, avoiding a /0
	// error there), otherwise scanline y1 is skipped here and handled
	// in the second loop...which also avoids a /0 error here if y0=y1
	// (flat-topped triangle).
	if (y1 == y2) last = y1; 					// Include y1 scanline
	else         last = y1 - 1; 				// Skip it

	for (y = y0; y <= last; y++) {
		a   = x0 + sa / dy01;
		b   = x0 + sb / dy02;
		sa += dx01;
		sb += dx02;

		if (a > b) swap32(&a, &b);
		TFT_DrawHLine(a, y, b - a + 1, color);
	}

	// For lower part of triangle, find scanline crossings for segments
	// 0-2 and 1-2.  This loop is skipped if y1=y2.
	sa = dx12 * (y - y1);
	sb = dx02 * (y - y0);
	for (; y <= y2; y++) {
		a   = x1 + sa / dy12;
		b   = x0 + sb / dy02;
		sa += dx12;
		sb += dx02;

		if (a > b) swap32(&a, &b);
		TFT_DrawHLine(a, y, b - a + 1, color);
	}

}

void TFT_DrawEllipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color) {
	if (rx < 2 || ry < 2) return;
	int32_t x, y;
	int32_t rx2 = rx * rx;
	int32_t ry2 = ry * ry;
	int32_t fx2 = 4 * rx2;
	int32_t fy2 = 4 * ry2;
	int32_t s;

	for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++) {
		// These are ordered to minimize coordinate changes in x or y
		// drawPixel can then send fewer bounding box commands
		TFT_DrawPixel(x0 + x, y0 + y, color);
		TFT_DrawPixel(x0 - x, y0 + y, color);
		TFT_DrawPixel(x0 - x, y0 - y, color);
		TFT_DrawPixel(x0 + x, y0 - y, color);
		if (s >= 0) {
			s += fx2 * (1 - y);
			y--;
		}
		s += ry2 * ((4 * x) + 6);
	}

	for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++) {
		// These are ordered to minimize coordinate changes in x or y
		// drawPixel can then send fewer bounding box commands
		TFT_DrawPixel(x0 + x, y0 + y, color);
		TFT_DrawPixel(x0 - x, y0 + y, color);
		TFT_DrawPixel(x0 - x, y0 - y, color);
		TFT_DrawPixel(x0 + x, y0 - y, color);
		if (s >= 0) {
			s += fy2 * (1 - x);
			x--;
		}
		s += rx2 * ((4 * y) + 6);
	}
}

void TFT_DrawFilledEllipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color) {
	if (rx<2) return;
	if (ry<2) return;
	int32_t x, y;
	int32_t rx2 = rx * rx;
	int32_t ry2 = ry * ry;
	int32_t fx2 = 4 * rx2;
	int32_t fy2 = 4 * ry2;
	int32_t s;

	for (x = 0, y = ry, s = 2*ry2+rx2*(1-2*ry); ry2*x <= rx2*y; x++) {
		TFT_DrawHLine(x0 - x, y0 - y, x + x + 1, color);
		TFT_DrawHLine(x0 - x, y0 + y, x + x + 1, color);

		if (s >= 0) {
			s += fx2 * (1 - y);
			y--;
		}
		s += ry2 * ((4 * x) + 6);
	}

	for (x = rx, y = 0, s = 2*rx2+ry2*(1-2*rx); rx2*y <= ry2*x; y++) {
		TFT_DrawHLine(x0 - x, y0 - y, x + x + 1, color);
		TFT_DrawHLine(x0 - x, y0 + y, x + x + 1, color);

		if (s >= 0)
		{
			s += fy2 * (1 - x);
			x--;
		}
		s += rx2 * ((4 * y) + 6);
	}
}

// Draw rectangle Area. Every pixel generated by the callback function nextPixelCB
void TFT_DrawArea(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height, t_NextPixel nextPixelCB) {
	if (x0 >= TFT_WIDTH || y0 >= TFT_HEIGHT || area_width < 1 || area_height < 1) return;
	if ((x0 + area_width  - 1) > TFT_WIDTH)  area_width  = TFT_WIDTH  - x0;
	if ((y0 + area_height - 1) > TFT_HEIGHT) area_height = TFT_HEIGHT - y0;

	TFT_StartDrawArea(x0, y0, area_width, area_height);
	// Write color data row by row
    for (uint16_t row = 0; row < area_height; ++row) {
    	for (uint16_t bit = 0; bit < area_width; ++bit) {
			uint16_t color 	 = (*nextPixelCB)(row, bit);
			TFT_ColorBlockSend(color, 1);
    	}
    }
    TFT_FinishDrawArea();						// Flush color block buffer
}

// BITMAP functions
// Clear bitmap content
void TFT_BM_Clear(uint8_t *bitmap, uint32_t size) {
	for (uint32_t i = 0; i < size; ++i)
		bitmap[i] = 0;
}

// Set dot inside bitmap
void TFT_BM_DrawPixel(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y) {
	if (x >= bm_width || y >= bm_height) return;
	uint8_t  bytes_per_row = (bm_width + 7) >> 3;
	uint16_t in_byte = y * bytes_per_row + (x >> 3);
	uint8_t  in_mask = 0x80 >> (x & 0x7);
	bitmap[in_byte] |= in_mask;
}

// Read dot from bitmap; Returns 0 if no dot set
uint8_t	TFT_BM_Pixel(const uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y) {
	if (x >= bm_width || y >= bm_height) return 0;
	uint8_t  bytes_per_row = (bm_width + 7) >> 3;
	uint16_t in_byte = y * bytes_per_row + (x >> 3);
	uint8_t  in_mask = 0x80 >> (x & 0x7);
	return (bitmap[in_byte] & in_mask);
}

// Draw horizontal line inside bitmap
void TFT_BM_DrawHLine(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y, uint16_t length) {
	if (length == 0 || x >= bm_width || y >= bm_height) return;
	if (x + length >= bm_width) length = bm_width - x;
	uint16_t bytes_per_row	= (bm_width+7) >> 3;
	uint32_t byte_index		= y * bytes_per_row + (x >> 3);
	uint16_t start_bit		= x & 7;
	while (length > 0) {
		uint8_t mask = (0x100 >> start_bit) - 1; 			// several '1'staring from start_bit to end of byte
		uint16_t end_bit = start_bit + length;
		if (end_bit < 8) {									// Line terminates in the current byte
			uint8_t clear_mask = (0x100 >> end_bit) - 1;
			mask &= ~clear_mask;							// clear tail of the byte
			length = 0;										// Last pixel
		} else {
			length -= 8 - start_bit;
		}
		bitmap[byte_index++] |= mask;
		start_bit = 0;										// Fill up the next byte
	}
}

// Draw vertical line inside bitmap
void TFT_BM_DrawVLine(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y, uint16_t length) {
	if (length == 0 || x >= bm_width || y >= bm_height) return;
	if (y+length >= bm_height) length = bm_height - y;
	uint16_t bytes_per_row	= (bm_width+7) >> 3;
	uint32_t byte_index		= y * bytes_per_row + (x >> 3);
	uint16_t bit			= x & 7;
	uint8_t  mask = (0x80 >> bit);
	for (uint16_t i = 0; i < length; ++i) {
		bitmap[byte_index] |= mask;
		byte_index += bytes_per_row;
	}
}

// Join bitmaps. Draw icon (another bitmap) inside current bitmap.
void TFT_BM_JoinIcon(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t x, uint16_t y, const uint8_t *icon, uint16_t ic_width, uint16_t ic_height) {
	if (x >= bm_width || y >= bm_height) return;
	uint16_t bm_bytes_per_row = (bm_width + 7) >> 3;
	uint16_t ic_bytes_per_row = (ic_width + 7) >> 3;
	uint8_t  first_bit 	= x & 7;							// The first bit in the bitmap byte
	uint16_t first_byte	= x >> 3;							// First byte of bitmap to start drawing the icon
	for (uint16_t row = 0; row < ic_height; ++ row) {
		if (row + y >= bm_height)							// Out of the bitmap border
			break;
		uint8_t lb = icon[row * ic_bytes_per_row];			// Left icon byte in the row
		lb >>= first_bit;
		uint16_t bm_byte = (row + y) * bm_bytes_per_row + first_byte; // Left bitmap byte in the row
		bitmap[bm_byte++] |= lb;							// Draw left byte in the row
		for (uint16_t ic_byte = 0; ic_byte < ic_bytes_per_row; ++ic_byte) {
			if (ic_byte + first_byte >= bm_bytes_per_row-1)	// Out of the bitmap border
				break;
			uint16_t icon_pos = ic_bytes_per_row * row + ic_byte;
			uint16_t mask = icon[icon_pos] << 8;
			if (ic_byte < ic_bytes_per_row - 1)				// If first_bit > 0, the icon can be shifted right some pixels!
				mask |= icon[icon_pos+1];
			mask >>= first_bit;
			mask &= 0xff;
			bitmap[bm_byte++] = mask;
		}
	}
}

// Draw coordinate Axis
void TFT_BM_DrawVGauge(uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, uint16_t gauge, uint8_t edged) {
	if (gauge > bm_height) gauge = bm_height;
	uint16_t top_line = bm_height - gauge;
	uint8_t  bytes_per_row = (bm_width + 7) >> 3;
	uint32_t size = bytes_per_row * bm_height;
	TFT_BM_Clear(bitmap, size);

	uint16_t x_offset		= 0;
	uint16_t half_height	= bm_height >> 1;
	if (edged) {
		TFT_BM_DrawHLine(bitmap, bm_width, bm_height, 0, 0, bm_width); // Horizontal top line
		for (uint16_t i = 1; i < top_line; ++i) {			// draw edged part of the triangle
			uint16_t next_offset = (bm_width * i + half_height) / bm_height;
			TFT_BM_DrawHLine(bitmap, bm_width, bm_height, x_offset, i, next_offset - x_offset+1); // draw left line of the triangle
			x_offset = next_offset;
		}
		TFT_BM_DrawVLine(bitmap, bm_width, bm_height, bm_width-1, 0, top_line);
	}
	for (uint16_t i = top_line; i < bm_height; ++i) {
		x_offset = (bm_width * i + half_height) / bm_height;
		TFT_BM_DrawHLine(bitmap, bm_width, bm_height, x_offset, i, bm_width - x_offset); // fill-up the triangle
	}
}

// Draw bitmap
void TFT_DrawBitmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,
		const uint8_t *bitmap, uint16_t bm_width, uint16_t bg_color, uint16_t fg_color) {

	if (x0 >= TFT_WIDTH || y0 >= TFT_HEIGHT || area_width < 1 || area_height < 1 || bm_width < 1 || !bitmap) return;
	if ((x0 + area_width  - 1) > TFT_WIDTH)  area_width  = TFT_WIDTH  - x0;
	if ((y0 + area_height - 1) > TFT_HEIGHT) area_height = TFT_HEIGHT - y0;

	TFT_StartDrawArea(x0, y0, area_width, area_height);

	uint16_t bytes_per_row = (bm_width + 7) >> 3;
	// Write color data row by row
    for (uint16_t row = 0; row < area_height; ++row) {
    	uint16_t out_bit  = 0;					// Number of bits were pushed out
    	for (uint16_t bit = 0; bit < bm_width; ++bit) {
			if (out_bit >= area_width)			// row is over
				break;
			uint16_t in_byte = row * bytes_per_row + (bit >> 3);
			uint8_t  in_mask = 0x80 >> (bit & 0x7);
			uint16_t color 	 = (in_mask & bitmap[in_byte])?fg_color:bg_color;
			TFT_ColorBlockSend(color, 1);
			++out_bit;
    	}
    	// Fill-up rest area with background color
    	if (area_width > out_bit) {
    		TFT_ColorBlockSend(bg_color, area_width - out_bit);
    	}
    }
    // Send rest data from the buffer
    TFT_FinishDrawArea();						// Flush color block buffer
}

// Draw bitmap created from the string.
void TFT_DrawScrolledBitmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,
		const uint8_t *bitmap, uint16_t bm_width, int16_t offset, uint8_t gap, uint16_t bg_color, uint16_t fg_color) {

	if (x0 >= TFT_WIDTH || y0 >= TFT_HEIGHT || area_width < 1 || area_height < 1 || bm_width < 1 || !bitmap) return;
	if ((x0 + area_width  - 1) > TFT_WIDTH)  area_width  = TFT_WIDTH  - x0;
	if ((y0 + area_height - 1) > TFT_HEIGHT) area_height = TFT_HEIGHT - y0;
	while (offset >= bm_width) offset -= bm_width + gap;

	TFT_StartDrawArea(x0, y0, area_width, area_height);

    uint16_t bytes_per_row = (bm_width + 7) >> 3;
    // Write color data row by row
    for (uint16_t row = 0; row < area_height; ++row) {
    	uint16_t out_bit = 0;					// Number of bits were pushed out
    	if (offset < 0) {						// Negative offset means bitmap should be shifted right
    		// Fill-up left side with background color
    		out_bit = abs(offset);
    		if (out_bit > area_width) out_bit = area_width;
    		TFT_ColorBlockSend(bg_color, out_bit);
    	}
    	int16_t bitmap_offset = offset;				// The bitmap offset is actual on the first while() loop only
    	while (out_bit < area_width) {				// The bitmap can fit the region several times
			for (uint16_t bit = (bitmap_offset > 0)?bitmap_offset:0; bit < bm_width; ++bit) {
				if (out_bit >= area_width)			// row is over
					break;
				uint16_t in_byte = row * bytes_per_row + (bit >> 3);
				uint8_t  in_mask = 0x80 >> (bit & 0x7);
				uint16_t color 	 = (in_mask & bitmap[in_byte])?fg_color:bg_color;
				TFT_ColorBlockSend(color, 1);
				++out_bit;
			}
			bitmap_offset = 0;						// The bitmap offset is actual on the first while() loop only
			if (gap == 0) {							// Not looped bitmap. Fill-up rest area with background color
				if (area_width > out_bit)
					TFT_ColorBlockSend(bg_color, area_width - out_bit);
			} else {								// Looped bitmap
				uint8_t bg_bits = gap;				// Draw the gap between looped bitmap images
				if (area_width > out_bit) {			// There is a place to draw
					if (bg_bits > (area_width - out_bit))
						bg_bits = area_width - out_bit;
					TFT_ColorBlockSend(bg_color, bg_bits);
					out_bit += bg_bits;
				}
			}
    	}
    }
    // Send rest data from the buffer
    TFT_FinishDrawArea();									// Flush color block buffer
}

// Draw pixmap
void TFT_DrawPixmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,
		const uint8_t *pixmap, uint16_t pm_width, uint8_t depth, uint16_t palette[]) {

	if (x0 >= TFT_WIDTH || y0 >= TFT_HEIGHT || area_width < 1 || area_height < 1 || pm_width < 1 || !pixmap) return;
	if ((x0 + area_width  - 1) > TFT_WIDTH)  area_width  = TFT_WIDTH  - x0;
	if ((y0 + area_height - 1) > TFT_HEIGHT) area_height = TFT_HEIGHT - y0;

	TFT_StartDrawArea(x0, y0, area_width, area_height);

	uint16_t bytes_per_row = (pm_width*depth + 7) >> 3;
	// Write color data row by row
    for (uint16_t row = 0; row < area_height; ++row) {
    	uint16_t out_pixel  = 0;							// Number of pixels were pushed out
    	uint16_t in_mask  = ((1 << depth) - 1) << (16-depth); // pixel bit mask shifted to the left position in the word
    	uint16_t in_byte  = row * bytes_per_row;
    	uint8_t	 sh_right = 16 - depth;
    	for (uint16_t bit = 0; bit < pm_width; ++bit) {
    		if (out_pixel >= area_width)					// row is over
				break;
			uint16_t code = pixmap[in_byte] & (in_mask >> 8);
			code <<= 8;
			code |= pixmap[in_byte+1] & (in_mask & 0xff);
			code >>= sh_right;
			uint16_t color = palette[code];
			TFT_ColorBlockSend(color, 1);
			in_mask >>= depth;
			if ((in_mask & 0xff00) == 0) {					// Go to the next byte
				in_mask <<= 8;								// Correct input mask
				sh_right += 8;								// Correct shift bits value
				++in_byte;
			}
			sh_right -= depth;
			++out_pixel;
    	}
    	// Fill-up rest area with background color
    	if (area_width > out_pixel) {
    		TFT_ColorBlockSend(palette[0], area_width - out_pixel);
    	}
    }
    // Send rest data from the buffer
    TFT_FinishDrawArea();									// Flush color block buffer
}

void TFT_Touch_Adjust_Rotation_XY(uint16_t *x, uint16_t *y) {
	int16_t X = *x;
	int16_t Y = *y;
	switch(angle) {
		case TFT_ROTATION_90:
			X = *y;
			Y = TFT_WIDTH_0 - *x;
			break;
		case TFT_ROTATION_180:
			X = TFT_WIDTH_0 - *x;
			Y = TFT_HEIGHT_0 - *y;
			break;
		case TFT_ROTATION_270:
			X = TFT_HEIGHT_0 - *y;
			Y = *x;
			break;
		default:
			break;
	}
	if (X < 0)
		X = 0;
	else if (X >= TFT_WIDTH)
		X = TFT_WIDTH - 1;
	if (Y < 0)
		Y = 0;
	else if (Y >= TFT_HEIGHT)
		Y = TFT_HEIGHT - 1;
	*x = X; *y = Y;
}

// ======================================================================
// Display specific default functions
// ======================================================================
uint32_t TFT_DEF_ReadID4(void) {
	return TFT_Read(0xD3, 3);
}

uint16_t TFT_ReadPixel(uint16_t x, uint16_t y, bool is16bit_color) {
	if ((x >=TFT_WIDTH) || (y >=TFT_HEIGHT)) return 0;

	if (is16bit_color)
		TFT_Command(0x3A, (uint8_t *)"\x66", 1);
	TFT_SetAttrWindow(x, y, x, y);
	uint16_t ret = 0;
	uint8_t data[4];
	if (TFT_ReadData(0x2E, data, 4)) {
		ret = TFT_Color(data[1], data[2], data[3]);
	}
	if (is16bit_color)
		TFT_Command(0x3A, (uint8_t *)"\x55", 1);
	return ret;
}

// ======================================================================
// Internal functions
// ======================================================================
// Read up-to 4 bytes of data from the display and returns 32-bits word
static uint32_t TFT_Read(uint8_t cmd, uint8_t length) {
	if (length == 0 || length > 4)
		return 0xffffffff;
	uint8_t data[4];
	if (TFT_ReadData(cmd, data, 4)) {
		uint32_t value = 0;
		for (uint8_t i = 0; i < length; ++i) {
			value <<= 8;
			value |= data[i];
		}
		return value;
	}
	return 0xffffffff;
}

// Draw round corner
static void TFT_DrawCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t color) {
	int32_t f     = 1 - r;
	int32_t ddF_x = 1;
	int32_t ddF_y = -2 * r;
	int32_t x     = 0;

	while (x < r) {
		if (f >= 0) {
			r--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;
		if (cornername & 0x4) {
			TFT_DrawPixel(x0 + x, y0 + r, color);
			TFT_DrawPixel(x0 + r, y0 + x, color);
		}
		if (cornername & 0x2) {
			TFT_DrawPixel(x0 + x, y0 - r, color);
			TFT_DrawPixel(x0 + r, y0 - x, color);
		}
		if (cornername & 0x8) {
			TFT_DrawPixel(x0 - r, y0 + x, color);
			TFT_DrawPixel(x0 - x, y0 + r, color);
		}
		if (cornername & 0x1) {
			TFT_DrawPixel(x0 - r, y0 - x, color);
			TFT_DrawPixel(x0 - x, y0 - r, color);
		}
	}
}

// Draw filled corner
static void TFT_DrawFilledCircleHelper(uint16_t x0, uint16_t y0, uint16_t r, uint8_t cornername, uint16_t delta, uint16_t color) {
	int32_t f     = 1 - r;
	int32_t ddF_x = 1;
	int32_t ddF_y = -r - r;
	int32_t x     = 0;

	delta++;
	while (x < r) {
		if (f >= 0) {
			r--;
			ddF_y += 2;
			f     += ddF_y;
		}
		x++;
		ddF_x += 2;
		f     += ddF_x;

		if (cornername & 0x1) {
			TFT_DrawVLine(x0 + x, y0 - r, r + r + delta, color);
			TFT_DrawVLine(x0 + r, y0 - x, x + x + delta, color);
		}
		if (cornername & 0x2) {
			TFT_DrawVLine(x0 - x, y0 - r, r + r + delta, color);
			TFT_DrawVLine(x0 - r, y0 - x, x + x + delta, color);
		}
	}
}

static void swap16(uint16_t* a, uint16_t* b) {
	uint16_t t = *a;
	*a = *b;
	*b = t;
}

static void swap32(int32_t* a, int32_t* b) {
	uint16_t t = *a;
	*a = *b;
	*b = t;
}

