#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include <stdbool.h>
#include <stdint.h>
#include "fsmc_nor.h"

/********************************************************************************************************
* NOR_FLASH地址映射表
********************************************************************************************************/
//NOR_FLASH
#define NORFLASH_SECTOR_SIZE   				          ((uint32_t)0x00001000)/*扇区大小*/ 


#define NORFLASH_MANAGEM_ADDR                   ((uint32_t)0x00001000)/*网关地址、服务器IP地址、端口号*/
#define NORFLASH_STRATEGY_OFFSET        		    ((uint32_t)0x00001000)

#define UNICODE_TABLE_ADDR (0x0E0000)
#define UNICODE_TABLE_END_ADDR (UNICODE_TABLE_ADDR + 0x3B2E)
#define GBK_TABLE_OFFSET_FROM_UNICODE (0x3B30)
#define GBK_TABLE_ADDR (UNICODE_TABLE_ADDR + GBK_TABLE_OFFSET_FROM_UNICODE)
#define GBK_TABLE_END_ADDR (UNICODE_TABLE_END_ADDR + GBK_TABLE_OFFSET_FROM_UNICODE)

void NorFlashInit(void);
void NorFlashWrite(uint32_t flash, const short *ram, int len);
void NorFlashEraseParam(uint32_t flash);
void NorFlashRead(uint32_t flash, short *ram, int len);
void NorFlashEraseChip(void);

bool NorFlashMutexLock(uint32_t time);
void NorFlashMutexUnlock(void);


#endif
