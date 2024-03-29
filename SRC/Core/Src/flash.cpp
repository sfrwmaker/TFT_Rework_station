/*
 * flash.cpp
 *
 *
 * 2024 MAR 28
 *     Changed W25Q::init(). In case if no cfg file read, do not unmount the FLASH
 *     Added comments to the W25Q::formatFlashDrive()
 */
#include <string.h>
#include "flash.h"
#include "W25Qxx.h"

FATFS	fs;

FLASH_STATUS W25Q::init(void) {
	if (!W25Qxx_Init()) return FLASH_ERROR;
	if (!mount())		return FLASH_NO_FILESYSTEM;

	uint16_t	good_tips = 0;
	if (FR_OK == f_open(&cfg_f, fn_tip_calib, FA_READ)) {	// Check the tip calibration data
		while (true) {										// Read all tip calibration data
			UINT	br = 0;
			TIP		tmp_tip;
			f_read(&cfg_f, (void *)&tmp_tip, (UINT)sizeof(TIP), &br);
			if (br ==  (UINT)sizeof(TIP)) {
				if (TIP_checkSum(&tmp_tip, false)) {		// CRC of the tip record is correct
					++good_tips;
				}
			} else {
				break;										// File is over
			}
		}
		f_close(&cfg_f);
	}
	if (good_tips == 0) {									// Not tip loaded, try the backup file
		FILINFO fno;
		if (FR_OK == f_stat(fn_tip_backup, &fno)) {			// There is the backup file exists
			f_unlink(fn_tip_calib);
			f_rename(fn_tip_backup, fn_tip_calib);
		}
	}
	return FLASH_OK;
}

bool W25Q::reset() {
	return W25Qxx_Init();
}

bool W25Q::mount(void) {
	if (act_f == W25Q_NOT_MOUNTED) {
		// Try to mount flash
		FRESULT res = f_mount(&fs, "0:/", 1);
		if (res == FR_OK) {
			uint32_t free_clust;
			FATFS* fs_ptr = &fs;
			res = f_getfree("", &free_clust, &fs_ptr);
			if (res != FR_OK) {
				f_mount(NULL, "", 0);						// unmount file system
				return false;
			}
			act_f = W25Q_NONE;
			return true;
		}
		return false;										// failed to mount file system
	}
	return true;
}

void W25Q::umount(void) {
	W25Q::close();
	f_mount(NULL, "", 0);									// unmount file system
	act_f = W25Q_NOT_MOUNTED;
}

void W25Q::close(void) {
	if (act_f == W25Q_NOT_MOUNTED || act_f == W25Q_NONE)
		return;
	f_close(&cfg_f);
	act_f = W25Q_NONE;
}

bool W25Q::loadRecord(RECORD* config_record) {
	if (!mount())
		return false;
	W25Q::close();
	UINT br = 0;
	bool ret = false;
	RECORD tmp_record;
	if (FR_OK == f_open(&cfg_f, fn_cfg, FA_READ | FA_OPEN_EXISTING)) {
		f_read(&cfg_f, (void *)&tmp_record, (UINT)sizeof(RECORD), &br);
		if (br ==  (UINT)sizeof(RECORD)) {
			if (CFG_checkSum(&tmp_record, false)) {
				memcpy((void *)config_record, (void *)&tmp_record, sizeof(RECORD));
				ret = true;
			}
		}
		f_close(&cfg_f);
	}
	if (!ret) {												// Failed to load configuration record from main file
		if (FR_OK == f_open(&cfg_f, fn_cfg_backup, FA_READ | FA_OPEN_EXISTING)) {
			f_read(&cfg_f, (void *)&tmp_record, (UINT)sizeof(RECORD), &br);
			if (br == (UINT)sizeof(RECORD)) {
				if (CFG_checkSum(&tmp_record, false)) {
					memcpy((void *)config_record, (void *)&tmp_record, sizeof(RECORD));
					ret = true;
				}
			}
			f_close(&cfg_f);
			if (ret) {										// Backup is valid, revert to the backup config
				f_unlink(fn_cfg);
				f_rename(fn_cfg_backup, fn_cfg);
			} else {										// Backup config is invalid, remove it
				f_unlink(fn_cfg_backup);
			}
		}
	}
	umount();
	return ret;
}

