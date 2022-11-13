/*
 * nls_cfg.cpp
 *
 */

#include <string.h>
#include "nls_cfg.h"

void NLS::init(NLS_MSG *pMsg) {
	msg_parser.setNLS_MSG(pMsg);							// Setup pointer to the NLS_MSG class instance to use NLS_MSG::set() method in the value callback procedure
	if (FR_OK == f_mount(&flashfs, "0:/", 1)) {				// Try to mount SPI flash
		std::string cfg_path = "0:" + std::string(fn_cfg);	// fn_cfg defined in vars.h, "cfg.json"
		if (FR_OK == f_open(&cfg_f, cfg_path.c_str(), FA_READ)) {
			lang_cfg.readConfig(&cfg_f);					// readConfig closes the file automatically
		}
	}
	lang_cfg.addEnglish();									// Add default language to the list
	language_index	= 0;									// Current language index (English)
	font_data		= 0;									// No font loaded
}

void NLS::loadLanguageData(uint8_t index) {
	if (index == language_index)							// The language is already loaded
		return;
	std::string msg_file = messageFile(index);
	if (msg_file.empty()) {									// No message file for this language, load default messages
		defaultNLS();
		return;
	}
	defaultNLS();											// Free font data
	if (loadFont(index)) {
		if (loadMessages(index)) {							// Both font and messages are loaded successfully
			language_index = index;
		}
	}
}

void NLS::loadLanguageData(const char *language) {
	uint8_t indx = index(language);
	loadLanguageData(indx);
}

void NLS::defaultNLS() {
	if (font_data) {
		free(font_data);
		font_data			= 0;
		language_index		= 0;
	}
}

std::string NLS::languageName(uint8_t index) {
	uint8_t num_lang = lang_cfg.listSize();					// Number of loaded languages
	if (index < num_lang){
		t_lang_list *ll = lang_cfg.getLangList();
		return ll->at(index).lang;
	}
	return std::string("");
}

uint8_t NLS::index(const char *lang) {
	uint8_t num_lang = lang_cfg.listSize();					// Number of loaded languages
	for (uint8_t i = 0; i < num_lang; ++i) {
		std::string l = languageName(i);
		if (strncmp(lang, l.c_str(), LANG_LENGTH) == 0) {	// lang_length defined in vars.h
			return i;
		}
	}
	return 0;
}

std::string NLS::messageFile(uint8_t index) {
	uint8_t num_lang = lang_cfg.listSize();					// Number of loaded languages
	if (index < num_lang){
		t_lang_list *ll = lang_cfg.getLangList();
		return ll->at(index).messages_file;
	}
	return 0;
}

std::string NLS::fontFile(uint8_t index) {
	uint8_t num_lang = lang_cfg.listSize();					// Number of loaded languages
	if (index < num_lang){
		t_lang_list *ll = lang_cfg.getLangList();
		return ll->at(index).font_file;
	}
	return 0;
}

bool NLS::loadFont(uint8_t indx) {
	if (FR_OK != f_mount(&flashfs, "0:/", 1))				// Try to mount SPI flash
		return false;
	std::string f = fontFile(indx);
	if (f.empty())
		return true;
	std::string cfg_path = "0:" + f;
	FILINFO fno;
	if (FR_OK != f_stat(cfg_path.c_str(), &fno))
		return false;
	if (fno.fsize == 0 || (fno.fattrib & AM_ARC) == 0)
		return false;
	if (FR_OK != f_open(&cfg_f, cfg_path.c_str(), FA_READ))
		return false;
	font_data = (uint8_t *)malloc(fno.fsize);				// Try to allocate memory for the font
	if (!font_data) {
		f_close(&cfg_f);
		return false;
	}
	UINT br = 0;											// Read bytes
	f_read(&cfg_f, (void *)font_data, (UINT)fno.fsize, &br);
	f_close(&cfg_f);
	if (br != fno.fsize) {
		free(font_data);
		font_data = 0;
		return false;
	}
	return true;
}

bool NLS::loadMessages(uint8_t indx) {
	if (FR_OK != f_mount(&flashfs, "0:/", 1))				// Try to mount SPI flash
		return false;
	std::string cfg_path = "0:" + messageFile(indx);		// Here messageFile is not null for sure
	if (FR_OK != f_open(&cfg_f, cfg_path.c_str(), FA_READ))
		return false;
	msg_parser.readConfig(&cfg_f);							// readConfig closes the file automatically
	return true;
}

