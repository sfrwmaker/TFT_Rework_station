/*
 * TFT.h
 *
 *  Created on: 4 Nov 2022
 *      Author: Alex
 *
 *  2024 MAY 22
 *  	Moved the bitmap & pixmap methods inside corresponding classes
 *
 *  2024 AUG 02
 *  	Added ILI9341v support, i.e. tft_ILI9341v class
 */

#ifndef _TFT_H_
#define _TFT_H_

#include "common.h"
#include "u8g_font.h"
#include "picture.h"
#include "ILI9341.h"
#include "ILI9488.h"
#include "ST7735.h"
#include "ST7796.h"
#include "NT35510.h"
#include "SSD1963.h"


#ifdef __cplusplus
#include "bitmap.h"
#include "pixmap.h"
#include "region.h"

#define BUFFER_SIZE 	(32)

class u8gFont {
	public:
		u8gFont(void)										{ u8g2_u8gFont(&u8g); 											}
		void		setFont(const uint8_t *font)			{ u8g2_SetFont(&u8g, font);										}
		void		setFontMode(uint8_t is_transparent, uint16_t bg_color)
															{ u8g2_SetFontMode(&u8g, is_transparent, bg_color);				}
		uint8_t		isGlyph(uint16_t requested_encoding)	{ return u8g2_IsGlyph(&u8g, requested_encoding);				}
		int8_t		getGlyphWidth(uint16_t requested_encoding)
															{ return u8g2_GetGlyphWidth(&u8g, requested_encoding);			}
		u8g2_uint_t drawStr(u8g2_uint_t x, u8g2_uint_t y, const char *str, uint16_t color)
															{ return u8g2_DrawStr(&u8g, x, y, str, color);					}
		u8g2_uint_t drawUTF8(u8g2_uint_t x, u8g2_uint_t y, const char *str, uint16_t color)
															{ return u8g2_DrawUTF8(&u8g, x, y, str, color); 				}

		uint8_t		isAllValidUTF8(const char *str)			{ return u8g2_IsAllValidUTF8(&u8g, str);						}
		u8g2_uint_t	strToBitmap(BITMAP& bm, const char *str, BM_ALIGN align = align_left, uint16_t margin = 0, bool utf8 = false)
															{ return u8g2_StrToBitmap(&u8g, bm.bitmap(), bm.width(), str, align, margin, (uint8_t)utf8);	}
		u8g2_uint_t getStrWidth(const char *s)				{ return u8g2_GetStrWidth(&u8g, s);								}
		u8g2_uint_t getUTF8Width(const char *str)			{ return u8g2_GetUTF8Width(&u8g, str);							}
		uint8_t		getMaxCharWidth(void)					{ return u8g2_GetMaxCharWidth(&u8g);							}
		int8_t		getMaxCharHeight(void)					{ return u8g2_GetMaxCharHeight(&u8g);							}
		int16_t		getFontHeight(void)						{ return u8g2_GetMaxCharHeight(&u8g) - u8g2_GetDescent(&u8g); 	}
		int16_t		getFontTopOffset(void)					{ return u8g2_GetMaxCharHeight(&u8g) + u8g2_GetDescent(&u8g); 	}
		void 		setFontRefHeightText(void)				{ u8g2_SetFontRefHeightText(&u8g);								}
		void 		setFontRefHeightExtendedText(void)		{ u8g2_SetFontRefHeightExtendedText(&u8g);						}
		void 		setFontRefHeightAll(void)				{ u8g2_SetFontRefHeightAll(&u8g);								}
		void 		setFontPosBaseline(void)				{ u8g2_SetFontPosBaseline(&u8g);								}
		void 		setFontPosBottom(void)					{ u8g2_SetFontPosBottom(&u8g);									}
		void 		setFontPosTop(void)						{ u8g2_SetFontPosTop(&u8g);										}
		void 		setFontPosCenter(void)					{ u8g2_SetFontPosCenter(&u8g);									}
		void		setFontScale(uint8_t scale) 			{ u8g2_SetFontScale(&u8g, scale);								}
		uint8_t		getFontScale(void)						{ return u8g2_GetFontScale(&u8g);								}
		u8g2_t		u8g;
};

