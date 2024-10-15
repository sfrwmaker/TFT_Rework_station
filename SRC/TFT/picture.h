/*
 * picture.h
 *
 *  Created on: May 27 2020
 *      Author: Alex
 */

#ifdef TFT_BMP_JPEG_ENABLE

#ifndef _PICTURE_H_
#define _PICTURE_H_

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool	TFT_jpegAllocate(void);
void	TFT_jpegDeallocate(void);
bool	TFT_DrawJPEG(const char *filename, int16_t x, int16_t y);

/*
 * Draw clipped JPEG file image. Put Upper-Left corner of JPEG image to (x, y)
 * and draw only a rectangular area with coordinates (area_x, area_y) and size of area_width, area_height
 * As slow as a drawJPEG() function because need to read whole jpeg file
 */
bool	TFT_ClipJPEG(const char *filename, int16_t x, int16_t y, uint16_t area_x, uint16_t area_y, uint16_t area_width, uint16_t area_height);

/*
 * Get BMP image size
 * Return value 32-bits word: high 16-bits word is width, low 16-bits word is height
 */
uint32_t TFT_BMPsize(const char *filename);

/* Draw clipped BMP file image. Put Upper-Left corner of BMP image to (x, y)
 * and draw only a rectangular area with coordinates (area_x, area_y) and size of area_width, area_height
 * Only two BMP formats are supported: 24-bit per pixel, not-compressed and 16-bit per pixel R5-G6-B5
 */
bool 	TFT_ClipBMP(const char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t bmp_x, uint16_t bmp_y);

/*
 * Draw scrolled bitmap on BMP background loaded from file
 */
bool 	TFT_ScrollBitmapOverBMP(const char *filename, int16_t x, int16_t y, uint16_t area_x, uint16_t area_y, uint16_t area_width, uint16_t area_height,
			const uint8_t *bitmap, uint16_t bm_width, int16_t offset, uint8_t gap, uint16_t txt_color);

/*
 * Load BMP file image at the given coordinates (x, y) to the memory region.
 * The memory region has dimensions rw and rh pixels
 * Both coordinates can be less than zero. In this case, top-left area of the BMP image will be clipped
 * If use_transparent is true, do not load the transparent color from BMP file
 * Only two BMP formats are supported: 24-bit per pixel, not-compressed and 16-bit per pixel R5-G6-B5
 * For best performance, save BMP file as 16-bits R6-G5-B6 format and flipped row order
 */
bool	TFT_LoadBMP(uint16_t region[], uint16_t rw, uint16_t rh, const char *filename, int16_t bmp_x, int16_t bmp_y,
			uint16_t transparent, bool use_transparent);

/*
 * Draw the memory region to the screen
 */
void	TFT_DrawRegion(uint16_t x, uint16_t y, uint16_t region[], uint16_t rw, uint16_t rh);

/*
 * Scroll bitmap (usually text line) over region and draw the result on the screen
 * Speed-up the scrolling string over background image
 */
void	TFT_ScrollBitmapOverRegion(uint16_t x, uint16_t y, uint16_t region[], uint16_t rw, uint16_t rh, uint16_t y_offset,
			const uint8_t *bitmap, uint16_t bm_width, uint16_t bm_height, int16_t offset, uint8_t gap, uint16_t txt_color);

#ifdef __cplusplus
}
#endif

#endif

#endif
