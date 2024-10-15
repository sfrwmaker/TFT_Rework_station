/*
 * u8g_font.c
 *
 */

#include <string.h>
#include <stdlib.h>
#include "u8g_font.h"
#include "common.h"

/* size of the font data structure, there is no struct or class... */
/* this is the size for the new font format */
#define U8G2_FONT_DATA_STRUCT_SIZE 23

/*
  font data:

  offset	bytes	description
  0			1		glyph_cnt		number of glyphs
  1			1		bbx_mode	0: proportional, 1: common height, 2: monospace, 3: multiple of 8
  2			1		bits_per_0	glyph rle parameter
  3			1		bits_per_1	glyph rle parameter

  4			1		bits_per_char_width		glyph rle parameter
  5			1		bits_per_char_height	glyph rle parameter
  6			1		bits_per_char_x			glyph rle parameter
  7			1		bits_per_char_y			glyph rle parameter
  8			1		bits_per_delta_x		glyph rle parameter

  9			1		max_char_width
  10		1		max_char_height
  11		1		x offset
  12		1		y offset (descent)

  13		1		ascent (capital A)
  14		1		descent (lower g)
  15		1		ascent '('
  16		1		descent ')'

  17		1		start pos 'A' high byte
  18		1		start pos 'A' low byte

  19		1		start pos 'a' high byte
  20		1		start pos 'a' low byte

  21		1		start pos unicode high byte
  22		1		start pos unicode low byte

  Font build mode, 0: proportional, 1: common height, 2: monospace, 3: multiple of 8

  Font build mode 0:
    - "t"
    - Ref height mode: U8G2_FONT_HEIGHT_MODE_TEXT, U8G2_FONT_HEIGHT_MODE_XTEXT or U8G2_FONT_HEIGHT_MODE_ALL
    - use in transparent mode only (does not look good in solid mode)
    - most compact format
    - different font heights possible

  Font build mode 1:
    - "h"
    - Ref height mode: U8G2_FONT_HEIGHT_MODE_ALL
    - transparent or solid mode
    - The height of the glyphs depend on the largest glyph in the font. This means font height depends on postfix "r", "f" and "n".

*/

// Forward local functions declaration
static uint16_t 		u8g2_font_get_word(const uint8_t *font, uint8_t offset);
static void 			u8g2_read_font_info(u8g2_font_info_t *font_info, const uint8_t *font);
static void 			u8g2_UpdateRefHeight(u8g2_t *u8g2);
static const uint8_t   *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding);
static void 			u8g2_font_setup_decode(u8g2_t *u8g2, const uint8_t *glyph_data);
static uint8_t 			u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t *f, uint8_t cnt);
static int8_t 			u8g2_font_decode_get_signed_bits(u8g2_font_decode_t *f, uint8_t cnt);
static u8g2_uint_t 		u8g2_font_draw_glyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding);
static int8_t 			u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data);
static u8g2_uint_t		u8g2_draw_string(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str);
static void 			u8g2_font_decode_len(u8g2_t *u8g2, uint8_t len, uint8_t is_foreground);
static void				u8g2_font_draw_HLine_bitmap(uint8_t* buff, uint16_t width, uint16_t x, uint16_t y, uint8_t length);
static uint8_t			u8g2_is_all_valid(u8g2_t *u8g2, const char *str);
static u8g2_uint_t		u8g2_string_width(u8g2_t *u8g2, const char *str);

static uint16_t	 		u8x8_ascii_next(u8x8_t *u8x8, uint8_t b);
static uint16_t 		u8x8_utf8_next(u8x8_t *u8x8, uint8_t b);

static u8g2_uint_t		u8g2_font_calc_vref_font(u8g2_t *u8g2);
static u8g2_uint_t		u8g2_font_calc_vref_bottom(u8g2_t *u8g2);
static u8g2_uint_t 		u8g2_font_calc_vref_top(u8g2_t *u8g2);
static u8g2_uint_t		u8g2_font_calc_vref_center(u8g2_t *u8g2);


void u8g2_u8gFont(u8g2_t *u8g2) {
	u8g2->font = 0;
	u8g2->font_decode.is_transparent	= 1;
	u8g2->bitmap_transparency			= 0;
	u8g2->scale							= 1;
	u8g2->font_decode.fg_color			= 0;
	u8g2->font_decode.bg_color 			= 0xffff;
	u8g2_SetFontPosBaseline(u8g2);
}