bool W25Q::saveRecord(RECORD* config_record) {
	if (!mount())
		return false;
	W25Q::close();
	backup(W25Q_CONFIG_CURRENT);
	CFG_checkSum(config_record, true);
	bool ret = false;
	if (FR_OK == f_open(&cfg_f, fn_cfg, FA_CREATE_ALWAYS | FA_WRITE)) {
		UINT written = 0;
		f_write(&cfg_f, (void *)config_record, sizeof(RECORD), &written);
		ret = (written == sizeof(RECORD));
		f_close(&cfg_f);
	}
	umount();
	return ret;
}

// Load tip configuration data from file
TIP_IO_STATUS W25Q::loadTipData(TIP* tip, uint8_t tip_index, bool keep) {
	if (!mount())											// Cannot mount W25Qxx flash
		return TIP_IO;
	if (act_f != W25Q_TIPS_CURRENT) {						// Close other configuration file
		close();
		if (FR_OK == f_open(&cfg_f, fn_tip_calib, FA_READ)) {
			act_f = W25Q_TIPS_CURRENT;
		}
	}
	if (act_f != W25Q_TIPS_CURRENT)
		return returnStatus(keep, TIP_IO);

	if (FR_OK != f_lseek(&cfg_f, tip_index * sizeof(TIP))) { // Invalid tip index
		return returnStatus(keep, TIP_INDEX);
	}
	// Read tip calibration data
	UINT	br = 0;
	TIP		tmp_tip;
	f_read(&cfg_f, (void *)&tmp_tip, (UINT)sizeof(TIP), &br);
	if (br == (UINT)sizeof(TIP)) {
		if (TIP_checkSum(&tmp_tip, false)) {				// CRC of the tip record is correct
			memcpy((void *)tip, (const void *)&tmp_tip, sizeof(TIP)); // Copy the tip record from the data buffer
			return returnStatus(keep, TIP_OK);
		}
		return returnStatus(keep, TIP_CHECKSUM);
	}
	return returnStatus(keep, TIP_IO);
}

// Return tip index in the file or -1 if error
int16_t W25Q::saveTipData(TIP* tip, bool keep) {
	if (!mount())
		return -1;
	bool new_entry = false;
	if (act_f == W25Q_TIPS_CURRENT) {						// The tip configuration file is already opened
		f_lseek(&cfg_f, 0);									// Rewind to the top of the file
	} else {
		W25Q::close();
		backup(W25Q_TIPS_CURRENT);
		if (FR_OK != f_open(&cfg_f, fn_tip_calib, FA_WRITE | FA_READ | FA_OPEN_EXISTING)) {
			if (FR_OK == f_open(&cfg_f, fn_tip_calib, FA_CREATE_NEW | FA_WRITE)) {
				new_entry = true;
			} else {
				return -1;
			}

		}
		act_f = W25Q_TIPS_CURRENT;
	}
	if (!new_entry) {										// Try to locate our tip in the file
		UINT	br = 0;										// Bytes actually read from the file
		TIP		tmp_tip;
		while(true) {										// Looking for the tip
			f_read(&cfg_f, (void *)&tmp_tip, (UINT)sizeof(TIP), &br);
			if (br == (UINT)sizeof(TIP)) {
				if (strncmp(tip->name, tmp_tip.name, tip_name_sz) == 0) {
					f_lseek(&cfg_f, cfg_f.fptr-sizeof(TIP));
					break;
				}
			} else {										// Tip not calibrated
				break;
			}
		}
	}
	// Update or add new tip information
	int16_t tip_index = cfg_f.fptr / sizeof(TIP);
	TIP_checkSum(tip, true);								// calculate CRC inside the data buffer
	UINT	written = 0;
	f_write(&cfg_f, (void *)tip, sizeof(TIP), &written);
	if (written != sizeof(TIP))
		tip_index = -1;
	if (!keep) {
		f_close(&cfg_f);									// Close file for sure
		W25Q::umount();
	}
	return tip_index;
}

bool W25Q::formatFlashDrive(void) {
	MKFS_PARM p;
	p.fmt		= FM_FAT | FM_SFD;							// No partition table
	p.au_size	= 4096;										// W25Qxx sector size
	p.align		= 0;										// 8
	p.n_fat		= 1;										// Number of FAT copies
	p.n_root	= 128;										// 32 bytes per entry, 4096 bytes, 1 sector!

	uint8_t *buff = (uint8_t *)malloc(blk_size);
	if (buff == 0)
		return false;
	bool ret = (FR_OK == f_mkfs("0:/", &p, buff, blk_size));
	free(buff);
	return ret;
}

