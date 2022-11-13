/*
 * nls_cfg.h
 *
 */

#ifndef NLS_CFG_H_
#define NLS_CFG_H_

#include <string>
#include "jsoncfg.h"
#include "nls.h"
#include "ff.h"
#include "vars.h"

typedef std::vector<std::string> tLangList;

class NLS {
	public:
		NLS(void)											{ }
		void			init(NLS_MSG *pMsg);
		uint8_t			numLanguages(void)					{ return lang_cfg.listSize();	}
		uint8_t			languageIndex(void)					{ return language_index;		}
		uint8_t*		font(void)							{ return font_data;				}
		void			loadLanguageData(const char *language);
		void			loadLanguageData(uint8_t index);
		void			defaultNLS();
		std::string		languageName(uint8_t index);
	private:
		uint8_t			index(const char *lang);
		std::string		messageFile(uint8_t index);
		std::string		fontFile(uint8_t index);
		bool			loadFont(uint8_t indx);
		bool			loadMessages(uint8_t indx);
		FATFS			flashfs;
		FIL				cfg_f;
		JSON_LANG_CFG	lang_cfg;
		JSON_MESSAGES	msg_parser;
		uint8_t			language_index	= 0;				// Current language index
		uint8_t			*font_data		= 0;				// Loaded font data, memory allocated by malloc()
		const TCHAR*	fn_cfg			= nsl_cfg;			// vars.h
};

#endif