void u8g2_SetFont(u8g2_t *u8g2, const uint8_t  *font) {
	if (u8g2->font != font ) {
		u8g2->font	= font;
		u8g2_read_font_info(&(u8g2->font_info), font);
		u8g2_UpdateRefHeight(u8g2);
	}
	u8g2->scale	= 1;
}

void u8g2_SetFontMode(u8g2_t *u8g2, uint8_t is_transparent, uint16_t bg_color) {
	u8g2->font_decode.is_transparent = is_transparent;
	u8g2->font_decode.bg_color = bg_color;
}

void u8g2_SetFontScale(u8g2_t *u8g2, uint8_t scale) {
	if (scale >= 1 && scale <= U8G2_FONT_MAX_SCALE) {
		u8g2->scale = scale;
	}
}

uint8_t u8g2_GetFontScale(u8g2_t *u8g2) {
	return u8g2->scale;
}

uint8_t u8g2_IsGlyph(u8g2_t *u8g2, uint16_t requested_encoding) {
	return (u8g2_font_get_glyph_data(u8g2, requested_encoding) != 0);
}

// side effect: updates u8g2->font_decode and u8g2->glyph_x_offset
int8_t u8g2_GetGlyphWidth(u8g2_t *u8g2, uint16_t requested_encoding) {
	const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
	if (glyph_data == 0) return 0;

	u8g2_font_setup_decode(u8g2, glyph_data);
	u8g2->glyph_x_offset = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
	u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);

	/* glyph width is here: u8g2->font_decode.glyph_width */
	return u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}

static u8g2_uint_t u8g2_DrawGlyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding) {
	if (u8g2->font_decode.buff_width > 0) {
		y += (u8g2->font_info.max_char_height + u8g2->font_ref_descent) * u8g2->scale;
	} else {
		y += u8g2->font_calc_vref(u8g2);
	}
	return u8g2_font_draw_glyph(u8g2, x, y, encoding);
}

u8g2_uint_t u8g2_DrawStr(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str, uint16_t color) {
	u8g2->font_decode.buff			= 0;	// Draw string directly to the display
	u8g2->font_decode.buff_width	= 0;
	u8g2->font_decode.fg_color		= color;
	u8g2->u8x8.next_cb = u8x8_ascii_next;
	return u8g2_draw_string(u8g2, x, y, str);
}

u8g2_uint_t u8g2_allocateBitmap(u8g2_t *u8g2, uint8_t** buff, const char *str, uint8_t utf8) {
	uint16_t width	= utf8?u8g2_GetUTF8Width(u8g2, str):u8g2_GetStrWidth(u8g2, str);
	uint16_t size = (width+7) >> 3;				// Bytes per row
	size *= u8g2->font_info.max_char_height * u8g2->scale;
	*buff = calloc(size, sizeof(uint8_t));
	if (*buff == 0) width = 0;
	return width;
}

u8g2_uint_t u8g2_StrToBitmap(u8g2_t *u8g2, uint8_t* buff, uint16_t width, const char *str, BM_ALIGN align, uint16_t margin, uint8_t utf8) {
	if (buff == 0 || width == 0 || margin >= width) return 0;
	u8g2->u8x8.next_cb = utf8?u8x8_utf8_next:u8x8_ascii_next;
	u8g2->font_decode.buff			= buff;
	u8g2->font_decode.buff_width	= width;
	uint16_t x = margin;
	if (align != align_left) {
		u8g2_uint_t w = utf8?u8g2_GetUTF8Width(u8g2, str):u8g2_GetStrWidth(u8g2, str);
		if (width > w + margin) {
			if (align == align_center) {
				x = (width - w)/2;
			} else {
				x = width - w - margin;
			}
		}
	}
	return u8g2_draw_string(u8g2, x, 0, str);
}

u8g2_uint_t u8g2_DrawUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str, uint16_t color) {
	u8g2->font_decode.buff			= 0;	// Draw string directly to the display
	u8g2->font_decode.buff_width	= 0;
	u8g2->font_decode.fg_color = color;
	u8g2->u8x8.next_cb = u8x8_utf8_next;
	return u8g2_draw_string(u8g2, x, y, str);
}

uint8_t u8g2_IsAllValidUTF8(u8g2_t *u8g2, const char *str) {
	u8g2->u8x8.next_cb = u8x8_utf8_next;
	return u8g2_is_all_valid(u8g2, str);
}