bool W25Q::clearTips(void) {
	if (!mount())
		return false;
	f_unlink(fn_tip_calib);
	f_unlink(fn_tip_backup);
	umount();
	return true;
}

// Remove configuration files
bool W25Q::clearConfig(void) {
	if (FR_OK == f_mount(&fs, "0:/", 1)) {
		f_unlink(fn_cfg);
		f_unlink(fn_cfg_backup);
		umount();
		return true;
	}
	return false;
}

bool W25Q::canDelete(const TCHAR *file_name) {
	if (strcmp(file_name, fn_tip_calib) == 0)	return false;
	if (strcmp(file_name, fn_tip_backup) == 0)	return false;
	if (strcmp(file_name, fn_cfg) == 0)			return false;
	if (strcmp(file_name, fn_cfg_backup) == 0)	return false;
	return true;
}

TIP_IO_STATUS W25Q::returnStatus(bool keep, TIP_IO_STATUS ret_code) {
	if (!keep) {
		f_close(&cfg_f);
		act_f = W25Q_NONE;
	}
	return ret_code;
}

// Checks the CRC inside tip structure. Returns true if OK, replaces the CRC with the correct value
uint8_t W25Q::TIP_checkSum(TIP* tip, bool write) {
	uint32_t summ = tip->t200;
	summ <<= 1; summ += tip->t260;
	summ <<= 1; summ += tip->t330;
	summ <<= 1; summ += tip->t400;
	summ <<= 1; summ += tip->mask;
	summ <<= 1; summ += tip->ambient;
	for (int i = 0; i < tip_name_sz; ++i) {
		summ <<= 1; summ += (uint8_t)tip->name[i];
	}
	summ += 117;											// To avoid good check sum with all-zero
	uint8_t res = (tip->crc == (summ & 0xFF));
	if (write) tip->crc = summ & 0xFF;
	return res;
}

// Checks the CRC of the RECORD structure. Returns true if OK. Replace the CRC with the correct value if write is true
uint8_t W25Q::CFG_checkSum(RECORD* cfg, bool write) {
	uint16_t 	summ 		= 117;							// To avoid good check sum with all-zero, start with 117
	uint16_t    rec_summ 	= cfg->crc;
	cfg->crc				= 0;
	uint8_t*	d 			= (uint8_t*)cfg;
	for (uint8_t i = 0; i < sizeof(RECORD); ++i) {
		summ <<= 1; summ += d[i];
	}
	bool res = (rec_summ == summ);
	if (write) cfg->crc = summ;
	return res;
}

// Create backup of configuration data
bool W25Q::backup(ACT_FILE type) {
	if (type != W25Q_TIPS_CURRENT && type != W25Q_CONFIG_CURRENT)
		return false;
	const TCHAR* fn = (type == W25Q_TIPS_CURRENT)?fn_tip_calib:fn_cfg;
	const TCHAR* fb = (type == W25Q_TIPS_CURRENT)?fn_tip_backup:fn_cfg_backup;

	FSIZE_t f_size = 0;
	if (type == W25Q_TIPS_CURRENT) {
		FILINFO fno;
		if (FR_OK == f_stat(fn_tip_calib, &fno)) {
			f_size = fno.fsize;								// Ensure the file size is multiple of TIP size
			uint16_t tips = f_size / sizeof(TIP);
			f_size = tips * sizeof(TIPS);
		}
	}

	// Copy data to the backup file
	uint8_t *buff = (uint8_t *)malloc(blk_size);			// Copy buffer
	if (buff == 0)
		return false;
	FIL in_f, out_f;
	if (FR_OK != f_open(&in_f, fn, FA_READ | FA_OPEN_EXISTING)) {
		free(buff);
		return false;
	}
	if (FR_OK != f_open(&out_f, fb, FA_CREATE_ALWAYS | FA_WRITE)) {
		free(buff);
		return false;
	}
	bool ret = true;
	while (true) {
		UINT to_read	= blk_size;
		if (f_size > 0) {
			to_read = f_size;
			if (to_read > blk_size)
				to_read = blk_size;
		}
		UINT read		= 0;
		UINT write		= 0;
		f_read(&in_f, buff, blk_size, &read);				// Read a chunk of data from the source file
		if (read == 0)										// EOF
			break;
		f_size -= read;
		f_write(&out_f, buff, read, &write);				// Write the chunk to the destination file
		if (write < read) {									// Error or disk full
			ret = false;
			break;
		}
	}
	f_close(&in_f);
	f_close(&out_f);
	free(buff);
	return ret;
}
