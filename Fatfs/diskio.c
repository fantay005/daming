/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
//#include "usbdisk.h"	/* Example: USB drive control */
//#include "atadrive.h"	/* Example: ATA drive control */
#include "sdcard.h"		/* Example: MMC/SDC contorl */

/* Definitions of physical drive number for each media */

/* Definitions of physical drive number for each media */



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
) {
	SD_Error Status;
	
	if(pdrv) {
		return STA_NOINIT;
	}
	
	Status = SD_Init();
	if(Status != SD_OK) {
		printf("SD_Init: %d", Status); 
		return STA_NOINIT;
	} else {
		return RES_OK;
	}
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
) {
	return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
) {
	int status;
//	printf("read %d sector at %08x\n", count, sector);
	if(count > 1) {
		status = SD_ReadMultiBlocks((uint32_t)sector * BLOCK_SIZE, (uint32_t *)buff, BLOCK_SIZE, count);
	} else {
		status = SD_ReadBlock((uint32_t)sector * BLOCK_SIZE, (uint32_t *)buff, BLOCK_SIZE);
	}
	if (SD_OK == status)
		return RES_OK;
//	printf("read block error: %d\n", status);
	return RES_ERROR;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if 0
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	if(count > 1) {
		SD_WriteMultiBlocks((uint32_t)sector * BLOCK_SIZE, (uint32_t *)buff,, BLOCK_SIZE, count);
	} else {
		SD_WriteBlock((uint32_t)sector * BLOCK_SIZE, (uint32_t *)buff, BLOCK_SIZE);
	}
	return RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
) {
	 DRESULT sta=RES_ERROR;
	 switch(cmd) {
      case CTRL_SYNC:
        sta=RES_OK;
			  break;                             
			case GET_SECTOR_COUNT:
				sta=RES_OK;
  			break;
			case GET_SECTOR_SIZE:
				*(WORD*)buff = 512;
		  	sta=RES_OK;
		   	break;
			case GET_BLOCK_SIZE:
				sta=RES_OK;
		  	break;
			case CTRL_ERASE_SECTOR:
				sta=RES_OK;
		  	break;
		}
	return RES_OK;

}
#endif