u8g2_uint_t u8g2_GetStrWidth(u8g2_t *u8g2, const char *s) {
	u8g2->u8x8.next_cb = u8x8_ascii_next;
	return u8g2_string_width(u8g2, s);
}

u8g2_uint_t u8g2_GetUTF8Width(u8g2_t *u8g2, const char *str) {
	u8g2->u8x8.next_cb = u8x8_utf8_next;
	return u8g2_string_width(u8g2, str);
}

void u8g2_SetFontRefHeightText(u8g2_t *u8g2) {
	u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_TEXT;
	u8g2_UpdateRefHeight(u8g2);
}

void u8g2_SetFontRefHeightExtendedText(u8g2_t *u8g2) {
	u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_XTEXT;
	u8g2_UpdateRefHeight(u8g2);
}

void u8g2_SetFontRefHeightAll(u8g2_t *u8g2) {
	u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_ALL;
	u8g2_UpdateRefHeight(u8g2);
}

void u8g2_SetFontPosBaseline(u8g2_t *u8g2) {
	u8g2->font_calc_vref = u8g2_font_calc_vref_font;
}

void u8g2_SetFontPosBottom(u8g2_t *u8g2) {
	u8g2->font_calc_vref = u8g2_font_calc_vref_bottom;
}

void u8g2_SetFontPosTop(u8g2_t *u8g2) {
	u8g2->font_calc_vref = u8g2_font_calc_vref_top;
}

void u8g2_SetFontPosCenter(u8g2_t *u8g2) {
	u8g2->font_calc_vref = u8g2_font_calc_vref_center;
}

int8_t	u8g2_GetMaxCharHeight(u8g2_t *u8g2) {
	return u8g2->font_info.max_char_height * u8g2->scale;
}

int8_t u8g2_GetMaxCharWidth(u8g2_t *u8g2) {
	return u8g2->font_info.max_char_width * u8g2->scale;
}

int8_t	u8g2_GetAscent(u8g2_t *u8g2) {
	return u8g2->font_ref_ascent * u8g2->scale;
}

int8_t u8g2_GetDescent(u8g2_t *u8g2) {
	return u8g2->font_ref_descent * u8g2->scale;
}

/*
 * ==================================================================
 * Local functions
 * ==================================================================
 */

static void u8g2_read_font_info(u8g2_font_info_t *font_info, const uint8_t *font) {
	uint8_t i = 0;
	/* offset 0 */
	font_info->glyph_cnt 			= font[i++];
	font_info->bbx_mode 			= font[i++];
	font_info->bits_per_0 			= font[i++];
	font_info->bits_per_1 			= font[i++];
	/* offset 4 */
	font_info->bits_per_char_width	= font[i++];
	font_info->bits_per_char_height = font[i++];
	font_info->bits_per_char_x 		= font[i++];
	font_info->bits_per_char_y 		= font[i++];
	font_info->bits_per_delta_x 	= font[i++];
	/* offset 9 */
	font_info->max_char_width		= font[i++];
	font_info->max_char_height 		= font[i++];
	font_info->x_offset				= font[i++];
	font_info->y_offset				= font[i++];
	/* offset 13 */
	font_info->ascent_A				= font[i++];
	font_info->descent_g			= font[i++];
	font_info->ascent_para 			= font[i++];
	font_info->descent_para 		= font[i++];
	/* offset 17 */
	font_info->start_pos_upper_A	= font[i] << 8 | font[i+1]; i += 2;
	font_info->start_pos_lower_a 	= font[i] << 8 | font[i+1]; i += 2;
	/* offset 21 */
	font_info->start_pos_unicode 	= font[i] << 8 | font[i+1];
}

// set ascent/descent for reference point calculation
static void u8g2_UpdateRefHeight(u8g2_t *u8g2) {
	if (u8g2->font == 0) return;

	u8g2->font_ref_ascent = u8g2->font_info.ascent_A;
	u8g2->font_ref_descent = u8g2->font_info.descent_g;
	if (u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_TEXT) {
	} else if (u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_XTEXT) {
		if (u8g2->font_ref_ascent < u8g2->font_info.ascent_para)
			u8g2->font_ref_ascent = u8g2->font_info.ascent_para;
		if (u8g2->font_ref_descent > u8g2->font_info.descent_para)
			u8g2->font_ref_descent = u8g2->font_info.descent_para;
	} else {
		if (u8g2->font_ref_ascent < u8g2->font_info.max_char_height+u8g2->font_info.y_offset)
			u8g2->font_ref_ascent = u8g2->font_info.max_char_height+u8g2->font_info.y_offset;
		if (u8g2->font_ref_descent > u8g2->font_info.y_offset)
			u8g2->font_ref_descent = u8g2->font_info.y_offset;
	}
}

