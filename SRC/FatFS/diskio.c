/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "W25Qxx.h"
#include "sdspi.h"

/* Definitions of physical drive number for each drive */
#define DEV_W25Q16	(0)
#define DEV_SDCARD	(1)

SDCARD sd;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{

	switch (pdrv) {
	case DEV_W25Q16:
		if (W25Qxx_SectorCount() > 0)
		return 0;
		break;
	case DEV_SDCARD:
		return 0;
		break;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{

	switch (pdrv) {
	case DEV_W25Q16:
		return 0;
		break;
	case DEV_SDCARD:
		if (SD_Init(&sd) == 0)
			return 0;
		break;
	}
	return STA_NOINIT;

}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res	 = RES_OK;
	uint8_t code = 0;

	switch (pdrv) {
	case DEV_W25Q16:
		{
		uint32_t addr = sector << 12;	/* sector size is 4096 bytes */
		uint16_t size = count  << 12;
		W25Qxx_RET r = W25Qxx_Read(addr, buff, size);
		switch (r) {
		case W25Qxx_RET_ADDR:
		case W25Qxx_RET_SIZE:
			res = RES_PARERR;
			break;
		case W25Qxx_RET_READ:
			res = RES_ERROR;
			break;
		case W25Qxx_RES_BUSY:
			res = RES_NOTRDY;
			break;
		default:
			break;
		}
		return res;
		}
	case DEV_SDCARD:
		code = SD_Read(&sd, sector, count, buff);
		if (code >= 2)
			res = RES_ERROR;
		else if (code == 1)
			res = RES_NOTRDY;
		return res;
	}
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res  = RES_OK;
	uint8_t code = 0;

	switch (pdrv) {
	case DEV_W25Q16:
		{
		uint32_t addr = sector << 12;	/* sector size is 4096 bytes */
		uint16_t size = count  << 12;
		W25Qxx_RET r = W25Qxx_Write(addr, (uint8_t *)buff, size);
		switch (r) {
		case W25Qxx_RET_ALIGN:
		case W25Qxx_RET_SIZE:
		case W25Qxx_RET_ADDR:
			res = RES_PARERR;
			break;
		case W25Qxx_RET_ERASE:
		case W25Qxx_RET_WRITE:
			res = RES_ERROR;
			break;
		case W25Qxx_RES_BUSY:
			res = RES_NOTRDY;
			break;
		case W25Qxx_RES_RO:
			res = RES_WRPRT;
			break;
		default:
			break;
		}
		return res;
		}
	case DEV_SDCARD:
		code = SD_Write(&sd, sector, count, buff);
		if (code >= 2)
			res = RES_ERROR;
		else if (code == 1)
			res = RES_NOTRDY;
		return res;
	}
	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_OK;

	if (pdrv == DEV_W25Q16) {
		switch (cmd) {
		case CTRL_SYNC:
			res = RES_OK;
		    break;
		case GET_SECTOR_COUNT:
			{
			LBA_t *sc = buff;
			*sc =  W25Qxx_SectorCount(); /* 4k sector number */
			}
			break;
		case GET_SECTOR_SIZE:
			{
			WORD *ss = buff;
			*ss = 4096;
			}
			break;
		case GET_BLOCK_SIZE:
			{
				DWORD *sz_blk = buff;
				*sz_blk = 1;	/* sectors */
			}
			break;
		case CTRL_TRIM:
			{
				LBA_t *lba = buff;
				uint16_t size = lba[1] - lba[0] + 1;
				W25Qxx_RET r = W25Qxx_Erase(lba[0], size);
				switch (r) {
				case W25Qxx_RET_ADDR:
				case W25Qxx_RET_SIZE:
					res = RES_PARERR;
					break;
				case W25Qxx_RET_ERASE:
					res = RES_ERROR;
					break;
				case W25Qxx_RES_BUSY:
					res = RES_NOTRDY;
					break;
				default:
					break;
				}
			}
			break;
		default:
		    res = RES_ERROR;
		    break;
		}
		return res;
	} else if (pdrv == DEV_SDCARD) {
		switch (cmd) {
		case CTRL_SYNC:
			res = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			{
			LBA_t *sc = buff;
			*sc =  sd.blocks; 	/* 512 bytes sector number */
			}
			break;
		case GET_SECTOR_SIZE:
			{
			WORD *ss = buff;
			*ss = 512;
			}
			break;
		case GET_BLOCK_SIZE:
			{
				DWORD *sz_blk = buff;
				*sz_blk = 1;	/* sectors */
			}
			break;
		case CTRL_TRIM:
		default:
			res = RES_ERROR;
			break;
		}
		return res;
	}
	return RES_PARERR;
}

