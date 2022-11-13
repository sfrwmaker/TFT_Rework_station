/*
 * sdload.cpp
 *
 */

#include "sdload.h"
#include "jsoncfg.h"

t_msg_id SDLOAD::load(void) {
	t_msg_id e = init();
	if (MSG_LAST != e)
		return e;
	if (!allocateCopyBuffer())
		return MSG_SD_MEMORY;
	uint8_t l = copyLanguageData();
	umountAll();
	if (buffer) {											// Deallocate copy buffer memory
		free(buffer);
		buffer_size = 0;
	}
	return (l>0)?MSG_LAST:MSG_SD_INCONSISTENT;
}

t_msg_id SDLOAD::init(void) {
	if (FR_OK != f_mount(&sdfs, "1:/", 1))
		return MSG_SD_MOUNT;
	std::string cfg_path = "1:" + std::string(fn_cfg);
	if (FR_OK != f_open(&cfg_f, cfg_path.c_str(), FA_READ)) {
		f_mount(NULL, "1:/", 0);
		return MSG_SD_NO_CFG;
	}
	lang_cfg.readConfig(&cfg_f);
	lang_list = lang_cfg.getLangList();
	if (lang_list->empty()) {
		f_mount(NULL, "1:/", 0);
		return MSG_SD_NO_LANG;
	}

	if (FR_OK != f_mount(&flashfs, "0:/", 1)) {
		f_mount(NULL, "1:/", 0);
		return MSG_EEPROM_WRITE;
	}
	return MSG_LAST;										// Everything OK, No error message!
}

void SDLOAD::umountAll(void) {
	f_mount(NULL, "0:/", 0);
	f_mount(NULL, "1:/", 0);
}

bool SDLOAD::allocateCopyBuffer(void) {
	for (uint8_t i = 0; i < 3; ++i) {
		buffer = (uint8_t *)malloc(b_sizes[i]);
		if (buffer) {										// Successfully allocated
			buffer_size = b_sizes[i];
			return true;
		}
	}
	return false;
}

uint8_t SDLOAD::copyLanguageData(void) {
	uint8_t l_copied = 0;
	while (!lang_list->empty()) {
		t_lang_cfg lang = lang_list->back();
		lang_list->pop_back();
		bool lang_ok = isLanguageDataConsistent(lang);		// Check the language files exist
		if (lang_ok)
			lang_ok = copyFile(lang.messages_file);
		if (lang_ok)
			lang_ok = copyFile(lang.font_file);
		if (lang_ok)
			++l_copied;
	}
	if (l_copied > 0) {
		std::string cfg_name = fn_cfg;
		if (copyFile(cfg_name))
			return l_copied;
	}
	return 0;
}

bool SDLOAD::isLanguageDataConsistent(t_lang_cfg &lang_data) {
	std::string file_path = "1:" + lang_data.messages_file;
	FILINFO fno;
	// Checking message file
	if (FR_OK != f_stat(file_path.c_str(), &fno))
		return false;
	if (fno.fsize == 0 || (fno.fattrib & AM_ARC) == 0)
		return false;
	// Checking font file
	file_path = "1:" + lang_data.font_file;
	if (FR_OK != f_stat(file_path.c_str(), &fno))
		return false;
	if (fno.fsize == 0 || (fno.fattrib & AM_ARC) == 0)
		return false;
	return true;
}

bool SDLOAD::haveToUpdate(std::string &name) {
	FILINFO fno;
	std::string file_path = "1:" + name;					// Source file path on SD-CARD
	if (FR_OK != f_stat(file_path.c_str(), &fno))			// Failed to get info of the new file
		return true;
	uint32_t source_date = fno.fdate << 16 | fno.ftime;		// The source file timestamp
	file_path = "0:" + name;								// Destination file path on SPI FLASH
	if (FR_OK != f_stat(file_path.c_str(), &fno))			// Failed to get info of the file, perhaps, the destination file does not exist
		return true;
	if (fno.fsize == 0 || (fno.fattrib & AM_ARC) == 0)		// Destination file size is zero or is not a n archive file at all
		return false;
	uint32_t dest_date = fno.fdate << 16 | fno.ftime;		// The destination file timestamp
	return (dest_date < source_date);						// If the destination file timestamp is older than source one
}

bool SDLOAD::copyFile(std::string &name) {
	FIL	sf, df;												// Source and destination file descriptors
	if (!haveToUpdate(name)) return true;					// The file already exists on the SPI FLASH and its date is not older than the source file on SD-CARD
	if (!buffer || buffer_size == 0)						// Here the copy buffer has to be allocated already, but double check it
		return false;
	std::string s_file_path = "1:" + name;					// Source file path on SD-CARD
	if (FR_OK != f_open(&sf, s_file_path.c_str(), FA_READ))	// Failed to open source file for reading
		return false;
	std::string d_file_path = "0:" + name;								// Destination file path on SPI FLASH
	if (FR_OK != f_open(&df, d_file_path.c_str(), FA_CREATE_ALWAYS | FA_WRITE)) // Failed to create destination file for writing
		return false;
	bool copied = true;
	while (true) {											// The file copy loop
		UINT br = 0;										// Read bytes
		f_read(&sf, (void *)buffer, (UINT)buffer_size, &br);
		if (br == 0)										// End of source file
			break;
		UINT written = 0;									// Written bytes
		f_write(&df, (void *)buffer, br, &written);
		if (written != br) {
			copied = false;
			break;
		}
	}
	f_close(&df);
	f_close(&sf);
	if (!copied) {
		f_unlink(d_file_path.c_str());						// Remove destination file in case on any error
	} else {												// Copy date and time from the source file to the destination file
		FILINFO fno;
		if (FR_OK == f_stat(s_file_path.c_str(), &fno)) {
			f_utime(d_file_path.c_str(), &fno);
		}
	}
	return copied;
}