/*
 * Find the starting point of the glyph data.
 * 	  Args:
 *   	encoding: Encoding (ASCII or Unicode) of the glyph
 * 	  Return:
 *   	Address of the glyph data or 0, if the encoding is not avialable in the font.
 */
static const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding) {
	const uint8_t *font = u8g2->font;
	font += U8G2_FONT_DATA_STRUCT_SIZE;

	if (encoding <= 255) {
		if (encoding >= 'a') {
			font += u8g2->font_info.start_pos_lower_a;
		} else if (encoding >= 'A') {
			font += u8g2->font_info.start_pos_upper_A;
		}

		for (;;) {
			if ( *(font + 1) == 0)
				break;
			if (*font == encoding) {
				return font+2;				// skip encoding and glyph size
			}
			font += *(font + 1);			// skip glypth
		}
	} else {								// Unicode symbol
		uint16_t e = 0;
		const uint8_t *unicode_lookup_table;
		font += u8g2->font_info.start_pos_unicode;
		unicode_lookup_table = font;
		do {
			font += u8g2_font_get_word(unicode_lookup_table, 0);
			e = u8g2_font_get_word(unicode_lookup_table, 2);
			unicode_lookup_table += 4;
		} while(e < encoding);

		for (;;) {
			e = *font;
			e <<= 8;
			e |= *(font + 1);
			if (e == 0)
				break;
			if (e == encoding) {
				return font+3;				// skip encoding and glyph size
			}
			font += *(font + 2);
		}
	}
  return 0;
}

static void u8g2_font_setup_decode(u8g2_t *u8g2, const uint8_t *glyph_data) {
	u8g2_font_decode_t *decode = &(u8g2->font_decode);
	decode->decode_ptr = glyph_data;
	decode->decode_bit_pos = 0;

	decode->glyph_width  = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
	decode->glyph_height = u8g2_font_decode_get_unsigned_bits(decode,u8g2->font_info.bits_per_char_height);
}

static uint8_t u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t *f, uint8_t cnt)  {
	uint8_t bit_pos = f->decode_bit_pos;
	uint8_t bit_pos_plus_cnt;

	uint8_t val = *(f->decode_ptr);

	val >>= bit_pos;
	bit_pos_plus_cnt = bit_pos;
	bit_pos_plus_cnt += cnt;
	if (bit_pos_plus_cnt >= 8)  {
		uint8_t s = 8;
		s -= bit_pos;
		f->decode_ptr++;
		val |= *(f->decode_ptr) << (s);
		bit_pos_plus_cnt -= 8;
	}
	val &= (1U<<cnt)-1;

	f->decode_bit_pos = bit_pos_plus_cnt;
	return val;
}

static int8_t u8g2_font_decode_get_signed_bits(u8g2_font_decode_t *f, uint8_t cnt) {
	int8_t v = (int8_t)u8g2_font_decode_get_unsigned_bits(f, cnt);
	int8_t d = 1;
	cnt--;
	d <<= cnt;
	v -= d;
	return v;
}

static u8g2_uint_t u8g2_font_draw_glyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding) {
	u8g2_uint_t dx = 0;
	u8g2->font_decode.target_x = x;
	u8g2->font_decode.target_y = y;
	const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
	if (glyph_data != 0) {
		dx = u8g2_font_decode_glyph(u8g2, glyph_data);
	}
	return dx;
}

static int8_t u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data) {
	u8g2_font_decode_t *decode = &(u8g2->font_decode);

	u8g2_font_setup_decode(u8g2, glyph_data);
	int8_t h = u8g2->font_decode.glyph_height;

	int8_t x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
	int8_t y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
	int8_t d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);

	if (decode->glyph_width > 0) {
		decode->target_x += x * u8g2->scale;
		decode->target_y -= (h+y) * u8g2->scale;

		/* reset local x/y position */
		decode->x = 0;
		decode->y = 0;

		/* decode glyph */
		for (;;) {
			uint8_t a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
			uint8_t b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
			do {
				u8g2_font_decode_len(u8g2, a, 0);
				u8g2_font_decode_len(u8g2, b, 1);
			} while (u8g2_font_decode_get_unsigned_bits(decode, 1) != 0);

			if ( decode->y >= h )
				break;
		}
	}
	return d * u8g2->scale;
}