class tft : public u8gFont {
	public:
		tft()		: u8gFont()								{ }
		virtual			~tft()								{ }
		virtual	void	init(void)							{ }
		virtual void	sleepIn(void)						{ TFT_DEF_SleepIn();											}
		virtual void 	sleepOut(void)						{ TFT_DEF_SleepOut();											}
		virtual void	invertDisplay(bool on)				{ TFT_DEF_InvertDisplay(on);									}
		virtual void	on(bool on)							{ TFT_DEF_DisplayOn(on);										}
		virtual void	idleMode(bool on)					{ TFT_DEF_IdleMode(on);											}

		uint16_t 	width(void)								{ return TFT_Width();											}
		uint16_t 	height(void)							{ return TFT_Height();											}
		void		setRotation(tRotation rotation)			{ TFT_SetRotation(rotation);									}
		tRotation 	rotation(void)							{ return TFT_Rotation();										}
		void 		fillScreen(uint16_t color)				{ TFT_FillScreen(color);										}
		void		drawPixel(uint16_t x,  uint16_t y, uint16_t color)
															{ TFT_DrawPixel(x, y, color); 									}
		void 		drawHLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color)
															{ TFT_DrawHLine(x, y, length, color);							}
		void 		drawVLine(uint16_t x, uint16_t y, uint16_t length, uint16_t color)
															{ TFT_DrawVLine(x, y, length, color);							}
		void 		drawFilledRect(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color)
															{ TFT_DrawFilledRect(x0,  y0, width, height, color);			}
		void		drawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
															{ TFT_DrawRect(x, y, width, height, color);						}
		void		drawLine(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
															{ TFT_DrawLine(x0, y0, x1,y1, color);							}
		void 		drawCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color)
															{ TFT_DrawCircle(x, y, radius, color);							}
		void		drawFilledCircle(uint16_t x, uint16_t y, uint8_t radius, uint16_t color)
															{ TFT_DrawFilledCircle(x, y, radius, color);					}
		void		drawVarThickLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, LineThickness thickness, uint16_t color)
															{ TFT_DrawVarThickLine( x0, y0, x1, y1, thickness, color); 		}
		void 		drawThickLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t thickness, uint16_t color)
															{ TFT_DrawThickLine(x0, y0, x1, y1, thickness, color); 			}
		void		drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
															{ TFT_DrawTriangle(x0, y0, x1, y1, x2, y2, color); 				}
		void		drawFilledTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
															{ TFT_DrawfilledTriangle(x0, y0, x1, y1, x2, y2, color); 		}
		void 		drawEllipce(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color)
															{ TFT_DrawEllipse(x0, y0, rx, ry, color);						}
		void		drawFilledEllipce(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color)
															{ TFT_DrawFilledEllipse(x0, y0, rx, ry, color); 				}
		void 		drawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
															{ TFT_DrawRoundRect(x, y, w, h, r, color);						}
		void 		drawFilledRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
															{ TFT_DrawFilledRoundRect(x, y, w, h, r, color); 				}
		void		drawArea(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height, t_NextPixel nextPixelCB)
															{ TFT_DrawArea(x0, y0, area_width, area_height, nextPixelCB); }
		void		drawIcon(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height, const uint8_t *bitmap, uint16_t bm_width, uint16_t bg_color, uint16_t fg_color)
															{ TFT_DrawBitmap(x0, y0, area_width, area_height, bitmap, bm_width, bg_color, fg_color); }
		void		drawBitmapArea(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height, BITMAP& bm, uint16_t bg_color, uint16_t fg_color)
															{ TFT_DrawBitmap(x0, y0, area_width, area_height, bm.bitmap(), bm.width(), bg_color, fg_color); }
		void		drawBitmap(uint16_t x0, uint16_t y0, BITMAP& bm, uint16_t bg_color, uint16_t fg_color)
															{ TFT_DrawBitmap(x0, y0, bm.width(), bm.height(), bm.bitmap(), bm.width(), bg_color, fg_color); }
		void 		drawScrolledBitmap(uint16_t x0, uint16_t y0, uint16_t area_width, BITMAP& bm, int16_t offset, uint8_t gap, uint16_t bg_color, uint16_t fg_color)
															{ TFT_DrawScrolledBitmap(x0, y0, area_width, bm.height(), bm.bitmap(), bm.width(), offset, gap, bg_color, fg_color);}
		void 		drawPixmap(uint16_t x0, uint16_t y0, uint16_t area_width, uint16_t area_height,	PIXMAP& pm)
															{ TFT_DrawPixmap(x0, y0, area_width, area_height, pm.pixmap(), pm.width(), pm.depth(), pm.palette()); }
		uint16_t 	color(uint8_t red, uint8_t green, uint8_t blue)
															{ return TFT_Color(red, green, blue);							}
		uint16_t 	wheelColor(uint8_t wheel_pos)			{ return TFT_WheelColor(wheel_pos);								}
		void		touchAdjustRoration(uint16_t *x, uint16_t *y)
															{ TFT_Touch_Adjust_Rotation_XY(x, y);							}
