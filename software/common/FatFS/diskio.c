/*-----------------------------------------------------------------------*/
/* Low level disk I/O module glue functions         (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <debug.h>
#include "diskio.h"		/* FatFs lower layer API */
#include "mmc.h"

//#define DEBUG_TERMINAL 0

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
		BYTE pdrv __attribute__((unused))		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS s =  mmc_disk_status();
	debugf("status = %d", s);
	return s;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
		BYTE pdrv __attribute__((unused))				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS s = mmc_disk_initialize();
	debugf("status = %d", s);
	return s;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read_INTERNAL (
		uint32_t line_number,
		BYTE pdrv __attribute__((unused)),		/* Physical drive nmuber to identify the drive */
		BYTE *buff,		/* Data buffer to store read data */
		DWORD sector,	/* Sector address in LBA */
		UINT count		/* Number of sectors to read */
)
{
	//debugf("Call from %ld", line_number);
	DRESULT r = mmc_disk_read(buff, sector, count);
	debugf("call from %ld buf=%p sector=%ld count=%ld", line_number, buff, sector, count);
	return r;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
		BYTE pdrv __attribute__((unused)),			/* Physical drive nmuber to identify the drive */
		const BYTE *buff,	/* Data to be written */
		DWORD sector,		/* Sector address in LBA */
		UINT count			/* Number of sectors to write */
)
{
	DRESULT r = mmc_disk_write(buff, sector, count);
	debugf("buf=%p sector=%ld count=%ld", buff, sector, count);
	return r;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
		BYTE pdrv __attribute__((unused)),		/* Physical drive nmuber (0..) */
		BYTE cmd,		/* Control code */
		void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT r = mmc_disk_ioctl(cmd, buff);
	debugf("cmd=%d buff=%p", cmd, buff);
	return r;
}
#endif


/*-----------------------------------------------------------------------*/
/* Timer driven procedure                                                */
/*-----------------------------------------------------------------------*/

//
//void disk_timerproc(void)
//{
//	mmc_disk_timerproc();
//}
//