static u8g2_uint_t u8g2_draw_string(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str) {
	u8g2_uint_t delta;
	u8g2->u8x8.utf8_state = 0;				// u8x8_utf8_init()
	u8g2_uint_t sum = 0;
	for (;;) {
		uint16_t e = u8g2->u8x8.next_cb(&u8g2->u8x8, (uint8_t)*str);
		if (e == 0x0ffff)					// Last symbol in the string
			break;
		str++;
		if (e != 0x0fffe) {
			delta = u8g2_DrawGlyph(u8g2, x, y, e);
			x 	+= delta;
			sum += delta;
		}
	}
	return sum;
}

/*
 *	Draw a run-length area of the glyph. "len" can have any size and the line
 *  length has to be wrapped at the glyph border.
 *  Args:
 *  	len: 							Length of the line
 *  	is_foreground					foreground/background?
 *   	u8g2->font_decode.target_x		X position
 *   	u8g2->font_decode.target_y		Y position
 *   	u8g2->font_decode.is_transparent	Transparent mode
 */
static void u8g2_font_decode_len(u8g2_t *u8g2, uint8_t len, uint8_t is_foreground) {
	u8g2_font_decode_t *decode = &(u8g2->font_decode);
	uint8_t cnt = len;						// total number of remaining pixels, which have to be drawn
	uint8_t lx	= decode->x;				// local coordinates of the glyph
	uint8_t ly 	= decode->y;
	uint8_t scale = u8g2->scale;
	int8_t	max_char_height = u8g2->font_info.max_char_height * scale;

	for (;;) {
		uint8_t rem = decode->glyph_width;	// remaining pixel to the right edge of the glyph
		rem -= lx;

		// calculate how many pixel to draw. This is either to the right edge
		// or lesser, if not enough pixel are left
		uint8_t current = rem;				// number of pixels, which need to be drawn for the draw procedure
		if (cnt < rem)
			current = cnt;

		// now draw the line, but apply the rotation around the glyph target position
		u8g2_uint_t x = decode->target_x;	// target position on the screen
		u8g2_uint_t y = decode->target_y;

		x += lx * scale;
		y += ly * scale;

		if (decode->buff_width > 0) {		// Decode to the buffer
			if (is_foreground && y < max_char_height) {
				for (uint8_t l = 0; l < scale; ++l) {
					u8g2_font_draw_HLine_bitmap(decode->buff, decode->buff_width, x, y+l, current * scale);
				}
			}
		} else {							// Decode directly to the display
			// draw foreground and background (if required)
			for (uint8_t l = 0; l < scale; ++l) {
				if (is_foreground) {
					TFT_DrawHLine(x,  y+l,  current * scale, decode->fg_color);
				} else if (decode->is_transparent == 0) {
					TFT_DrawHLine(x,  y+l,  current * scale, decode->bg_color);
				}
			}
		}

		// check, whether the end of the run length code has been reached
		if (cnt < rem)
			break;
		cnt -= rem;
		lx = 0;
		ly++;
	}
	lx += cnt;

	decode->x = lx;
	decode->y = ly;
}

static void u8g2_font_draw_HLine_bitmap(uint8_t* buff, uint16_t width, uint16_t x, uint16_t y, uint8_t length) {
//	if (length == 0) return;
	if (length == 0 || x >= width) return;
	if (x+length > width) length = width - x;
	uint16_t bytes_per_row	= (width+7) >> 3;
	uint32_t byte_index		= y * bytes_per_row + (x >> 3);
	uint16_t start_bit		= x & 7;
	while (length > 0) {
		uint8_t mask = (0x100 >> start_bit) - 1; // several '1'staring from start_bit to end of byte
		uint16_t end_bit = start_bit + length;
		if (end_bit < 8) {				// Line terminates in the current byte
			uint8_t clear_mask = (0x100 >> end_bit) - 1;
			mask &= ~clear_mask;		// clear tail of the byte
			length = 0;					// Last pixel
		} else {
			length -= 8 - start_bit;
		}
		buff[byte_index++] |= mask;
		start_bit = 0;					// Fill up the next byte
	}
}

static uint8_t u8g2_is_all_valid(u8g2_t *u8g2, const char *str) {
	u8g2->u8x8.utf8_state = 0;			// u8x8_utf8_init()
	for (;;) {
		uint16_t e = u8g2->u8x8.next_cb(&u8g2->u8x8, (uint8_t)*str);
		if (e == 0x0ffff)
			break;
		str++;
		if (e != 0x0fffe) {
			if (u8g2_font_get_glyph_data(u8g2, e) == 0)
				return 0;
		}
	}
	return 1;
}