#ifdef TFT_BMP_JPEG_ENABLE
		uint32_t	BMPsize(const char *filename)			{ return TFT_BMPsize(filename);				 					}
		bool		drawBMP(const char *filename, uint16_t bmp_x, uint16_t bmp_y)
															{ return TFT_ClipBMP(filename, 0, 0, TFT_Width(), TFT_Height(), bmp_x, bmp_y); }
		bool		clipBMP(const char *filename, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t bmp_x, uint16_t bmp_y)
															{ return TFT_ClipBMP(filename, x, y, width, height, bmp_x, bmp_y); }
		bool		drawIconOverBMP(const char *filename, uint16_t bmp_x, uint16_t bmp_y, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
						const uint8_t *bitmap, uint16_t bm_width, uint16_t txt_color)
															{ return TFT_ScrollBitmapOverBMP(filename, bmp_x, bmp_y, x, y, width, height,
																bitmap, bm_width, 0, 0, txt_color); 						}
		bool		drawBitmapOverBMP(const char *filename, uint16_t bmp_x, uint16_t bmp_y, uint16_t x, uint16_t y, uint16_t width, uint16_t height, BITMAP &bm, uint16_t txt_color)
															{ return TFT_ScrollBitmapOverBMP(filename, bmp_x, bmp_y, x, y, width, height,
																		bm.bitmap(), bm.width(), 0, 0, txt_color); 			}
		bool		scrollBitmapOverBMP(const char *filename, uint16_t bmp_x, uint16_t bmp_y, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
						BITMAP &bm, int16_t offset, uint8_t gap, uint16_t txt_color)
															{ return TFT_ScrollBitmapOverBMP(filename, bmp_x, bmp_y, x, y, width, height,
																bm.bitmap(), bm.width(), offset, gap, txt_color); 			}
		bool		drawJPEG(const char *filename, int16_t x, int16_t y)
															{ return TFT_DrawJPEG(filename, x, y);							}
		bool 		clipJPEG(const char *filename, int16_t x, int16_t y, uint16_t area_x, uint16_t area_y, uint16_t area_width, uint16_t area_height)
															{ return TFT_ClipJPEG(filename, x, y, area_x, area_y, area_width, area_height);}
// drawJPEG() allocates and frees buffer for jpeg processing in memory automatically for each file if this buffer was not allocated before
		bool		jpegAllocate(void)						{ return TFT_jpegAllocate();									}
		void		jpegDeallocate(void)					{ TFT_jpegDeallocate();											}
#endif
};

class tft_ST7735: public tft {
	public:
		tft_ST7735()	: tft()								{ }
		virtual			~tft_ST7735()						{ }
		virtual	void	init(void)							{ ST7735_Init();												}
};

class tft_ST7796: public tft {
	public:
		tft_ST7796()	: tft()								{ }
		virtual			~tft_ST7796()						{ }
		virtual	void	init(void)							{ ST7796_Init();												}
};

class tft_ILI9341: public tft {
	public:
		tft_ILI9341()	: tft()								{ }
		virtual 		~tft_ILI9341()						{ }
		virtual	void	init(void)							{ ILI9341_Init();												}
		void			initIPS(void)						{ ILI9341v_Init();												}
};

class tft_ILI9488: public tft {
	public:
		tft_ILI9488()	: tft()								{ }
		virtual			~tft_ILI9488()						{ }
		virtual	void	init(void)							{ ILI9488_Init();												}
		void			initIPS(void)						{ ILI9488_IPS_Init();											}
};

class tft_NT35510: public tft {
	public:
		tft_NT35510()	: tft()								{ }
		virtual			~tft_NT35510()						{ }
		virtual	void	init(void)							{ NT35510_Init();												}
};

class tft_SSD1963: public tft {
	public:
		tft_SSD1963()	: tft()								{ }
		virtual			~tft_SSD1963()						{ }
		virtual	void	init(void)							{ SSD1963_Init();												}
};
#endif	// __cplusplus

#endif
