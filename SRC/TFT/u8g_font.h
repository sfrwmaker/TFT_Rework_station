/*
 * u8g_font.h
 *
 */

#ifndef _U8G_FONT_H_
#define _U8G_FONT_H_

#include "main.h"
#include "u8g2_fonts.h"

#ifdef __cplusplus
extern "C" {
#endif

#define U8G2_FONT_HEIGHT_MODE_TEXT	(0)
#define U8G2_FONT_HEIGHT_MODE_XTEXT (1)
#define U8G2_FONT_HEIGHT_MODE_ALL 	(2)
#define U8G2_FONT_MAX_SCALE			(4)

typedef uint16_t u8g2_uint_t;				// for pixel position only
typedef int16_t u8g2_int_t;					// introduced for circle calculation
typedef int32_t u8g2_long_t;				// introduced for ellipse calculation
typedef struct u8g2_struct u8g2_t;
typedef u8g2_uint_t (*u8g2_font_calc_vref_fnptr)(u8g2_t *u8g2);

struct _u8g2_font_info_t {
	/* offset 0 */
	uint8_t glyph_cnt;
	uint8_t bbx_mode;
	uint8_t bits_per_0;
	uint8_t bits_per_1;
	/* offset 4 */
	uint8_t bits_per_char_width;
	uint8_t bits_per_char_height;
	uint8_t bits_per_char_x;
	uint8_t bits_per_char_y;
	uint8_t bits_per_delta_x;
	/* offset 9 */
	int8_t max_char_width;
	int8_t max_char_height;					// overall height, NOT ascent. Instead ascent = max_char_height + y_offset
	int8_t x_offset;
	int8_t y_offset;
	/* offset 13 */
	int8_t  ascent_A;
	int8_t  descent_g;						// usually a negative value
	int8_t  ascent_para;
	int8_t  descent_para;
	/* offset 17 */
	uint16_t start_pos_upper_A;
	uint16_t start_pos_lower_a;
	/* offset 21 */
	uint16_t start_pos_unicode;
};
typedef struct _u8g2_font_info_t u8g2_font_info_t;

struct _u8g2_font_decode_t {
	const uint8_t *decode_ptr;				// pointer to the compressed data
	u8g2_uint_t target_x;
	u8g2_uint_t target_y;
	int8_t x;								// local coordinates, (0,0) is upper left
	int8_t y;
	int8_t glyph_width;
	int8_t glyph_height;
	uint8_t decode_bit_pos;					// bitpos inside a byte of the compressed data
	uint8_t is_transparent;
	uint16_t fg_color;
	uint16_t bg_color;
	// To decode string into memory buffer
	uint8_t*	buff;						// Buffer to decode string into
	uint16_t	buff_width;					// String buffer width in pixels
};
typedef struct _u8g2_font_decode_t u8g2_font_decode_t;

struct _u8g2_kerning_t {
  uint16_t first_table_cnt;
  uint16_t second_table_cnt;
  const uint16_t *first_encoding_table;
  const uint16_t *index_to_second_table;
  const uint16_t *second_encoding_table;
  const uint8_t *kerning_values;
};
typedef struct _u8g2_kerning_t u8g2_kerning_t;

typedef struct u8x8_struct u8x8_t;
typedef uint16_t (*u8x8_char_cb)(u8x8_t *u8x8, uint8_t b);

struct u8x8_struct {
	u8x8_char_cb		next_cb; 			//  procedure, which will be used to get the next char from the string
	uint8_t 			utf8_state;			// number of chars which are still to scan
	uint16_t			encoding;			// encoding result for utf8 decoder in next_cb
};

struct u8g2_struct {
	u8x8_t u8x8;
	const uint8_t *font;             		// current font for all text procedures
	u8g2_font_calc_vref_fnptr	font_calc_vref;
	u8g2_font_decode_t			font_decode; // new font decode structure
	u8g2_font_info_t			font_info;	// new font info structure
	uint8_t		font_height_mode;
	int8_t 		font_ref_ascent;
	int8_t		font_ref_descent;
	int8_t 		glyph_x_offset;				// set by u8g2_GetGlyphWidth as a side effect
	uint8_t 	bitmap_transparency;		// black pixels will be treated as transparent (not drawn)
	uint8_t		scale;						// Font scale factor, default 1. Maximum value is U8G2_FONT_MAX_SCALE
};

typedef enum {
	align_left = 0, align_center, align_right
} BM_ALIGN;

void		u8g2_u8gFont(u8g2_t *u8g2);
void 		u8g2_SetFont(u8g2_t *u8g2, const uint8_t  *font);
void 		u8g2_SetFontMode(u8g2_t *u8g2, uint8_t is_transparent, uint16_t bg_color);
void		u8g2_SetFontScale(u8g2_t *u8g2, uint8_t scale);
uint8_t		u8g2_GetFontScale(u8g2_t *u8g2);
uint8_t		u8g2_IsGlyph(u8g2_t *u8g2, uint16_t requested_encoding);
int8_t		u8g2_GetGlyphWidth(u8g2_t *u8g2, uint16_t requested_encoding);

u8g2_uint_t u8g2_DrawStr(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str, uint16_t color);
u8g2_uint_t u8g2_DrawUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str, uint16_t color);

u8g2_uint_t u8g2_allocateBitmap(u8g2_t *u8g2, uint8_t** buff, const char *str, uint8_t utf8);
u8g2_uint_t u8g2_StrToBitmap(u8g2_t *u8g2, uint8_t* buff, uint16_t width, const char *str, BM_ALIGN align, uint16_t margin, uint8_t utf8);

uint8_t		u8g2_IsAllValidUTF8(u8g2_t *u8g2, const char *str);		// checks whether all codes are valid

u8g2_uint_t u8g2_GetStrWidth(u8g2_t *u8g2, const char *s);
u8g2_uint_t u8g2_GetUTF8Width(u8g2_t *u8g2, const char *str);

void 		u8g2_SetFontRefHeightText(u8g2_t *u8g2);
void 		u8g2_SetFontRefHeightExtendedText(u8g2_t *u8g2);
void 		u8g2_SetFontRefHeightAll(u8g2_t *u8g2);

void 		u8g2_SetFontPosBaseline(u8g2_t *u8g2);
void 		u8g2_SetFontPosBottom(u8g2_t *u8g2);
void 		u8g2_SetFontPosTop(u8g2_t *u8g2);
void 		u8g2_SetFontPosCenter(u8g2_t *u8g2);

int8_t		u8g2_GetMaxCharHeight(u8g2_t *u8g2);
int8_t		u8g2_GetMaxCharWidth(u8g2_t *u8g2);
int8_t		u8g2_GetAscent(u8g2_t *u8g2);
int8_t		u8g2_GetDescent(u8g2_t *u8g2);

/*
#define u8g2_GetMaxCharHeight(u8g2) ((u8g2)->font_info.max_char_height)
#define u8g2_GetMaxCharWidth(u8g2) ((u8g2)->font_info.max_char_width)
#define u8g2_GetAscent(u8g2) ((u8g2)->font_ref_ascent)
#define u8g2_GetDescent(u8g2) ((u8g2)->font_ref_descent)
#define u8g2_GetFontAscent(u8g2) ((u8g2)->font_ref_ascent)
#define u8g2_GetFontDescent(u8g2) ((u8g2)->font_ref_descent)
*/
#ifdef __cplusplus
}
#endif

#endif