static u8g2_uint_t u8g2_string_width(u8g2_t *u8g2, const char *str) {
  	u8g2->font_decode.glyph_width = 0;
  	u8g2->u8x8.utf8_state = 0;			// u8x8_utf8_init()

  	// reset the total width to zero, this will be expanded during calculation
  	u8g2_uint_t w = 0;
  	u8g2_uint_t dx = 0;

  	for (;;) {
  		uint16_t e = u8g2->u8x8.next_cb(&u8g2->u8x8, (uint8_t)*str);
  		if (e == 0x0ffff)
  			break;
  		str++;
  		if (e != 0x0fffe) {
  			dx = u8g2_GetGlyphWidth(u8g2, e); // delta x value of the glyph
  			w += dx;
  		}
  	}

  	// adjust the last glyph, check for issue #16: do not adjust if width is 0
  	if (u8g2->font_decode.glyph_width != 0) {
  		w -= dx;
  		w += u8g2->font_decode.glyph_width;	// the real pixel width of the glyph, sideeffect of GetGlyphWidth
  		w += u8g2->glyph_x_offset;			// this value is set as a side effect of u8g2_GetGlyphWidth()
  	}
  return w * u8g2->scale;
}


static uint16_t u8g2_font_get_word(const uint8_t *font, uint8_t offset) {
	return *(font+offset) << 8 | *(font+offset+1);
}

static uint16_t u8x8_ascii_next(u8x8_t *u8x8, uint8_t b) {
	if ( b == 0 || b == '\n' )			// '\n' terminates the string to support the string list procedures
		return 0x0ffff;					// end of string detected
	return b;
}

/*
  pass a byte from an utf8 encoded string to the utf8 decoder state machine
  returns
    0x0fffe: no glyph, just continue
    0x0ffff: end of string
    anything else: The decoded encoding
*/
static uint16_t u8x8_utf8_next(u8x8_t *u8x8, uint8_t b) {
	if ( b == 0 || b == '\n' )			// '\n' terminates the string to support the string list procedures
		return 0x0ffff;					// end of string detected, pending UTF8 is discarded
	if (u8x8->utf8_state == 0) {
		if (b >= 0xfc) {				// 6 byte sequence
			u8x8->utf8_state = 5;
			b &= 1;
		} else if ( b >= 0xf8 ) {
			u8x8->utf8_state = 4;
			b &= 3;
		} else if ( b >= 0xf0 ) {		// 4 byte sequence
			u8x8->utf8_state = 3;
			b &= 7;
		} else if ( b >= 0xe0 ) {		// 3 bytes
			u8x8->utf8_state = 2;
			b &= 15;
		} else if ( b >= 0xc0 ) {		// 2 bytes
			u8x8->utf8_state = 1;
			b &= 0x01f;					// 5 bits from first byte in sequence
		} else {
			return b;					// do nothing, just use the value as encoding
		}
		u8x8->encoding = b;
		return 0x0fffe;
	} else {
		u8x8->utf8_state--;
		// The case b < 0x080 (an illegal UTF8 encoding) is not checked here.
		u8x8->encoding <<= 6;
		b &= 0x03f;
		u8x8->encoding |= b;
		if (u8x8->utf8_state != 0)
			return 0x0fffe;				// nothing to do yet
  }
  return u8x8->encoding;
}

/*
 *  callback procedures to correct the y position
 */

static u8g2_uint_t u8g2_font_calc_vref_font(u8g2_t *u8g2) {
	return 0;
}

static u8g2_uint_t u8g2_font_calc_vref_bottom(u8g2_t *u8g2) {
	return (u8g2_uint_t)(u8g2->font_ref_descent);
}

static u8g2_uint_t u8g2_font_calc_vref_top(u8g2_t *u8g2) {
  /* reference pos is one pixel above the upper edge of the reference glyph */
	u8g2_uint_t tmp = (u8g2_uint_t)(u8g2->font_ref_ascent);
	tmp++;
	return tmp;
}

static u8g2_uint_t u8g2_font_calc_vref_center(u8g2_t *u8g2) {
	int8_t tmp = u8g2->font_ref_ascent;
	tmp -= u8g2->font_ref_descent;
	tmp /= 2;
	tmp += u8g2->font_ref_descent;
	return tmp;
}
