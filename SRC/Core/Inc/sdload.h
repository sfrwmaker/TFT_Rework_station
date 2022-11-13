/*
 * sdload.h
 *
 */

#ifndef SDLOAD_H_
#define SDLOAD_H_

#include "jsoncfg.h"
#include "ff.h"
#include "vars.h"
#include "sdspi.h"

extern SDCARD sd;

class SDLOAD {
	public:
		SDLOAD(void)										{ }
		t_msg_id	load(void);								// Returns number of languages loaded
		uint8_t		sdStatus(void)							{ return sd.init_status; } // SD status initialized by SD_Init() function (see sdspi.c)
	private:
		t_msg_id	init(void);
		uint8_t		copyLanguageData(void);
		void		umountAll(void);
		bool		allocateCopyBuffer(void);
		bool		isLanguageDataConsistent(t_lang_cfg &lang_data);
		bool		haveToUpdate(std::string &name);
		bool		copyFile(std::string &name);
		uint8_t		*buffer		= 0;						// The buffer to copy the file, allocated later
		uint16_t	buffer_size	= 0;						// The allocated buffer size
		FATFS			sdfs, flashfs;
		FIL				cfg_f;
		JSON_LANG_CFG	lang_cfg;
		t_lang_list		*lang_list = 0;
		const TCHAR*	fn_cfg			= nsl_cfg;			// vars.h
		const uint16_t	b_sizes[3] = { 4096, 1024, 512};	// Possible copy buffer sizes
};

#endif
