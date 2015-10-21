#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include <stdbool.h>
#include <stdint.h>

#include "fsmc_nor.h"


#define IPO_PARAM_CONFIG_ADDR  (0x800000 - 4*1024)    // LAST 4K；存储是否为第一次配置中心节点标志
#define NODE_INFOR_NOW_ADDR    (0x800000 - 8*1024)    //倒数第二扇区，存储当前配置的中心节点的信息

#define UNICODE_TABLE_ADDR (0x0E0000)
#define UNICODE_TABLE_END_ADDR (UNICODE_TABLE_ADDR + 0x3B2E)
#define GBK_TABLE_OFFSET_FROM_UNICODE (0x3B30)
#define GBK_TABLE_ADDR (UNICODE_TABLE_ADDR + GBK_TABLE_OFFSET_FROM_UNICODE)
#define GBK_TABLE_END_ADDR (UNICODE_TABLE_END_ADDR + GBK_TABLE_OFFSET_FROM_UNICODE)

void NorFlashInit(void);
void NorFlashWrite(uint32_t flash, const short *ram, int len);
void NorFlashRead(uint32_t flash, short *ram, int len);

bool NorFlashMutexLock(uint32_t time);
void NorFlashMutexUnlock(void);

static inline void NorFlashRead2(uint32_t flash, short *ram, int len) {
	FSMC_NOR_ReadBuffer(ram, flash, len);
}

#endif
